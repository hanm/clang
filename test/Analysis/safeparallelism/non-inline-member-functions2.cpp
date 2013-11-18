// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//

class [[asap::param("Class")]] C {
  int x [[asap::arg("Class")]];
public:
   virtual void do_something ();
};

void C::do_something [[asap::writes("Class")]]  () { // expected-warning{{effect summary of canonical declaration does not cover the summary of this declaration}}
  x = 0;
}

[[asap::region("R")]];
void func ( C *c [[asap::arg("Local,R")]] ) {
  c->do_something(); // Note: the checker uses the canonical decl of do_something
                     // which has no declared effects. Therefore, the call here
                     // is assumed to have no effects (other than the default
                     // writes Local:*
}

