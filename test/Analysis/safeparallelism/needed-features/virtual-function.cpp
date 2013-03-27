// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//
//
class [[asap::param("Class")]] base {
  int x [[asap::arg("Class")]];
public:
  virtual void do_something  [[asap::reads("Class")]] () = 0;
};

class [[asap::param("Class")]] derived : public base {
  // No warning – should there be one?
  void do_something [[asap::writes("Class")]] () {} // expected-warning{{}}
};
