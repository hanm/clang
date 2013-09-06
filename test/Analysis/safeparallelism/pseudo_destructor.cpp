// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//

[[asap::region("R")]];

template<typename T>
T add_4stride(T &x [[asap::arg("R")]]) {
  T *y [[asap::arg("Local, R")]] = &x;
  T r = x + y[4]; // expected-warning{{effect not covered}} expected-warning{{effect not covered}}
  y[4].~T();      // expected-warning{{effect not covered}}
  return r;
}

void foo [[asap::reads("R")]] () {
  int x[[asap::arg("R")]] [] =  { 0, 1, 2, 3, 4, 5 };
  int r = add_4stride(x[0]);
}
