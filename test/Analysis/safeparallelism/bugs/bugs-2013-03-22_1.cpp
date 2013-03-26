// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify

void func0 [[asap::writes("A::B")]] ();  // expected-warning{{Name specifier was not found}}

