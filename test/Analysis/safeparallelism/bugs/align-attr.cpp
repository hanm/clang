// RUN: %clang_cc1 -std=c++11  -fcxx-exceptions -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=global %s -verify
// expected-no-diagnostics

typedef struct
{
  struct
  {
    int __mask_was_saved;
  } __cancel_jmp_buf[1];
  void *__pad[4];
} __pthread_unwind_buf_t __attribute__ ((aligned));

