// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=simple  %s -verify
// XFAIL:*

// The arg on the formal param of the canonical declaration should get copied onto that of the definition
class [[asap::region("R1, R2"), asap::param("C")]] A {
  int x1 [[asap::arg("C:R1")]];
  int x2 [[asap::arg("C:R2")]];
public:
  void copy [[asap::param("P"), asap::reads("P:*"), asap::writes("C:*")]] (A &a [[asap::arg("P")]]);
};

void A::copy(A &a) {
  x1 = a.x1; // param P is not copied onto a so checker assumes it is in Global!
  x2 = a.x2;
}

