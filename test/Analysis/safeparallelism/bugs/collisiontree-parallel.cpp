// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=parametric %s -verify
// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=inference %s -verify
// expected-no-diagnostics

#if 0
#  include <tbb/tbb.h>
#else
  namespace tbb {
    template<typename Func0, typename Func1>
    void parallel_invoke//[[asap::param("P1,P2")]]
                        (const Func0& f0 /*[[asap::arg("P1")]]*/, 
                         const Func1& f1 /*[[asap::arg("P2")]]*/) {
        //f0();
        //f1();
    }
  } // end namespace
#endif

class [[asap::param("R")]] IntersectInvoker {

public:
	void operator() [[asap::writes("R")]] () const {}
	
	
};

void foo [[asap::region("A,B")]] [[asap::writes("A,B")]] ()
{
	IntersectInvoker leftFn [[asap::arg("A")]];
	IntersectInvoker rightFn [[asap::arg("B")]];
	tbb::parallel_invoke(leftFn, rightFn);
}
            
