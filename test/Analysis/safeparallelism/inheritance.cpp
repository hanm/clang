// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//

// expected-no-diagnostics

// Derived class object calls base class method, which needs inheritance induced
// substitution to compute the effects of the call.
class [[asap::param("ClassB")]] base {
  int x [[asap::arg("ClassB")]];
public:
  void do_something [[asap::reads("ClassB")]] () {};
};

class [[asap::param("Class"),
       asap::base_arg("base", "Class")]] derived : public base {

};

[[asap::region("R")]];
void func [[asap::writes("R")]] (derived *d [[asap::arg("Local, R")]]) {
  d->do_something();
}
