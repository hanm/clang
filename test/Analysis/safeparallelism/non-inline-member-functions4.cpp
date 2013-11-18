// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//

class [[asap::param("Class")]] C {
  int x [[asap::arg("Class")]];
public:
   virtual void do_something [[asap::writes("Class")]] ();
};

[[asap::region("R")]];
void func ( C *c [[asap::arg("Local,R")]] ) {
  c->do_something(); // expected-warning{{effect not covered}}
}

void C::do_something [[asap::writes("Class")]]  () {
  x = 0;
}

