//refer:https://www.justsoftwaresolutions.co.uk/cplusplus/rvalue_references_and_perfect_forwarding.html


// 右值和左值的区别
// The key difference is of course that an rvalue reference can bind to an rvalue, whereas a non-const lvalue reference cannot.
// 右值引用可以绑定到那些非const左值引用不等绑定的右值，这里说的其实就是临时值/将亡值

// 完美转发
// When you combine rvalue references with function templates you get an interesting interaction: 
// if the type of a function parameter is an rvalue reference to a template type parameter then the type parameter is deduce to be an lvalue reference if an lvalue is passed, and a plain type otherwise
// 当右值引用和模板函数放在一起的时候，会有一些奇怪的现象：，那么该参数的类型将退化成&x，
