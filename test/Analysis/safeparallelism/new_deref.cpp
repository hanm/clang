// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//
// expected-no-diagnostics

class C {
  int x;
public:
  C(int _x) : x(_x) {}
};

void foo(int x) {
  C &c = * new C(x);
}
