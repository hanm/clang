// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=inference %s -verify
// expected-no-diagnostics

typedef unsigned int size_t;
typedef size_t __ssize_t;

//typedef int;
//__ssize_t __io_read_fn (void *__cookie, char *__buf, size_t __nbytes);
typedef __ssize_t __io_read_fn (void *__cookie, char *__buf, size_t __nbytes);
