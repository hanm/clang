// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//
// XFAIL:*

class [[asap::param("Pc")]] C {
  double y [[asap::arg("Pc")]];
public:
  C(double v) { y = v; }
};

