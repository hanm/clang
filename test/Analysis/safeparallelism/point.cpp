// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify

// expected-no-diagnostics

namespace tbb {
    template<typename Func0, typename Func1>
    void parallel_invoke(const Func0& f0, const Func1& f1);
    } // end namespace tbb


class [[asap::param("P"), asap::region("R")]] point  {
    double x [[asap::arg("P")]];
    double y [[asap::arg("P")]];
public:
    point() {}
    point(double x_, double y_) : x(x_), y(y_) {} 
    // implicitly: [[asap::reads("Local")]] point(double x_ [[asap:arg("Local")]], double y_ [[asap::arg("Local")]]) ...
    };

class [[asap::param("P"), asap::region("R")]]  write_functor {

public:
    point *pt [[asap::arg("P, P")]]; 

    write_functor () : pt(nullptr) {}

    write_functor (
        point *pt_ [[asap::arg("P")]] // implicitly: [[asap::arg("Local, P")]]
        )
       : pt(pt_) {}


    void operator()() // body is commented out to avoid crashes.
    {
      //*pt = point(0.0, 0.0);
    }
};



int main()
    {
    int x;
    point p1 [[asap::arg("Local")]];

    //warning: The RHS type [point *, IN:<empty>, ArgV:Local] is not a subtype of the LHS type [write_functor, IN:<empty>, ArgV:Local] invalid assignment: wf1.pt = &p1]
    write_functor wf1[[asap::arg("Local")]](&p1);

    write_functor wf2; // implicitly wf2 [[asap::arg("Local")]]
    //warning: The RHS type [point *, IN:<empty>, ArgV:Local] is not a subtype of the LHS type [write_functor, IN:<empty>, ArgV:Local] invalid assignment: wf1.pt = &p1]
    wf2.pt = &p1;

    tbb::parallel_invoke(wf1, wf2);

    return 0;
    }
