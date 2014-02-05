// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=global %s -verify
//
// expected-no-diagnostics

void foo() {
  int G[[asap::arg("Global")]] = 5 ;
  int &gRef[[asap::arg("Global")]] = G;
}
