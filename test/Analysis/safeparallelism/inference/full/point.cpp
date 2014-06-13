// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=inference %s -verify
// XFAIL: *
// expected-no-diagnostics
#include "../../tbb/parallel_invoke_fake.h"
class Point;

class SetXFunctor {
  Point &P;
  int v;
public:
  SetXFunctor(Point &P, int v) : P(P), v(v) {}
  void operator() () const;
}; // end class SetXFunctor

class SetYFunctor {
  Point &P;
  int v;
public:
  SetYFunctor(Point &P, int v) : P(P), v(v) {}
  void operator() () const;
}; // end class SetYFunctor

class [[asap::region("Rx,Ry")]] Point {
  int x; // [[asap::arg("Rx")]];
  int y; // [[asap::arg("Ry")]];
 public:
  void setX(int _x) { x = _x; }
  void setY(int _y) { y = _y; }
  void setXY(int _x, int _y) { 
    SetXFunctor SXF(*this, _x);
    SetYFunctor SYF(*this, _y);
    tbb::parallel_invoke(SXF,SYF);
  }
};

void SetXFunctor::operator() () const {
  P.setX(v);
}

void SetYFunctor::operator() () const {
  P.setY(v);
}
