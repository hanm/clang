// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
// expected-no-diagnostics

class empty_class
    {
public:
    void func1() {}
    };

void func1()
    {
    empty_class ec;
    ec.func1();
    }

