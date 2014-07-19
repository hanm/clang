// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=effect-inference %s -verify
//
//

void foo() { //expected-warning{{Inferred Effect Summary for foo: [reads(rpl([rGLOBAL],[]))]}}
  int G[[asap::arg("Global")]] = 5 ;
  int &gRef[[asap::arg("Global")]] = G;
}
