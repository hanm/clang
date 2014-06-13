// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=inference %s -verify
// XFAIL: *
// 

class [[asap::region("Rx,Ry")]] Point {
  int x ;
  int y ;
 public:
  void setX(int _x) { x = _x; } // expected-warning{{Inferred Effect Summary for setX: [reads(rpl([rLOCAL],[])),writes(rpl([r0_Rx],[]))]}}
  void setY(int _y) { y = _y; } // expected-warning{{Inferred Effect Summary for setY: [reads(rpl([rLOCAL],[])),writes(rpl([r1_Ry],[]))]}}
  void setXY(int _x, int _y) {  // expected-warning{{Inferred Effect Summary for setXY: [writes(rpl([r0_Rx],[])),writes(rpl([r1_Ry],[])),reads(rpl([rLOCAL],[]))]}}
    setX(_x);
    setY(_y);
  }
};
