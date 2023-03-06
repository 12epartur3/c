// refer:https://www.justsoftwaresolutions.co.uk/cplusplus/rvalue_references_and_perfect_forwarding.html
// https://www.jianshu.com/p/af7c2314e2dc


// 右值和左值的区别
// The key difference is of course that an rvalue reference can bind to an rvalue, whereas a non-const lvalue reference cannot.
// 右值引用可以绑定到那些非const左值引用不能绑定的右值，这里说的其实就是临时值/将亡值

// 完美转发
// When you combine rvalue references with function templates you get an interesting interaction: 
// if the type of a function parameter is an rvalue reference to a template type parameter then the type parameter is deduce to be an lvalue reference 
// if an lvalue is passed, and a plain type otherwise
// 当右值引用和函数模板放在一起的时候，会有一些奇怪的现象：如果函数的参数类型是模板参数的右值引用&&T，当传一个左值进来给这个函数，那么这个函数参数将推断成左值引用，
// 如果是其他类型（如右值），那么这个参数就推断成一个普通类型。

// 有什么用
// Why is this useful? Well, it means that a function template can pass its arguments through to another function whilst retaining the lvalue/rvalue 
// nature of the function arguments by using std::forward. This is called "perfect forwarding", avoids excessive copying, and avoids the template 
// author having to write multiple overloads for lvalue and rvalue

// 只在函数模板里面用
// Note that this is unique to template functions

void g(X&& t); // A
void g(X& t);      // B

template<typename T>
void f(T&& t)
{
    g(std::forward<T>(t));
}

void h(X&& t)
{
    g(t);
}

int main()
{
    X x;
    f(x);   // 1
    f(X()); // 2
    h(x);
    h(X()); // 3
}

// At line "1", we pass a named X object to f, so T is deduced to be an lvalue reference: X&, as we saw above. When T is an lvalue reference, 
// std::forward<T> is a no-op: it just returns its argument. We therefore call the overload of g that takes an lvalue reference (line B).

// At line "2", we pass a temporary to f, so T is just plain X. In this case, std::forward<T>(t) is equivalent to static_cast<T&&>(t): 
// it ensures that the argument is forwarded as an rvalue reference. This means that the overload of g that takes an rvalue reference is selected (line A).

