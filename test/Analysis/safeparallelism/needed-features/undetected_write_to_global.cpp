// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=global %s -verify
//

template<typename T>
class AddOperator {
public:
  inline T AddGeneric (T value1, T value2 ) { 
    return value1 + 2*value2; 
  }

};


class CustomType {
  int value1;
  int value2;
public:
  CustomType(int v1, int v2) : value1(v1), value2(v2) {}
  [[asap::reads("Global")]] CustomType(CustomType &C) : value1(C.value1), value2(C.value2) {}
  [[asap::reads("Global")]] CustomType(CustomType &&C) : value1(C.value1), value2(C.value2) {}
  inline CustomType &operator= [[asap::writes("Global")]] (CustomType &&C) noexcept {
    this->value1 = C.value1;
    this->value2 = C.value2;
    return *this;
  }


  int getValue1() { return value1; }  
  int getValue2() { return value2; }  
  void setValue1 [[asap::writes("Global")]] (int v) { value1 = v; }
  void setValue2 [[asap::writes("Global")]] (int v) { value2 = v; }
  // this class is not overloading the + operator
  // but it has Add
  CustomType add (CustomType &c2) {
    return CustomType(value1 + c2.getValue1(), value2 + c2.getValue2());
  }
};

template<>
class AddOperator<CustomType> {
public:
  inline CustomType AddGeneric(CustomType value1, CustomType value2) {
    return value1.add(value2);
  }
};

// GlobalCustomType is in Global but its fields are in Local because
// of the default regions
CustomType GlobalCustomType(7,11);

void foo[[asap::reads("Global")]] () {
  CustomType A(1,2), B(3,5);
  AddOperator<CustomType> Op;
  CustomType C = Op.AddGeneric(A, B);
  CustomType D = Op.AddGeneric(C, GlobalCustomType);
  GlobalCustomType = Op.AddGeneric(C, D); // expected-warning{{effect not covered}} // writing to Global
  GlobalCustomType.setValue1(0); // expected-warning{{effect not covered}}
  GlobalCustomType.setValue2(0); // expected-warning{{effect not covered}}
}

