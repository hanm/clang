// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=inference %s -verify
// XFAIL:*

// There are two points on which setXY is called in parallel, so the 
// inference will produce a solution which has only object 
// distinction (i.e., using a class region parameter).

#include "../../tbb/parallel_invoke_fake.h"
class Point;

class [[asap::param("P")]] SetXYFunctor {
  Point &P;
  int v1;
  int v2;

public:
  SetXYFunctor(Point &P [[asap::arg("P")]], int v1, int v2) : P(P), v1(v1), v2(v2) {} // expected-warning{{Inferred Effect Summary for SetYFunctor: [reads(rpl([rLOCAL],[]))]}}
  void operator() () const;
}; // end class SetXYFunctor

class //[[asap::region("Rx,Ry")]]
       Point {
  int x; // [[asap::arg("Rx")]];
  int y; // [[asap::arg("Ry")]];
 public:
  void setX(int _x) { x = _x; } // expected-warning{{Inferred Effect Summary for setX: [reads(rpl([rLOCAL],[])),writes(rpl([r8_x],[]))]}}
  void setY(int _y) { y = _y; } // expected-warning{{Inferred Effect Summary for setY: [reads(rpl([rLOCAL],[])),writes(rpl([r9_y],[]))]}}
  void setXY(int _x, int _y) { 
    x = _x;
    y = _y;
  }
};

void SetXYFunctor::operator() () const { // expected-warning{{Inferred Effect Summary for operator(): [reads(rpl([r4_P],[])),reads(rpl([rLOCAL],[])),writes(rpl([r9_y],[]))]}}
  P.setXY(v1, v2);
}

void foo //[[asap::region("R1, R2")]] 
         () {
  Point p1 ;//[[asap::arg("R1")]];
  Point p2 ;//[[asap::arg("R2")]];
  SetXYFunctor F1(p1, 3, 4);
  SetXYFunctor F2(p2, 5, 3);
  tbb::parallel_invoke(F1, F2);
}
