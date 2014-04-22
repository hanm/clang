// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
// XFAIL: *

/// expected-no-diagnostics

class Data {
public:
  int x;
  int y;

// Implicit copy assignment
//inline Data &operator=(const Data &D) noexcept {
//  this->x = D.x;
//  this->y = D.y;
//  return *this;
//}

// Implcitit copy constructor 
//inline Data(const Data &D) noexcept : x(D.x), y(D.y) {}
//
};

Data id (Data x) { return x; }

Data copy(Data in, Data out) {
  Data tmp(in);              // implicit copy constructor
  Data *tmp2 = new Data(in); // implicit copy constructor
  out = in;                  // implicit copy assignment
  out = id(in);              // implicit move assignment
  return tmp;                // implicit move constructor
}
