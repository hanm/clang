// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=inference %s -verify
// 

class [[asap::region("Rx,Ry")]] Point {
  int x ; // expected-warning{{Inferred region arguments: int, IN:[r2_x], ArgV:}}
  int y ; // expected-warning{{Inferred region arguments: int, IN:[r3_y], ArgV:}}
 public:
  void setX(int _x) { x = _x; } // expected-warning{{Inferred Effect Summary for setX: [reads(rpl([rLOCAL],[])),writes(rpl([r2_x],[]))]}}
  void setY(int _y) { y = _y; } // expected-warning{{Inferred Effect Summary for setY: [reads(rpl([rLOCAL],[])),writes(rpl([r3_y],[]))]}}
  void setXY(int _x, int _y) {  // expected-warning{{Inferred Effect Summary for setXY: [reads(rpl([rLOCAL],[])),writes(rpl([r2_x],[])),writes(rpl([r3_y],[]))]}}
    setX(_x);
    setY(_y);
  }
};
