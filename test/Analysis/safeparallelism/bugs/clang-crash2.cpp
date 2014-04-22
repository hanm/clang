// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=param %s -verify
// XFAIL: *

class [[asap::param("P"), asap::region("R")]] point  {
    double x [[asap::arg("P")]];
    double y [[asap::arg("P")]];
public:
    point() {}
    point(double x_, double y_) : x(x_), y(y_) {}
    };

class [[asap::param("P"), asap::region("R")]]  write_functor{
    point *pt [[asap::arg("P, R")]];
public:

#if 0
    //How do I annoate this?
    write_functor (
        point *pt_
        ) 
       : pt(pt_) {}
#endif

    void operator()()
        {
        *pt = point(0.0, 0.0);
        }
    };


int main()
    {
    point p;

    return 0;
    }
