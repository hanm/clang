// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.GlobalAccessChecker %s -verify
//
// expected-no-diagnostics

template<typename T>
class blocked_range {
  T my_begin;
  T my_end;
 
  bool empty() const {return !(my_begin<my_end);}
  bool nonEmpty() const {return (my_begin<my_end);}
};
