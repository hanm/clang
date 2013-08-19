// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//
// expected-no-diagnostics


// Casting on the LHS of an assignment
void *func(void *_Ptr, unsigned int _Marker)
    {
    if (_Ptr)
        {
        *((unsigned int*)_Ptr) = _Marker;
        }
    return _Ptr;
    }
