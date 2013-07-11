// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//

// expected-no-diagnostics
int arr[2];
const int arr2[3] = { 0 };
