// 虚基类在菱形继承中，只占有一份内存
// 这样是通过指向虚基类的指针完成的
// 深度探索c++对象模型，第三章page155
class A {
};

class B : public virtual A {
};

class C : public virtual A {
};
//到这里，B和C中分别有一个vptr，实例化后指向自己对应的虚基类A

class D : public B C {
};
//这里就是菱形继承，D由B和C构成，B和C中两个vptr指向同一个虚基类A，实现对虚基类的复用
//以上是Microsoft的实现

// 深度探索c++对象模型，第三章page157
// Bjarne是通过将虚基类的偏移放在虚函数表中实现的。
// B放有一个vptr指向虚函数表，虚函数表的正向偏移*vptr[x]，x >= 0，存放的是虚函数的地址，负向偏移，x<0，存放的是虚基类相对于B起始地址的偏移。
// C同样也是这样实现。
