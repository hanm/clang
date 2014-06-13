// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=simple  %s -verify
// expected-no-diagnostics

class [[asap::param("A")]] A {

};

class [[asap::param("P")]] B {
public:
  void operator() [[asap::param("PP"), asap::writes("P,PP")]] 
                  (A &a[[asap::arg("PP")]]) {}
  B() {}
  [[asap::param("PP"), asap::writes("P,PP")]] B(A &a[[asap::arg("PP")]]) {}
};

class [[asap::param("P1,P2")]] C {
public:
  void operator() [[asap::writes("P1,P2")]] (A &a[[asap::arg("P2")]]) {}
  C() {}
  [[asap::writes("P1,P2")]] C(A &a[[asap::arg("P2")]]) {}
};

[[asap::region("R1,R2,R,RR")]];

void foo [[asap::writes("R1,R2,R,RR")]] () {
//void foo () {
  A a1 [[asap::arg("R1")]];
  A a  [[asap::arg("R")]];
  C c [[asap::arg("R2,R1")]];
  c(a1); 
  B b [[asap::arg("RR")]];
  b(a);
}

void bar [[asap::writes("R1,R2,R,RR")]] () {
//void bar () {
  A a1 [[asap::arg("R1")]];
  A a  [[asap::arg("R")]];
  B b [[asap::arg("RR")]] (a);
  C c [[asap::arg("R2,R1")]] (a1);
}
