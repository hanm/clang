// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
// expected-no-diagnostics

_Atomic(bool) a;
