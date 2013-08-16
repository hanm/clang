// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.GlobalAccessChecker %s -verify
//
// expected-no-diagnostics

struct C { 
 union {
    struct {
      int x, y, z;
    }; 
    int xyz[3];
  }; 
};

void foo() {
  C x;
  x.xyz[0] = x.x;
  x.y = x.xyz[1];
  x.xyz[x.x] = x.xyz[x.y];
}
