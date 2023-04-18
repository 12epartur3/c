class A {
  public:
    void func(int x) {}
  
};
// non static member function
A a;
A *p = &a;
a.func(5);
p->func(5);
//  non static member function函数会被转换成 non member function

void A_func_I(A* this, int x) {}
// 通过name mangling，命名重写，并且加上this指针作为第一个参数。
// a.func(5); = A_func_I(&a, 5);
// p->func(5); =  A_func_I(p, 5);
