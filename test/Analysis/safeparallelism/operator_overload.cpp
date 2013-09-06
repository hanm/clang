// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//
// expected-no-diagnostics

class C {
  int x;

public:
  C() : x(0) {}
  C(int _x) : x(_x) {}

  void operator()(int _x) { x += _x; }
  void operator+(int _x)  { x += _x; }
  int operator-(int _x)  { x -= _x; return x; }
  void operator*(int _x)  { x *= _x; }
  void operator/(int _x)  { x /= _x; }
  bool operator==(int _x) { return x==_x; }

  void add(int _x)   { (*this)(_x); }
  void addv2(int _x) { (*this) + _x; }
  void sub(int _x)   { (*this) - _x; }
  void mult(int _x)  { (*this) * _x; }
  void div(int _x)   { (*this) / _x; }
  bool eq(int _x)    { return (*this) == _x; }
};

bool operator==(int _x, C &c) { return c==_x; }

void foo() {
  C a(3);
  if (3==a) {
    a(a-2);
  }

}
