// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//
class [[asap::param("Class")]] C;

template<typename T>
bool isNull [[asap::param("Q")]] (T *p [[asap::arg("Local, Q")]]) {
  return (p==0) ? true : false;
}

class [[asap::param("Class")]] C {
  int x [[asap::arg("Class")]];
public:
   virtual void do_something [[asap::writes("Class")]] ();
};

bool isNull [[asap::param("Q")]] (C *p[[asap::arg("Q")]]) {
  return (p==0) ? true : false;
}

[[asap::region("R")]];
void func0 [[asap::writes("R")]] ( C *c [[asap::arg("Local,R")]] ) {
  if (!isNull(c))
    c->do_something(); // The declaration of do_something that's in scope here is the canonical one, not the definition below.
}

void C::do_something [[asap::writes("Class")]]  () {
  x = 0;
}

void func1 ( C *c [[asap::arg("Local,R")]] ) {
  if (!isNull(c))
    c->do_something(); // expected-warning{{'Writes Effect on R': effect not covered by effect summary}}
    // Note: the checker uses the effect summary of the canonical declaration of do_something
    // to calculate the effects of the call above, so it complains that the reads effect is not covered.
}

