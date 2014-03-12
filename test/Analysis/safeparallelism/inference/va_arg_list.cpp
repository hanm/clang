// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=inference %s -verify
//
// expected-no-diagnostics

#include <stdarg.h>

// take an int x and x integers and print them to stdout
void foo(int x, ...) {
  va_list argptr;
  va_start(argptr,x);
  int sum = 0;
  for (int i = 0; i < x; x++) {
    int v = va_arg(argptr, int);
    sum += v;
  }
  va_end(argptr);
}
