// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=inference %s -verify
//
//

void foo() { //expected-warning{{Solution for foo: [Reads Effect on Global]}}
  int G[[asap::arg("Global")]] = 5 ;
  int &gRef[[asap::arg("Global")]] = G;
}
