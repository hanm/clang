// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=inference %s -verify

// There are two points on which setXY is called in parallel, so the 
// inference will produce a solution which has both field *and* object 
// distinction (i.e., using a class region parameter).

#include "../../tbb/parallel_invoke_fake.h"
class Point;

class [[asap::param("P")]] SetXFunctor {
  Point &P; // expected-warning{{Infered region arguments: class Point &, IN:<empty>, ArgV:[p6_P]}}
  int v; // expected-warning{{Infered region arguments: int, IN:[r1_v], ArgV:}}
public:
  SetXFunctor(Point &P[[asap::arg("P")]], int v) : P(P), v(v) {} // expected-warning{{Inferred Effect Summary for SetXFunctor: [reads(rpl([rLOCAL],[]))]}}
  SetXFunctor(SetXFunctor &F) = delete; // expected-warning{{Infered region arguments: class SetXFunctor &, IN:<empty>, ArgV:[r2_F]}}
  SetXFunctor(SetXFunctor &&F) = delete; // expected-warning{{Infered region arguments: class SetXFunctor &&, IN:<empty>, ArgV:[r3_F]}}
  void operator() () const;
}; // end class SetXFunctor

class [[asap::param("P")]] SetYFunctor {
  Point &P; // expected-warning{{Infered region arguments: class Point &, IN:<empty>, ArgV:[p7_P]}}
  int v; // expected-warning{{Infered region arguments: int, IN:[r5_v], ArgV:}}
public:
  SetYFunctor(Point &P [[asap::arg("P")]], int v) : P(P), v(v) {} // expected-warning{{Inferred Effect Summary for SetYFunctor: [reads(rpl([rLOCAL],[]))]}}
  SetYFunctor(SetYFunctor &F) = delete; // expected-warning{{Infered region arguments: class SetYFunctor &, IN:<empty>, ArgV:[r6_F]}}
  SetYFunctor(SetYFunctor &&F) = delete; // expected-warning{{Infered region arguments: class SetYFunctor &&, IN:<empty>, ArgV:[r7_F]}}

  void operator() () const;
}; // end class SetYFunctor

class [[asap::param("P")]] SetXYFunctor {
  Point &P; // expected-warning{{Infered region arguments: class Point &, IN:<empty>, ArgV:[p8_P]}}
  int v1; // expected-warning{{Infered region arguments: int, IN:[r9_v1], ArgV:}}
  int v2; // expected-warning{{Infered region arguments: int, IN:[r10_v2], ArgV:}}

public:
  SetXYFunctor(Point &P [[asap::arg("P")]], int v1, int v2) : P(P), v1(v1), v2(v2) {} // expected-warning{{Inferred Effect Summary for SetXYFunctor: [reads(rpl([rLOCAL],[]))]}}
  SetXYFunctor(SetXYFunctor &F) = delete; // expected-warning{{Infered region arguments: class SetXYFunctor &, IN:<empty>, ArgV:[r11_F]}}
  SetXYFunctor(SetXYFunctor &&F) = delete; // expected-warning{{Infered region arguments: class SetXYFunctor &&, IN:<empty>, ArgV:[r12_F]}}

  void operator() () const;
}; // end class SetYFunctor

class //[[asap::region("Rx,Ry")]]
       Point {
  int x;  // expected-warning{{Infered region arguments: int, IN:[p9_Point], ArgV:}}
  int y;  // expected-warning{{Infered region arguments: int, IN:[p9_Point,r14_y], ArgV:}}
 public:
  void setX(int _x) { x = _x; } // expected-warning{{Inferred Effect Summary for setX: [reads(rpl([rLOCAL],[])),writes(rpl([p9_Point],[]))]}}
  void setY(int _y) { y = _y; } // expected-warning{{Inferred Effect Summary for setY: [reads(rpl([rLOCAL],[])),writes(rpl([p9_Point,r14_y],[]))]}}
  void setXY(int _x, int _y) { // expected-warning{{Inferred Effect Summary for setXY: [reads(rpl([r1_v],[])),reads(rpl([r5_v],[])),reads(rpl([rLOCAL],[])),writes(rpl([p9_Point],[])),writes(rpl([p9_Point,r14_y],[]))]}}
    SetXFunctor SXF(*this, _x); // expected-warning{{Infered region arguments: class SetXFunctor, IN:<empty>, ArgV:[p9_Point]}}
    SetYFunctor SYF(*this, _y); // expected-warning{{Infered region arguments: class SetYFunctor, IN:<empty>, ArgV:[p9_Point]}}
    tbb::parallel_invoke(SXF, SYF);
  }
  Point() : x(0), y(0) {}
  Point(Point &P) = delete; // expected-warning{{Infered region arguments: class Point &, IN:<empty>, ArgV:[r17_P]}}
  Point(Point &&P) = delete; // expected-warning{{Infered region arguments: class Point &&, IN:<empty>, ArgV:[r18_P]}}
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
  Point p1 ; // expected-warning{{Infered region arguments: class Point, IN:<empty>, ArgV:[r19_p1]}}
  Point p2 ; // expected-warning{{Infered region arguments: class Point, IN:<empty>, ArgV:[r20_p2]}}
  SetXYFunctor F1(p1, 3, 4); // expected-warning{{Infered region arguments: class SetXYFunctor, IN:<empty>, ArgV:[r19_p1]}}
  SetXYFunctor F2(p2, 5, 3); // expected-warning{{Infered region arguments: class SetXYFunctor, IN:<empty>, ArgV:[r20_p2]}}
  tbb::parallel_invoke(F1, F2);
}
