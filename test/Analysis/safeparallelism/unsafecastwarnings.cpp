// RUN: %clang_cc1 -DASAP_CXX11_SYNTAX -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-warn-unsafe-casts=true %s -verify

void foo() {
  short s;
  int x;
  long xl = x;
  long long xll;
  xl = (long) x;
  int *p;
  p = (int *)xl; // expected-warning{{unsafe explicit cast}}
  x = xl;
  x = (int) xl;
  xll = s;
}
