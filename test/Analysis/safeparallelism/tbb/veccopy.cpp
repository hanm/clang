// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=param %s -verify


#include "parallel_for_fake.h"
#include "blocked_range.h"

#define GRAIN_SIZE 10

[[asap::region("A,B")]];
int VectorA [[asap::arg("A")]] [100];
int VectorB [[asap::arg("B")]] [100];


class VecCopyBody {
public:
  void operator () [[asap::reads("A"), asap::writes("B")]] 
                   (const tbb::blocked_range<int> &RangeParam) const {
    for (int I=RangeParam.begin(); I<RangeParam.end(); I++) {
      VectorB[I] = VectorA[I];
    }
  }
}; // end class vecCopyBody

class [[asap::param("P")]] VecCopyBodyArg {
int Unused;

public:
  // Constructors
  VecCopyBodyArg(int i) : Unused(i) {}
  
  void operator () [[asap::reads("A"), asap::writes("B")]] 
                   (const tbb::blocked_range<int> &RangeParam) const {
    for (int I=RangeParam.begin(); I<RangeParam.end(); I++) {
      VectorB[I] = VectorA[I];
    }
  }
}; // end class vecCopyBody


class VecCopyFunctor {
public:
  void operator () [[asap::reads("A"), asap::writes("B")]] 
                   (int I) {
    VectorB[I] = VectorA[I];
  }
}; // end class vecCopyFunctor


int main [[asap::writes("A,B")]]
         (void) {
  VecCopyBody Body;
  int i = 0;
  tbb::blocked_range<int> Range(0,100,GRAIN_SIZE);
  
  tbb::parallel_for( Range, Body ); // expected-warning{{interferes with}}

  tbb::parallel_for( Range, VecCopyBody() ); // expected-warning{{interferes with}}

  tbb::parallel_for( Range, VecCopyBodyArg(i) ); // expected-warning{{interferes with}}

  tbb::parallel_for( tbb::blocked_range<int> (0,100,GRAIN_SIZE), VecCopyBody() ); // expected-warning{{interferes with}} expected-warning{{region argument required but not yet supported in this syntax}}
 
  tbb::parallel_for( 0,100, VecCopyFunctor() ); // expected-warning{{interferes with}}
  
  tbb::parallel_for( 0,100,1, VecCopyFunctor() ); // expected-warning{{interferes with}}
}
