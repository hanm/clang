// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//
// expected-no-diagnostics

class C {
  int x;

public:
  C() : x(0) {}
  C(int _x) : x(_x) {}

  void operator()(int _x) { x += _x; }

  void add(int _x) { (*this)(_x); }
};
