// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=inference %s -verify
// 

void foo() {  // expected-warning{{Inferred Effect Summary for foo: [writes(rpl([r0_A],[]))]}}
  int A [10]; // expected-warning{{Inferred region arguments: int [10], IN:<empty>, ArgV:[r0_A]}}
  A[1] = 2;
}
