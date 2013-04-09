// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//

void set_to_zero [[asap::region("R"), asap::reads("R")]] (
    int n, double *d [[asap::arg("R")]] ) {

  for(int i = 0; i < n; ++i) {
    // Works – But I expected a warning because it is writing in R
    d[i] = 0; // expected-warning{{effect not covered}}
  }
}
