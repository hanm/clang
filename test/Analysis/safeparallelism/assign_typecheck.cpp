// RUN: %clang_cc1 -DASAP_CXX11_SYNTAX -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
// // RUN: %clang_cc1 -DASAP_GNU_SYNTAX -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify

#ifdef ASAP_CXX11_SYNTAX
class
[[asap::param("Pc"),   asap::region("Links"), asap::region("Data"), asap::region("FData") ]]
C {
private:
  /// Fields
  float *p [[asap::arg("Links"), asap::arg("Pc:Data")]];
  float fdata [[asap::arg("Pc:FData")]];

public:
  /// Methods
  float *getP [[asap::arg("Pc:Data"), asap::reads("Links")]]  () { return p; }
  float *getPWrong1 [[asap::arg("Pc:FData"), asap::reads("Links")]]  () { return p; } // expected-warning{{invalid return type}}
  float *getPWrong2 [[asap::arg("Pc:Data"), asap::no_effect]]  () { return &fdata; } // expected-warning{{invalid return type}}
  float *getFP [[asap::arg("Pc:FData"), asap::no_effect]] ()  { return &fdata; }

  void setPointer [[asap::writes("Links")]] () {
    p = &fdata; // expected-warning{{invalid assignment}}
    p = false ? &fdata : getP();  // expected-warning{{invalid assignment}}
  }
  void assignments () {
    float local_1 = 3,
          *local_p0 [[asap::arg("Local")]] = p,  // expected-warning{{invalid initialization}}
          local_2,
          *local_p1[[asap::arg("Local")]] = &local_1;
    float *local_p [[asap::arg("Local")]];
    local_p = p; // expected-warning{{invalid assignment}}
    
  }
}; // end class C
#endif

#ifdef ASAP_GNU_SYNTAX
class
__attribute__((param(("Pc")), region("Links"), region("Data"), region("FData") ))
C {
private:
  /// Fields
  float *p __attribute__((arg("Links"), arg("Pc:Data")));
  float fdata __attribute__((arg("Pc:FData")));

public:
  /// Methods
  void setPointer () __attribute__((writes("Links")))  {
    p = &fdata; // expected-warning{{invalid assignment}}
  }
}; // end class C
#endif
