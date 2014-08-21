// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=inference %s -verify

// There are two points on which setXY is called in parallel, so the 
// inference will produce a solution which has both field *and* object 
// distinction (i.e., using a class region parameter).

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

class [[asap::param("P")]] SetXYFunctor {
  Point &P;
  int v1;
  int v2;

public:
  SetXYFunctor(Point &P [[asap::arg("P")]], int v1, int v2) : P(P), v1(v1), v2(v2) {} // expected-warning{{Inferred Effect Summary for SetXYFunctor: [reads(rpl([rLOCAL],[]))]}}
  void operator() () const;
}; // end class SetYFunctor

class //[[asap::region("Rx,Ry")]]
       Point {
  int x; // [[asap::arg("Rx")]];
  int y; // [[asap::arg("Ry")]];
 public:
  void setX(int _x) { x = _x; } // expected-warning{{Inferred Effect Summary for setX: [reads(rpl([rLOCAL],[])),writes(rpl([p9_Point],[]))]}}
  void setY(int _y) { y = _y; } // expected-warning{{Inferred Effect Summary for setY: [reads(rpl([rLOCAL],[])),writes(rpl([p9_Point,r14_y],[]))]}}
  void setXY(int _x, int _y) { // expected-warning{{Inferred Effect Summary for setXY: [reads(rpl([r1_v],[])),reads(rpl([r5_v],[])),reads(rpl([rLOCAL],[])),writes(rpl([p9_Point],[])),writes(rpl([p9_Point,r14_y],[]))]}}
    SetXFunctor SXF(*this, _x);
    SetYFunctor SYF(*this, _y);
    tbb::parallel_invoke(SXF, SYF);
  }
};

void SetXFunctor::operator() () const { // expected-warning{{Inferred Effect Summary for operator(): [reads(rpl([r1_v],[])),reads(rpl([rLOCAL],[])),writes(rpl([p6_P],[]))]}}
  P.setX(v);
}

void SetYFunctor::operator() () const { // expected-warning{{Inferred Effect Summary for operator(): [reads(rpl([r5_v],[])),reads(rpl([rLOCAL],[])),writes(rpl([p7_P,r14_y],[]))]}}
  P.setY(v);
}

void SetXYFunctor::operator() () const { // expected-warning{{Inferred Effect Summary for operator(): [reads(rpl([r10_v2],[])),reads(rpl([r1_v],[])),reads(rpl([r5_v],[])),reads(rpl([r9_v1],[])),reads(rpl([rLOCAL],[])),writes(rpl([p8_P],[])),writes(rpl([p8_P,r14_y],[]))]}}
  P.setXY(v1, v2);
}

void foo //[[asap::region("R1, R2")]] 
         () { // expected-warning{{Inferred Effect Summary for foo: [reads(rpl([r10_v2],[])),reads(rpl([r1_v],[])),reads(rpl([r5_v],[])),reads(rpl([r9_v1],[])),reads(rpl([rLOCAL],[])),writes(rpl([r19_p1],[])),writes(rpl([r19_p1,r14_y],[])),writes(rpl([r20_p2],[])),writes(rpl([r20_p2,r14_y],[]))]}}
  Point p1 ;//[[asap::arg("R1")]];
  Point p2 ;//[[asap::arg("R2")]];
  SetXYFunctor F1(p1, 3, 4);
  SetXYFunctor F2(p2, 5, 3);
  tbb::parallel_invoke(F1, F2);
}
