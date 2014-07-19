// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=effect-inference  %s -verify
// expected-no-diagnostics

// Case 1: VarEffSum - VarEffSum
void foo1();
void foo1() {}

// Case 2: VarEffSum - ConcreteEffSum
void foo2();
void foo2 [[asap::no_effect]] () {}

// Case 3: ConcreteEffSum - VarEffSum
void foo3 [[asap::no_effect]] ();
void foo3 () {}
