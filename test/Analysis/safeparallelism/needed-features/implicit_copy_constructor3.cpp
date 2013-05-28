// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify

/// expected-no-diagnostics

class Data {
public:
  int x;
  int y;

/*inline Data &operator=(const Data &D) noexcept {
    this->x = D.x;
    this->y = D.y;
    return *this;
  }*/
};

void copy(Data in, Data out) {
  out = in;
}
