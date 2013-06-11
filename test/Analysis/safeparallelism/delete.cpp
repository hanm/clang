// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//

[[asap::region("R1, R2")]];
class scoped_delete {
public:
  double *ptr [[asap::arg("R1, R2")]];

  void func [[asap::reads("R1"), asap::writes("R2")]] () {
    delete ptr; // reads R1, freeing a region does not modify it 
                // because we are assuming that code is memory safe
                // thus it will not be accessed after it's freed.
  }
  void funcErr () {
    delete ptr; // reads R1 expected-warning{{effect not covered}} 
  }
};

