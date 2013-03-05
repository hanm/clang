// RUN: %clang_cc1 -DASAP_CXX11_SYNTAX -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
// // RUN: %clang_cc1 -DASAP_GNU_SYNTAX -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify

#ifdef ASAP_CXX11_SYNTAX
class 
[[asap::param("P")]] Point {
  double x [[asap::arg("P")]];
  double y [[asap::arg("P")]];
};

class
[[asap::param("Pc"),   asap::region("Links, Data, FData, Next")]]
C {
private:
  /// Fields
  float *p [[asap::arg("Links, Pc:Data")]];
  float fdata [[asap::arg("Pc:FData")]];
  float data [[asap::arg("Pc:Data")]];
  Point *point [[asap::arg("Links, Pc")]];
  C *next[[asap::arg("Links, Pc:Next")]];

public:
  /// Methods
  float *getP [[asap::arg("Pc:Data"), asap::reads("Links")]]  () { return p; }
  float *getPWrong1 [[asap::arg("Pc:FData"), asap::reads("Links")]]  () { return p; } // expected-warning{{invalid return type}}
  float *getPWrong2 [[asap::arg("Pc:Data"), asap::no_effect]]  () { return &fdata; } // expected-warning{{invalid return type}}
  float *getFP [[asap::arg("Pc:FData"), asap::no_effect]] ()  { return &fdata; }

  float &getDataRef [[asap::arg("Pc:Data")]] () { return data; }
  float &getFDataRef [[asap::arg("Pc:FData")]] () { return fdata; }

  float getData [[asap::reads("Pc:Data")]] () { return data; }
  float getFData [[asap::reads("Pc:FData")]] () { return fdata; }

  void setPointer [[asap::writes("Links")]] () {
    p = &fdata; // expected-warning{{invalid assignment}}
    p = false ? &fdata : getP();  // expected-warning{{invalid assignment}}
    p = false ? getP() : &fdata;  // expected-warning{{invalid assignment}}
    p = false ? getP() : &data;
    p = false ? &data : getP();
  }

  void setPointer [[asap::writes("Links")]] (float *p [[asap::arg("Pc:Data")]]) {
	this->p = p;
  }

  void setPointerBad [[asap::writes("Links")]] (float *p [[asap::arg("Pc:FData")]]) {
	this->p = p; // expected-warning{{invalid assignment}}
  }

  void setPoint [[asap::writes("Links")]] (Point *p[[asap::arg("Pc")]]) {
    point = p;
  }

  void setNext [[asap::writes("Links")]] (C *c[[asap::arg("Pc:Next")]]) {
    next = c;
  }

  void assignments () {
    float local_1 = 3,
          *local_p0 [[asap::arg("Local")]] = p,  // expected-warning{{invalid initialization}}
          local_2,
          *local_p1[[asap::arg("Local")]] = &local_1;
    float *local_p [[asap::arg("Local")]];
    setPointer(this->p);
	float *p[[asap::arg("Pc:Data")]];
	setPointer(p);
	setPointer(getP());
    local_p = p; // expected-warning{{invalid assignment}}
  }

}; // end class C

int main() {
  Point p;
  C c0, c1 [[asap::arg("Local:C::Next")]];
  c0.setNext(&c1);
  // references
  float &ref0[[asap::arg("Local")]] = *c0.getP(); // expected-warning{{invalid initialization}}
  float &ref1[[asap::arg("Local:C::Data")]] = *c0.getP();
  float &ref2[[asap::arg("Local:C::Data")]] = c0.getDataRef();
  float &ref3[[asap::arg("Local:C::Data")]] = c0.getFDataRef(); // expected-warning{{invalid initialization}}
  float &ref4[[asap::arg("Local:C::FData")]] = c0.getFDataRef();
  float &ref5[[asap::arg("Local:C::FData")]] = c1.getFDataRef(); // expected-warning{{invalid initialization}}
  float &ref6[[asap::arg("Local:C::Next:C::FData")]] = c1.getFDataRef();
  float &ref7[[asap::arg("Local:*:C::FData")]] = c1.getFDataRef();
  float &ref8[[asap::arg("Local:*:C::FData")]] = c1.getDataRef(); // expected-warning{{invalid initialization}}
  float &ref9[[asap::arg("Local:*:C::Data")]] = c1.getDataRef();
  float &ref10[[asap::arg("Local:*")]] = c1.getDataRef();
  float &ref11[[asap::arg("Local:*")]] = c0.getDataRef();
  float &ref12[[asap::arg("Local:*:C::Data")]] = c0.getDataRef();
  float &ref13[[asap::arg("Local:*:C::Data")]] = c0.getFDataRef(); // expected-warning{{invalid initialization}}
  float &ref14[[asap::arg("*:C::Data")]] = c0.getDataRef();
  float &ref15[[asap::arg("*:C::FData")]] = c0.getDataRef(); // expected-warning{{invalid initialization}}
  float &ref16[[asap::arg("*")]] = c0.getDataRef();

  return 0;
}
#endif

#ifdef ASAP_GNU_SYNTAX
class
__attribute__((param(("Pc")), region("Links"), region("Data"), region("FData") ))
C {
private:
  /// Fields
  float *p __attribute__((arg("Links, Pc:Data")));
  float fdata __attribute__((arg("Pc:FData")));

public:
  /// Methods
  void setPointer () __attribute__((writes("Links")))  {
    p = &fdata; // expected-warning{{invalid assignment}}
  }
}; // end class C
#endif
