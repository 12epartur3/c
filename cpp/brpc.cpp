//brpc 协程切换

//1、首先启动concurrency个pthread，每个pthread传递worker_thread函数进入执行。
int TaskControl::init(int concurrency) {
    if (_concurrency != 0) {
        LOG(ERROR) << "Already initialized";
        return -1;
    }
    if (concurrency <= 0) {
        LOG(ERROR) << "Invalid concurrency=" << concurrency;
        return -1;
    }
    _concurrency = concurrency;

    // Make sure TimerThread is ready.
    if (get_or_create_global_timer_thread() == NULL) {
        LOG(ERROR) << "Fail to get global_timer_thread";
        return -1;
    }
    
    _workers.resize(_concurrency);   
    for (int i = 0; i < _concurrency; ++i) {
        const int rc = pthread_create(&_workers[i], NULL, worker_thread, this);
        if (rc) {
            LOG(ERROR) << "Fail to create _workers[" << i << "], " << berror(rc);
            return -1;
        }
    }

    // Wait for at least one group is added so that choose_one_group()
    // never returns NULL.
    // TODO: Handle the case that worker quits before add_group
    while (_ngroup == 0) {
        usleep(100);  // TODO: Elaborate
    }
    return 0;
}

//2、执行work_thread中的run_main_task。
void* TaskControl::worker_thread(void* arg) {
    run_worker_startfn();    
    
    TaskControl* c = static_cast<TaskControl*>(arg);
    TaskGroup* g = c->create_group();
    TaskStatistics stat;
    if (NULL == g) {
        LOG(ERROR) << "Fail to create TaskGroup in pthread=" << pthread_self();
        return NULL;
    }
    std::string worker_thread_name = butil::string_printf(
        "brpc_worker:%d",
        c->_next_worker_id.fetch_add(1, butil::memory_order_relaxed));
    butil::PlatformThread::SetName(worker_thread_name.c_str());
    BT_VLOG << "Created worker=" << pthread_self()
            << " bthread=" << g->main_tid();

    tls_task_group = g;
    c->_nworkers << 1;
    g->run_main_task();

    stat = g->main_stat();
    BT_VLOG << "Destroying worker=" << pthread_self() << " bthread="
            << g->main_tid() << " idle=" << stat.cputime_ns / 1000000.0
            << "ms uptime=" << g->current_uptime_ns() / 1000000.0 << "ms";
    tls_task_group = NULL;
    g->destroy_self();
    c->_nworkers << -1;
    return NULL;
}

//3、run_main_task中的wait_task采用pthread_cond_wait等待需要调度的协程到来。
void TaskGroup::run_main_task() {
    bvar::PassiveStatus<double> cumulated_cputime(
        get_cumulated_cputime_from_this, this);
    std::unique_ptr<bvar::PerSecond<bvar::PassiveStatus<double> > > usage_bvar;

    TaskGroup* dummy = this;
    bthread_t tid;
    while (wait_task(&tid)) {
        TaskGroup::sched_to(&dummy, tid);
        DCHECK_EQ(this, dummy);
        DCHECK_EQ(_cur_meta->stack, _main_stack);
        if (_cur_meta->tid != _main_tid) {
            TaskGroup::task_runner(1/*skip remained*/);
        }
    }
    // Don't forget to add elapse of last wait_task.
    current_task()->stat.cputime_ns += butil::cpuwide_time_ns() - _last_run_ns;
}

