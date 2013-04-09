// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//

[[asap::region("R")]];

class [[asap::param("P")]] C;
//class C; // fwd declaration without attributes

class A {
  int x [[asap::arg("R")]];
};

class [[asap::param("BP")]] B {
  C* p [[asap::arg("BP, BP")]];
  C* p1 [[asap::arg("BP")]]; // expected-warning{{missing region argument}}
  A* p2 [[asap::arg("BP")]];
};

class [[asap::param("Class")]] C {
  int x [[asap::arg("Class")]];
public:
   virtual void do_something [[asap::writes("Class")]]();
   //virtual void do_something_else [[asap::writes("P")]]();
};

void func [[asap::writes("R")]] ( C *c [[asap::arg("Local,R")]] ) {
  c->do_something(); 
}

void C::do_something () {
  x = 0;
}

