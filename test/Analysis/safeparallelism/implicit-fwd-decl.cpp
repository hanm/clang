// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify

// The template specialization with Func0=Func1=write_functor has an implicit
// fwd declaration of that class without region parameters
// The semantic checker must detect that and parse the class write_functor
// before continuing. Hopefully this will not create cyclic dependencies &
// infinite looping...

namespace tbb {
    template<typename Func0, typename Func1>
    void parallel_invoke [[asap::param("P, Q")]] (const Func0& f0 [[asap::arg("P")]], const Func1& f1 [[asap::arg("Q")]]);
    } // end namespace tbb



class [[asap::param("P"), asap::region("R")]] point  {
    double x [[asap::arg("P")]];
    double y [[asap::arg("P")]];
public:
    point() {}
    point(double x_, double y_) : x(x_), y(y_) {} 
    // implicitly: [[asap::reads("Local")]] point(double x_ [[asap:arg("Local")]], double y_ [[asap::arg("Local")]]) : x(x_), y(y_) {}

    // some functions added for testing checker features & corner cases
    double *getXPtr [[asap::arg("Local,P")]] () { return &x; }
    double getX [[asap::reads("P")]] () { return x; }
    double &getXRef [[asap::arg("P")]] () { return x; }
    void setX1(double x_) { *getXPtr() = x_; } // expected-warning{{effect not covered by effect summary}}
    void setX2[[asap::reads("P")]](double x_) { *getXPtr() = x_; } // expected-warning{{effect not covered by effect summary}}
    void setX [[asap::writes("P")]](double x_) { *getXPtr() = x_; }
    }; // end class point

class [[asap::param("P"), asap::region("R")]]  write_functor {

public:
    point *pt [[asap::arg("P, P")]]; 

    write_functor () : pt(nullptr) {}

    write_functor (
        point *pt_ [[asap::arg("P")]] // implicitly: [[asap::arg("Local, P")]]
        )
       : pt(pt_) {}


    void operator()()
    {
      //*pt = point(0.0, 0.0); // Calls implicit function (copy constructor) which is unsupported
    }
};



int main()
    {
    int x;
    point p1 [[asap::arg("Local")]];

    write_functor wf1[[asap::arg("Local")]](&p1);

    write_functor wf2; // implicitly wf2 [[asap::arg("Local")]]
    wf2.pt = &p1;

    tbb::parallel_invoke(wf1, wf2);

    return 0;
    }
