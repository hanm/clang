// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//
//

int global = 0;

class [[asap::param("class")]] C  {
  double x [[asap::arg("class")]];
  static int count;
public:
 
  void func () const
    {
    static int x = 0;
    ++x; // expected-warning{{effect not covered}}
    ++count; // expected-warning{{effect not covered}}
    global = 100.0;  // expected-warning{{effect not covered}}
    }
  
  static int &getCount [[asap::arg("Global")]] () { return count; }
  static int *getCountPtr [[asap::arg("Global")]] () { return &count; }

}; 

int C::count = 0;


void funk () 
  {
    static int x = 0; // not expecting a warning here (initialization)

    ++x; // expected-warning{{effect not covered}}

    int &cR [[asap::arg("Global")]] = C::getCount(); // expected-warning{{effect not covered}}

    cR++; // expected-warning{{effect not covered}}
    cR += 1; // expected-warning{{effect not covered}}
    cR = cR + 1; // expected-warning{{effect not covered}} expected-warning{{effect not covered}}

    int *cp [[asap::arg("Global")]] = C::getCountPtr();

    (*cp)++; // expected-warning{{effect not covered}}
    global = 100.0; // expected-warning{{effect not covered}}
    ++x; // expected-warning{{effect not covered}}
  }

