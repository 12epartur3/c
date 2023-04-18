// 虚基类在菱形继承中，只占有一份内存
// 这样是通过指向虚基类的指针完成的
// 深度探索c++对象模型，第三章page155
class A {
};

class B : public virtual A {
};

class C : public virtual A {
};
//到这里，B和C中分别有一个vptr，这个vptr指向 virtual base calss table，真正的virtual base class指针放在该表格中

class D : public B C {
};
// 这里就是菱形继承，D由B和C构成，B和C中两个vptr指向两张表，表中第一项指向同一个虚基类A，实现对虚基类的复用
// 以上是Microsoft的实现

// 深度探索c++对象模型，第三章page157
// Bjarne是通过将虚基类的偏移放在虚函数表中实现的，这里放的是虚基类相对于该派生类起始地址的offset。
// B放有一个vptr指向虚函数表，虚函数表的正向偏移*vptr[x]，x >= 0，存放的是虚函数的地址，负向偏移，x<0，存放的是虚基类相对于B起始地址的偏移。
// C同样也是这样实现。

class Point3d {
  public:
  int x_;
  int y_;
  int z_;
};

// &Point3d::x_ = 0
// &Point3d::y_ = 4
// &Point3d::y_ = 8
// 这种操作返回的是变量在class中的偏移，存取的时候直接通过具体变量地址+偏移就可以访问。

// 深度探索c++对象模型，第三章page133
Point3d origin, *pt = &origin;
origin.x = 0.0;
pt->x = 0.0;

// 当origin是一个派生类，并且继承结构中有一个虚基类，并且x是从该虚基类继承过来的member时，会有重大的差异
