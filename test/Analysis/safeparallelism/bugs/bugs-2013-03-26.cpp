// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
// expected-no-diagnostics

[[asap::region("R")]];
class C {
public:
  void do_something();
};

void func [[asap::writes("R")]] ( C *c [[asap::arg("Local,R")]] ) {
  c->do_something();
}
