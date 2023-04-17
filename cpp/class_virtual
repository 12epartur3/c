// 虚基类在菱形继承中，只占有一份内存
// 这样是通过指向虚基类的指针完成的

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
