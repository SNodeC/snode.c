#ifdef __GNUC__
#pragma GCC diagnostic push
#ifdef __has_warning
#if __has_warning("-Wfloat-equal")
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
#if __has_warning("-Wweak-vtables")
#pragma GCC diagnostic ignored "-Wweak-vtables"
#endif
#if __has_warning("-Wcovered-switch-default")
#pragma GCC diagnostic ignored "-Wcovered-switch-default"
#endif
#endif
#endif
// #include "utils/CLI11.hpp"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include <cstdint>
#include <iostream>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

class Vector {
public:
    explicit Vector(std::vector<char>::size_type size) {
        vector.reserve(size);
    }

    explicit Vector(const Vector& vector)
        : vector(vector.vector.begin(), vector.vector.end()) {
        this->vector.reserve(vector.vector.capacity());
    }

    virtual ~Vector() = default;

    std::vector<char>::size_type size() {
        return vector.size();
    }

    std::vector<char>::size_type capacity() {
        return vector.capacity();
    }

    void resize(std::vector<char>::size_type size) {
        vector.resize(size);
    }

    void shrink_to_fit() {
        vector.shrink_to_fit();
    }

    void clear() {
        vector.clear();
    }

    operator char*() {
        return vector.data();
    }

    virtual operator std::vector<char>::size_type() = 0;

protected:
    std::vector<char> vector;
};

class OutVector : public Vector {
public:
    using Vector::Vector;

    operator std::vector<char>::size_type() override {
        return Vector::vector.size();
    }
};

class InVector : public Vector {
public:
    using Vector::Vector;

    operator std::vector<char>::size_type() override {
        return Vector::vector.capacity();
    }
};

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

    Member(Member&& member) noexcept
        : a(member.a)
        , b(member.b) {
        std::cout << "Member(Member&&)" << std::endl;
        member.a = 0;
        member.b = 0;
    }

    Member(const Member& member)
        : a(member.a)
        , b(member.b) {
        std::cout << "Member(const Member&)" << std::endl;
    }

    Member& operator=(Member&& member) noexcept {
        std::cout << "Member& operator=(Member&&)" << std::endl;
        a = member.a;
        b = member.b;
        member.a = 0;
        member.b = 0;
        return *this;
    }

    Member& operator=(const Member& member) {
        std::cout << "Member& operator=(const Member&)" << std::endl;
        if (this != &member) {
            a = member.a;
            b = member.b;
        }
        return *this;
    }

    void print() const {
        std::cout << "Test: a = " << a << std::endl;
        std::cout << "Test: b = " << b << std::endl;
    }

    // private:
    int a;
    int b;
};

class Test {
public:
    Test() {
        std::cout << "Test()" << std::endl;
    }

    Test(int a, int b)
        : member(a, b) {
        std::cout << "Test(int a, int b)" << std::endl;
    }

    Test(Test&& test) noexcept
        : member(std::move(test.member)) {
        std::cout << "Test(Test&& test)" << std::endl;
    }

    Test(const Test& test)
        : member(test.member) {
        std::cout << "Test(const Test& test)" << std::endl;
    }

    Test& operator=(Test&& test) noexcept {
        std::cout << "Test& operator=(Test&& test)" << std::endl;
        member = std::move(test.member);
        return *this;
    }

    Test& operator=(const Test& test) {
        std::cout << "Test& operator=(const Test& test)" << std::endl;
        if (this != &test) {
            member = test.member;
        }
        return *this;
    }

    void print(const std::string& message) const {
        std::cout << "Message: " << message << std::endl;
        member.print();
        std::cout << "-----------------------------" << std::endl;
    }

    // private:
    Member member;
};

Test getTest(int a, int b) {
    return Test(a, b);
}

static void test(uint32_t uintValue) {
    std::cout << "Test UINT32_t: " << uintValue << std::endl;
}

static void test(int intValue) {
    std::cout << "Test INT: " << intValue << std::endl;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    test(8080);
    test(8080U);

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

    [[maybe_unused]] const Test test5(std::move(test3));
    test4.print("Test test4(std::move(test3))");

    test3.print("Should be 0, 0");

    Test test8;
    test8 = std::move(test4);

    const bool cond = false;

    Test test6;

    test6 = cond ? test6 : Test(9, 10);

    test6.print("test6 = cond ? getTest(7, 8) : Test(9, 10)");

    [[maybe_unused]] Test&& test7(std::move(test6));

    InVector inVector(17);
    read(0, inVector, inVector);

    OutVector outVector(27);
    write(0, outVector, outVector);

    return 0;
}