//4、sched_to获取需要调度的协程的TaskMeta，也就是brpc自己写的协程栈，调用jump_stack函数，将%rsp保存在cur_meta->stack，并且将next_meta->stack存到%rsp，完成用户态协程栈的切换。
// context.cpp
// %rdi保存第一个参数cur_meta->stack
// %rsi保存第二个参数next_meta->stack
// "    movq  %rsp, (%rdi)\n"
// "    movq  %rsi, %rsp\n"
// "    cmp  $0, %rcx\n"
// "    je  2f\n"
// "    ldmxcsr  (%rsp)\n"
// "    fldcw  0x4(%rsp)\n"
void TaskGroup::sched_to(TaskGroup** pg, TaskMeta* next_meta) {
    TaskGroup* g = *pg;

    // Save errno so that errno is bthread-specific.
    const int saved_errno = errno;
    void* saved_unique_user_ptr = tls_unique_user_ptr;

    TaskMeta* const cur_meta = g->_cur_meta;
    const int64_t now = butil::cpuwide_time_ns();
    const int64_t elp_ns = now - g->_last_run_ns;
    g->_last_run_ns = now;
    cur_meta->stat.cputime_ns += elp_ns;
    if (cur_meta->tid != g->main_tid()) {
        g->_cumulated_cputime_ns += elp_ns;
    }
    ++cur_meta->stat.nswitch;
    ++ g->_nswitch;
    // Switch to the task
    if (__builtin_expect(next_meta != cur_meta, 1)) {
        g->_cur_meta = next_meta;
        // Switch tls_bls
        cur_meta->local_storage = tls_bls;
        tls_bls = next_meta->local_storage;

        // Logging must be done after switching the local storage, since the logging lib
        // use bthread local storage internally, or will cause memory leak.
        if ((cur_meta->attr.flags & BTHREAD_LOG_CONTEXT_SWITCH) ||
            (next_meta->attr.flags & BTHREAD_LOG_CONTEXT_SWITCH)) {
            LOG(INFO) << "Switch bthread: " << cur_meta->tid << " -> "
                      << next_meta->tid;
        }

        if (cur_meta->stack != NULL) {
            if (next_meta->stack != cur_meta->stack) {
                jump_stack(cur_meta->stack, next_meta->stack);
                // probably went to another group, need to assign g again.
                g = tls_task_group;
            }
#ifndef NDEBUG
            else {
                // else pthread_task is switching to another pthread_task, sc
                // can only equal when they're both _main_stack
                CHECK(cur_meta->stack == g->_main_stack);
            }
#endif
        }
        // else because of ending_sched(including pthread_task->pthread_task)
    } else {
        LOG(FATAL) << "bthread=" << g->current_tid() << " sched_to itself!";
    }

    while (g->_last_context_remained) {
        RemainedFn fn = g->_last_context_remained;
        g->_last_context_remained = NULL;
        fn(g->_last_context_remained_arg);
        g = tls_task_group;
    }

    // Restore errno
    errno = saved_errno;
    tls_unique_user_ptr = saved_unique_user_ptr;

#ifndef NDEBUG
    --g->_sched_recursive_guard;
#endif
    *pg = g;
}

//5、调度完成后执行task_runner，其中m->fn(m->arg)执行了用户函数，后续调用ending_sched(&g)持续切换协程栈调度其他任务，当调度到的协程栈的协程pid是当前pthread线程的pid时结束，
//又开始在wait_task处等待新的需要调度的协程到来。
void TaskGroup::task_runner(intptr_t skip_remained) {
    // NOTE: tls_task_group is volatile since tasks are moved around
    //       different groups.
    TaskGroup* g = tls_task_group;

    if (!skip_remained) {
        while (g->_last_context_remained) {
            RemainedFn fn = g->_last_context_remained;
            g->_last_context_remained = NULL;
            fn(g->_last_context_remained_arg);
            g = tls_task_group;
        }
    }

    do {
        // A task can be stopped before it gets running, in which case
        // we may skip user function, but that may confuse user:
        // Most tasks have variables to remember running result of the task,
        // which is often initialized to values indicating success. If an
        // user function is never called, the variables will be unchanged
        // however they'd better reflect failures because the task is stopped
        // abnormally.

        // Meta and identifier of the task is persistent in this run.
        TaskMeta* const m = g->_cur_meta;

        // Not catch exceptions except ExitException which is for implementing
        // bthread_exit(). User code is intended to crash when an exception is
        // not caught explicitly. This is consistent with other threading
        // libraries.
        void* thread_return;
        try {
            thread_return = m->fn(m->arg);
        } catch (ExitException& e) {
            thread_return = e.value();
        }

        // Group is probably changed
        g = tls_task_group;

        // TODO: Save thread_return
        (void)thread_return;


        // Clean tls variables, must be done before changing version_butex
        // otherwise another thread just joined this thread may not see side
        // effects of destructing tls variables.
        KeyTable* kt = tls_bls.keytable;
        if (kt != NULL) {
            return_keytable(m->attr.keytable_pool, kt);
            // After deletion: tls may be set during deletion.
            tls_bls.keytable = NULL;
            m->local_storage.keytable = NULL; // optional
        }

        // Increase the version and wake up all joiners, if resulting version
        // is 0, change it to 1 to make bthread_t never be 0. Any access
        // or join to the bthread after changing version will be rejected.
        // The spinlock is for visibility of TaskGroup::get_attr.
        {
            BAIDU_SCOPED_LOCK(m->version_lock);
            if (0 == ++*m->version_butex) {
                ++*m->version_butex;
            }
        }
        butex_wake_except(m->version_butex, 0);

        g->_control->_nbthreads << -1;
        g->set_remained(TaskGroup::_release_last_context, m);
        ending_sched(&g);

    } while (g->_cur_meta->tid != g->_main_tid);

    // Was called from a pthread and we don't have BTHREAD_STACKTYPE_PTHREAD
    // tasks to run, quit for more tasks.
}
