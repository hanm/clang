// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify

// expected-no-diagnostics

void foo [[asap::region("P")]] /*[[asap::param("P")]]*/ [[asap::writes("P")]] ();

void foo /*[[asap::param("P")]]*/ /*[[asap::writes("R")]]*/ () {}
