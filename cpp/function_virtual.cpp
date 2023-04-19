class A {
  public:
    void func(int x) {}
    virtual void vfunc(int x) {}
    static void sfunc(int x) {}
  private:
    int x;
  
};

// non static member function
A a;
A *p = &a;
a.func(5);
p->func(5);
// non static 成员函数会被转换成 non member function
// void A_func_I(A* this, int x) {}
// 通过name mangling，命名重写，并且加上this指针作为第一个参数。
// a.func(5); = A_func_I(&a, 5);
// p->func(5); =  A_func_I(p, 5);


// virtual function
// 虚函数的调用通过虚函数表来实现，每个含有虚函数或者继承自含有虚函数的类，都会有独立的虚函数表。虚函数的名字也会被name mangling
p->vfunc(x, 5);
// (*p->vptr[1])(p, 5)
a.vfunc(x, 5);
// 知道明确类型的虚函数调用，会被明确的决议，因为已经知道是A类型在调用了，不需要运行时决议。
// A_vfunc_I(&a, 5)

// static member function
// 静态成员函数的调用也会被转换成 non member function，但是不需要this指针了
// void A_sfunc_I(int x) {}
p->sfunc(5);
a.sfunc(5);
// A_sfunc_I(5)
// 由于静态成员函数没有this指针，所以其不能操作非静态的成员变量
// static void sfunc(int x) { a = 99;}
// static virtual void sfunc(int x) {}
// static void sfunc(int x) const {}
// static void sfunc(int x) volatile {}
// 以上这几种函数时不能编译通过的
// 获取一个static member function的地址，将获得其在内存中的真正地址。
// &A::sfunc(int x) 类型是 void (*)(int)，而不是void (A::*)(int)
  
