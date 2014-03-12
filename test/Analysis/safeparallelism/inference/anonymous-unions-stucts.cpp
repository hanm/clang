// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=inference %s -verify
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

void bar() {
  union {
    short s;
    char c1c2[2];
  };
  c1c2[0] = 8; 
  c1c2[1] = 4;
  s = c1c2[0] + c1c2[1];
}
