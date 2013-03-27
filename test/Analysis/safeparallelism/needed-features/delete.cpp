// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//

[[asap::region("R")]];
class scoped_delete {
public:
  double *ptr [[asap::arg("Local, R")]];

  void func [[asap::reads("R")]] () {
    delete ptr; // expected-warning{{effect not covered}}
  }
};

