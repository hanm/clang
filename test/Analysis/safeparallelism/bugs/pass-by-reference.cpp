// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=param %s -verify
// expected-no-diagnostics

void process [[asap::param("P"), asap::writes("P")]]
             (double &d [[asap::arg("P")]]) {
  d = 1;
}

int main [[asap::writes("*")]] (/*int argc, char **argv ARG(Global, Global)*/) {
  double d;
  process(d);
  return 0;
}
