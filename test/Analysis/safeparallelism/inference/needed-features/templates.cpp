// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=inference %s -verify
//
// expected-no-diagnostics

template<typename T>
inline T AddGeneric (T value1, T value2 );

template<typename T>
inline T AddGeneric (T value1, T value2 ) { return value1 + value2; }

/*template<>
inline unsigned char AddGeneric <unsigned char> (unsigned char value1, unsigned  char value2 ) {
      return value1 + value2;
}*/

typedef unsigned char uint8_t;

template<>
inline uint8_t AddGeneric <uint8_t> (uint8_t value1, uint8_t value2 );

template<>
inline uint8_t AddGeneric <uint8_t> (uint8_t value1, uint8_t value2 ) {
      return value1 + value2;
}

