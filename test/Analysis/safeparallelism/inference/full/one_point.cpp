// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=inference %s -verify

// There are no two points on which setXY is called in parallel, so the 
// inference will produce a solution which has only field distinction,
// but not object distinction (i.e., using a class region parameter).

#include "../../tbb/parallel_invoke_fake.h"
class Point;

class [[asap::param("P")]] SetXFunctor {
  Point &P; // expected-warning{{Infered region arguments: class Point &, IN:<empty>, ArgV:[p4_P]}}
  int v;    // expected-warning{{Infered region arguments: int, IN:[r1_v], ArgV:}}
public:
  SetXFunctor(Point &P[[asap::arg("P")]], int v) : P(P), v(v) {} // expected-warning{{Inferred Effect Summary for SetXFunctor: [reads(rpl([rLOCAL],[]))]}}

  SetXFunctor(SetXFunctor &F) = delete; // expected-warning{{Infered region arguments: class SetXFunctor &, IN:<empty>, ArgV:[r2_F]}}
  SetXFunctor(SetXFunctor &&F) = delete; // expected-warning{{Infered region arguments: class SetXFunctor &&, IN:<empty>, ArgV:[r3_F]}}  
  void operator() () const;
}; // end class SetXFunctor

class [[asap::param("P")]] SetYFunctor {
  Point &P;  // expected-warning{{Infered region arguments: class Point &, IN:<empty>, ArgV:[p5_P]}}  
  int v; // expected-warning{{Infered region arguments: int, IN:[r5_v], ArgV:}}  
public:
  SetYFunctor(Point &P [[asap::arg("P")]], int v) : P(P), v(v) {} // expected-warning{{Inferred Effect Summary for SetYFunctor: [reads(rpl([rLOCAL],[]))]}}
  SetYFunctor(SetYFunctor &F) = delete; // expected-warning{{Infered region arguments: class SetYFunctor &, IN:<empty>, ArgV:[r6_F]}}  
  SetYFunctor(SetYFunctor &&F) = delete; // expected-warning{{Infered region arguments: class SetYFunctor &&, IN:<empty>, ArgV:[r7_F]}}  
  void operator() () const;
}; // end class SetYFunctor

class //[[asap::region("Rx,Ry")]]
       Point {
  int x;  // expected-warning{{Infered region arguments: int, IN:[r8_x], ArgV:}}
  int y;  // expected-warning{{Infered region arguments: int, IN:[r9_y], ArgV:}}
 public:
  void setX(int _x) { x = _x; } // expected-warning{{Inferred Effect Summary for setX: [reads(rpl([rLOCAL],[])),writes(rpl([r8_x],[]))]}}
  void setY(int _y) { y = _y; } // expected-warning{{Inferred Effect Summary for setY: [reads(rpl([rLOCAL],[])),writes(rpl([r9_y],[]))]}}
  void setXY(int _x, int _y) { // expected-warning{{Inferred Effect Summary for setXY: [reads(rpl([r1_v],[])),reads(rpl([r5_v],[])),reads(rpl([rLOCAL],[])),writes(rpl([r8_x],[])),writes(rpl([r9_y],[]))]}}
    SetXFunctor SXF(*this, _x); // expected-warning{{Infered region arguments: class SetXFunctor, IN:<empty>, ArgV:[p6_Point]}}
    SetYFunctor SYF(*this, _y); // expected-warning{{Infered region arguments: class SetYFunctor, IN:<empty>, ArgV:[p6_Point]}}
    tbb::parallel_invoke(SXF, SYF);
  }
};

void SetXFunctor::operator() () const { // expected-warning{{Inferred Effect Summary for operator(): [reads(rpl([r1_v],[])),reads(rpl([rLOCAL],[])),writes(rpl([r8_x],[]))]}}
  P.setX(v);
}

void SetYFunctor::operator() () const { // expected-warning{{Inferred Effect Summary for operator(): [reads(rpl([r5_v],[])),reads(rpl([rLOCAL],[])),writes(rpl([r9_y],[]))]}}
  P.setY(v);
}
