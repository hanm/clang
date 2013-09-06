// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//
// expected-no-diagnostics

class C {
  int x;
public:
  C(int _x) : x(_x) {}
  C() : x(0) {}
};

void foo(int x) {
  C &c = * new C(x);
  //C *a = new  C(x)[10]; not allowed
  //C *a = new  C[10](x); not allowed
  C *a1 = new  C[10]();
  C *a2 = new  C[10];
}
