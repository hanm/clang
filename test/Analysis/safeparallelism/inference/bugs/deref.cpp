// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=inference %s -verify
// expected-no-diagnostics


typedef unsigned int size_t;

template<typename T> class C {
public:
  //T *_M_x;
  T _M_x[10];
  size_t _M_p;
  
  T foo() {
    T __z = this->_M_x[this->_M_p++];
    return __z;
  }
};

int main (void) {
  C<unsigned int> c;
  unsigned int x = c.foo();
  return 0;
}
