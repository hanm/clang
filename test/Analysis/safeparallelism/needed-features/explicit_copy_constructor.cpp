// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify

/// expected-no-diagnostics

class Data {
public:
  int x;
  int y;

  // Explicit code of the would-be implicit copy constructor
  inline Data &operator=(const Data &D) noexcept {
    this->x = D.x;
    this->y = D.y;
    return *this;
  }

  inline Data(const Data &D) noexcept : x(D.x), y(D.y) {}
};

void copy(Data in, Data out) {
  Data tmp(in);
  Data *tmp2 = new Data(in);
  out = in;
}
