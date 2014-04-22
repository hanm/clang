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
    point &p [[asap::arg("P")]];
public:
    // Implicitly
    // inline point &operator=(point &&) noexcept { 
    //   this->x = static_cast<point &&>().x;
    //   this->y = static_cast<point &&>().y;
    //   return *this; // expected warning: return type implicitly point&<Local> but returns point&<P>
    //                 // we must infer that the return type is point&<P>
    // }
    // infer signature:
    // <region Q> inline point<P> &operator=(point &&<Q>) noexcept reads Q, writes P;

    write_functor(point &p_) : p(p_) {} // expected-warning{{invalid initialization}}

    void operator()()
        {
        p = point(0.0, 0.0); // expected warning because 
        }
    };


int main()
    {
    point p;

    return 0;
    }
