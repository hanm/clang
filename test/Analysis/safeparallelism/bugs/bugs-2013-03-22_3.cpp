// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify

namespace tbb {
    template<typename Func0, typename Func1>
    void parallel_invoke(const Func0& f0, const Func1& f1) { 
        f0();
        f1();
        }
    } // end namespace
 
void do_nothing()
    {
    }
 
 
int func()
    {
    tbb::parallel_invoke(do_nothing, do_nothing); // expected-warning{{Non-interference check not implemented}} expected-warning{{Non-interference check not implemented}}
    return 0;
    }


