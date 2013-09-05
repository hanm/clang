// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
// 
// expected-no-diagnostics

void foo () {
  char *** p;
  long s1 = sizeof(p); // sizeof with expression argument
  long s2 = sizeof(char ***); // sizeof with type argument
}
