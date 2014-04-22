// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
// XFAIL: *

class [[asap::param("P"), asap::region("R1, R2")]] C1 {
  int   x [[asap::arg("R1")]],
       *p [[asap::arg("P:R1, R1")]],
      **pp[[asap::arg("P, *, *")]];
  
  void capture [[asap::writes("*")]] (int **ptr[[asap::arg("Local, *, *")]]) {
    *ptr = &this->x;   // expected-warning{{cannot modify aliasing through pointer to partly specified region}}
  }
}; // end class C1<P>


/// Taken from Rob Bocchino's PhD pg 67
class [[asap::param("P"), asap::region("R")]] C {
  C *f [[asap::arg("Root, P")]];

  C *weaken [[asap::arg("*")]] (C *x[[asap::arg("P")]]) { return x; }

  void assign [[asap::writes("Root")]] (C *x[[asap::arg("P")]]) {
    f = x;
  }

  void unsound [[asap::writes("Root")]] () {
    C *x [[asap::arg("Root")]] = new C();
    C *x1 [[asap::arg("Root:*")]] = weaken(x);
    C *x2 [[asap::arg("Root:R")]] = new C();
    x1->assign(x2);
  }
  
  
}; // end class C<P>
