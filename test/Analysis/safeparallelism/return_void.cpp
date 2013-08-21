// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//
// expected-no-diagnostics

typedef int int32_t;

static inline void __TBB_machine_pause(int32_t delay) {
    for (int32_t i = 0; i < delay; i++) {
        asm volatile ("pause;");
    }
    return;
}


