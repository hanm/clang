// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//
// expected-no-diagnostics


struct A {
  int fieldA;
  double *ptrA;
};

struct B {
  int fieldB;
  double *ptrB;
};

struct B2 : B {
  char cB2;
};

B *foo [[asap::writes("Global")]]
       (A *a, B2 *b2) {
  B* tmp0 = b2;
  B* tmp1 = (B*)a;
  //tmp1->fieldB = static_cast<B*>(a)->fieldB;
  tmp1->fieldB = ((B*)a)->fieldB;
  return (B*)a;
}


