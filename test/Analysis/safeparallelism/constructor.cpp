// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//
// expected-no-diagnostics

[[asap::region("Rb, Rc")]];

class [[asap::param("Pa")]] A {
  int i[[asap::arg("Pa")]];
  double x [[asap::arg("Pa")]];
public:
  A(int i, double x) : i(i), x(x) {}
};

class [[asap::param("Pb"), asap::base_arg("A", "Pb:Rb")]] B : public A {
  double y [[asap::arg("Pb")]];
public:
  B(int i, double x, double y) : A(i,x), y(y) {}
};

