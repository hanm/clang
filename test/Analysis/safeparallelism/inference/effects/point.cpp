// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=inference %s -verify
//
// 

class [[asap::region("Rx,Ry")]] Point {
  int x [[asap::arg("Rx")]];
  int y [[asap::arg("Ry")]];
 public:
  void setX(int _x) { x = _x; } // expected-warning{{Solution for setX: [Reads Effect on Local,Writes Effect on Rx]}}
  void setY(int _y) { y = _y; } // expected-warning{{Solution for setY: [Reads Effect on Local,Writes Effect on Ry]}}
  void setXY(int _x, int _y);
};
