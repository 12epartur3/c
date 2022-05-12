#include <iostream>
/*
c++03中，函数的返回值有一个临时变量来承接，如函数：
A fun()
{
  A a;
  return a;
}
执行：
A a = fun();
此时会分解步骤为:
tmp = fun();
A a = tmp;
会看到一次构造函数（函数中定义A a），两次移动构造（tmp = return a和 A a = tmp）
编译方式：g++ rvo.cpp -std=c++11 -fno-elide-constructors

c++11中，采用rvo方式，避免tmp的产生，只会调用一次构造函数（函数中定义A a）
编译方式：g++ rvo.cpp -std=c++11

关于参数 -fno-elide-constructors：
The C ++ standard allows an implementation to omit creating a temporary which is only used to initialize another object of the same type. Specifying this option disables that optimization, and forces G++ to call the copy constructor in all cases.
（https://linux.die.net/man/1/g++）
*/
class A {
  public:
    A() {
      std::cout << "[C] constructor fired." << std::endl;
    }

    A(const A &a) {
      std::cout << "[C] copying constructor fired." << std::endl;
      x = a.x;
    }

    A(A &&a) {
      std::cout << "[C] moving copying constructor fired." << std::endl;
      x = a.x;
    }

    ~A() {
      std::cout << "[C] destructor fired." << std::endl;
    }
    int x;
};

A getTempA() {
  A a;
  a.x = 99999;
  std::cout<< __func__ << '\n';
  return a;
  //return std::move(a);
}

int main(int argc, char **argv) {
  A a = getTempA();
  std::cout<< "x = "<< a.x<< '\n';
  return 0;
}
