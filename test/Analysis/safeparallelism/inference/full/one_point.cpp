// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=inference %s -verify

#include "../../tbb/parallel_invoke_fake.h"
class Point;

class [[asap::param("P")]] SetXFunctor {
  Point &P;
  int v;
public:
  SetXFunctor(Point &P[[asap::arg("P")]], int v) : P(P), v(v) {} // expected-warning{{Inferred Effect Summary for SetXFunctor: [reads(rpl([rLOCAL],[]))]}}
  void operator() () const;
}; // end class SetXFunctor

class [[asap::param("P")]] SetYFunctor {
  Point &P;
  int v;
public:
  SetYFunctor(Point &P [[asap::arg("P")]], int v) : P(P), v(v) {} // expected-warning{{Inferred Effect Summary for SetYFunctor: [reads(rpl([rLOCAL],[]))]}}
  void operator() () const;
}; // end class SetYFunctor

class //[[asap::region("Rx,Ry")]]
       Point {
  int x; // [[asap::arg("Rx")]];
  int y; // [[asap::arg("Ry")]];
 public:
  void setX(int _x) { x = _x; } // expected-warning{{Inferred Effect Summary for setX: [reads(rpl([rLOCAL],[])),writes(rpl([r8_x],[]))]}}
  void setY(int _y) { y = _y; } // expected-warning{{Inferred Effect Summary for setY: [reads(rpl([rLOCAL],[])),writes(rpl([r9_y],[]))]}}
  void setXY(int _x, int _y) { // expected-warning{{Inferred Effect Summary for setXY: [reads(rpl([r0_P],[])),reads(rpl([r4_P],[])),reads(rpl([rLOCAL],[])),writes(rpl([r8_x],[])),writes(rpl([r9_y],[]))]}}
    SetXFunctor SXF(*this, _x);
    SetYFunctor SYF(*this, _y);
    tbb::parallel_invoke(SXF, SYF);
  }
};

void SetXFunctor::operator() () const { // expected-warning{{Inferred Effect Summary for operator(): [reads(rpl([r0_P],[])),reads(rpl([rLOCAL],[])),writes(rpl([r8_x],[]))]}}
  P.setX(v);
}

void SetYFunctor::operator() () const { // expected-warning{{Inferred Effect Summary for operator(): [reads(rpl([r4_P],[])),reads(rpl([rLOCAL],[])),writes(rpl([r9_y],[]))]}}
  P.setY(v);
}
