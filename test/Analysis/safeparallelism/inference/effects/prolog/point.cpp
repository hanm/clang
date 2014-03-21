// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=inference %s -verify
//
// expected-no-diagnostics

class [[asap::region("Rx,Ry")]] Point {
  int x [[asap::arg("Rx")]];
  int y [[asap::arg("Ry")]];
 public:
  void setX(int _x) { x = _x; }
  void setY(int _y) { y = _y; }
  void setXY(int _x, int _y);
};
