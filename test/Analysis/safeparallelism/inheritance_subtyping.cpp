// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify

// expected-no-diagnostics
//
[[asap::region("Rb, Rc")]];

class [[asap::param("Pa")]] A {
  double x [[asap::arg("Pa")]];
};

class [[asap::param("Pb"), asap::base_arg("A", "Pb:Rb")]] B : public A {
  double y [[asap::arg("Pb")]];
};

class [[asap::param("Pc"), asap::base_arg("B", "Pc:Rc")]] C : public B {
  double z [[asap::arg("Pc")]];
};

int main() {
  A *a [[asap::arg("Local, Local")]] = new A();
  B *b [[asap::arg("Local, Local")]] = new B();
  C *c [[asap::arg("Local, Local")]] = new C();
  A *aa [[asap::arg("Local, Local")]] = a;
  A *ab [[asap::arg("Local, Local:Rb")]] = b;
  A *ac [[asap::arg("Local, Local:Rc:Rb")]] = c;
  //C *e;// = b;
  //e = b;
  return 0;
}
