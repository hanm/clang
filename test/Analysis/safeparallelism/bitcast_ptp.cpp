// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//
// expected-no-diagnostics

class C {
public:
  int x;
};

bool arePtrsEq(void *p1, void *p2) { return p1==p2; }

void foo() {
  char *cP = 0;
  int *iP = 0;
  int i = 3;
  C o;
  C *oP = 0;
  bool eqPtrs = arePtrsEq(&o, oP);

  bool eqPtrs0 = arePtrsEq(&iP, iP);
  bool eqPtrs1 = arePtrsEq(&i, iP);
  bool eqPtrs2 = arePtrsEq(cP, iP);
}
