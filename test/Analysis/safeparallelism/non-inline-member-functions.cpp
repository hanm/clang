// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//
// expected-no-diagnostics

class [[asap::param("Class")]] C {
  int x [[asap::arg("Class")]];
public:
   virtual void do_something [[asap::writes("Class")]]();
};

[[asap::region("R")]];
void func [[asap::writes("R")]] ( C *c [[asap::arg("Local,R")]] ) {
  c->do_something(); 
}

void C::do_something () {
  x = 0;
}

