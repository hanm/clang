// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//
// expected-no-diagnostics

class C {
  int x;

public:
  C() : x(0) {}
  C(int _x) : x(_x) {}

  void operator() [[asap::writes("Global")]] (int _x) { x += _x; }
  void operator+ [[asap::writes("Global")]] (int _x)  { x += _x; }
  int operator- [[asap::writes("Global")]] (int _x)  { x -= _x; return x; }
  void operator* [[asap::writes("Global")]] (int _x)  { x *= _x; }
  void operator/ [[asap::writes("Global")]] (int _x)  { x /= _x; }
  bool operator== [[asap::reads("Global")]] (int _x) { return x==_x; }

  void add   [[asap::writes("Global")]] (int _x) { (*this)(_x); }
  void addv2 [[asap::writes("Global")]] (int _x) { (*this) + _x; }
  void sub   [[asap::writes("Global")]] (int _x) { (*this) - _x; }
  void mult  [[asap::writes("Global")]] (int _x) { (*this) * _x; }
  void div   [[asap::writes("Global")]] (int _x) { (*this) / _x; }
  bool eq    [[asap::reads("Global")]]  (int _x) { return (*this) == _x; }
};

bool operator== [[asap::reads("Global")]] (int _x, C &c) { return c==_x; }

void foo [[asap::writes("Global")]] () {
  C a(3);
  if (3==a) {
    a(a-2);
  }

}
