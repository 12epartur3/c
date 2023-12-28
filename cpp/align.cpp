#include <iostream>

struct Test {
        char arr[9];
        alignas(16) int a; // a后面要填充4字节
        int a;
        double b;
        char c;
};

int main() {
        auto sizeof_test = sizeof(Test);
        auto alignof_test = alignof(Test);
        auto offset_arr = offsetof(Test, arr);
        auto offset_a = offsetof(Test, a);
        auto offset_b = offsetof(Test, b);
        auto offset_c = offsetof(Test, c);
        std::cout << sizeof_test << '\n';
        std::cout << alignof_test << '\n';
        std::cout << offset_arr << '\n';
        std::cout << offset_a << '\n';
        std::cout << offset_b << '\n';
        std::cout << offset_c << '\n';
}
//对齐要求变量的地址 mod 对齐长度 = 0

//默认对齐规则：结构体的成员按照声明的顺序依次排列，并且每个成员的对齐要求必须被满足。默认情况下，成员的对齐要求通常等于其大小。

//最大对齐要求规则：结构体的对齐要求等于其成员中对齐要求最大的数据类型的对齐要求。这确保结构体的每个成员都满足其对齐要求，并且结构体本身也满足对齐要求。

//结构体的大小是最大对齐长度的整数倍，这样在结构体数组的时候，Test[1]才能满足对齐。
