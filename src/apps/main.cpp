#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#endif
//#include "utils/CLI11.hpp" // IWYU pragma: export
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include <iostream>
#include <string>
#include <utility>

void g(int&& b) {
    int&& a = std::move(b);
    std::cout << "RREF";
    std::cout << "B = " << b << std::endl;
    std::cout << "B A = " << a << std::endl;
}

void g(int& b) {
    std::cout << "LREF";
    std::cout << "B = " << b << std::endl;
}

void f(int&& a) {
    std::cout << "A = " << a << std::endl;
    g(std::move(a));
    g(a);
}

class Member {
public:
    Member()
        : a(0)
        , b(0) {
        std::cout << "Member()" << std::endl;
    }

    Member(int a, int b)
        : a(a)
        , b(b) {
        std::cout << "Member(int a, int b)" << std::endl;
    }

    Member(Member&& member)
        : a(std::move(member.a))
        , b(std::move(member.b)) {
        std::cout << "Member(Member&&)" << std::endl;
        member.a = 0;
        member.b = 0;
    }

    Member(const Member& member)
        : a(member.a)
        , b(member.b) {
        std::cout << "Member(const Member&)" << std::endl;
    }

    Member& operator=(Member&& member) {
        std::cout << "Member& operator=(const Member&)" << std::endl;
        a = std::move(member.a);
        b = std::move(member.b);
        member.a = 0;
        member.b = 0;
        return *this;
    }

    Member& operator=(const Member& member) {
        std::cout << "Member& operator=(const Member&)" << std::endl;
        a = member.a;
        b = member.b;
        return *this;
    }

    void print() {
        std::cout << "Test: a = " << a << std::endl;
        std::cout << "Test: b = " << b << std::endl;
    }

private:
    int a;
    int b;
};

class Test {
public:
    Test() {
    }

    Test(int a, int b)
        : member(a, b) {
        std::cout << "Test(int a, int b)" << std::endl;
    }

    Test(Test&& test)
        : member(std::move(test.member)) {
        std::cout << "Test(Test&& test)" << std::endl;
    }

    Test(const Test& test)
        : member(test.member) {
        std::cout << "Test(const Test& test)" << std::endl;
    }

    Test& operator=(Test&& test) {
        std::cout << "Test& operator=(Test&& test)" << std::endl;
        member = std::move(test.member);
        return *this;
    }

    Test& operator=(const Test& test) {
        std::cout << "Test& operator=(const Test& test)" << std::endl;
        member = test.member;
        return *this;
    }

    void print(const std::string& message) {
        std::cout << "Message: " << message << std::endl;
        member.print();
        std::cout << std::endl;
    }

private:
    Member member;
};

Test getTest(int a, int b) {
    return Test(a, b);
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    Test test1(5, 6);
    test1.print("Test test1(5, 6)");

    Test test2;
    test2.print("Test test2");

    test2 = test1;
    test2.print("test2 = test1");

    Test test3(Test(7, 8));
    test3.print("Test test3(Test(7, 8))");

    Test test4(test3);
    test4.print("Test test4(test3)");

    Test test5(std::move(test3));
    test4.print("Test test4(std::move(test3))");

    test3.print("Should be 0, 0");

    bool cond = false;

    Test test6;
    test6 = cond ? getTest(7, 8) : Test(9, 10);

    test6.print("test6 = cond ? getTest(7, 8) : Test(9, 10)");

    return 0;
}
