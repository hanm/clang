// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=inference %s -verify

class [[asap::param("P")]] C {
  int v [[asap::arg("P")]];
public:
  C &foo [[asap::param("P2")]] (C &x [[asap::arg("P2")]]); // expected-warning{{Inferred region arguments: class C &(class C &), IN:<empty>, ArgV:[p0_P]: Inferred region arguments}}
}; // end class C1

C &C::foo /*param(P2) */ (C &x [[asap::arg("P2")]]) { // expected-warning{{Inferred region arguments: class C &(class C &), IN:<empty>, ArgV:[p0_P]: Inferred region arguments}} // expected-warning{{Inferred Effect Summary for foo: [reads(rpl([p1_P2],[])),writes(rpl([p0_P],[]))]:}}
  v = v * x.v;
  return *this;
}
