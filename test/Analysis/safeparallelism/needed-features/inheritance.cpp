// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//

class [[asap::param("Class")]] base {
  int x [[asap::arg("Class")]];
public:
  void do_something [[asap::reads("Class")]] () {};
};

class [[asap::param("Class")]] derived : public base {
};

[[asap::region("R")]];
void func [[asap::writes("R")]] (derived *d [[asap::arg("Local, R")]]) {
  // warning: 'Reads Effect on Class' effect not covered by effect summary
  d->do_something();
}
