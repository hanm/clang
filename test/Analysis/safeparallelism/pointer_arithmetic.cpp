// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//
// expected-no-diagnostics

typedef int ptrdiff_t;

template<typename T>
inline ptrdiff_t __distance(T *_first, T *_last) {
  return _last - _first;
}

void foo() {
  int x;
  char c = 'a';

  char *a = 0;

  char *b = &c;

  x = __distance(a, b);

}
