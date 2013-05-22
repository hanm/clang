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

}; 

int C::count = 0;


void funk [[asap::writes("Global")]] () 
  {
    static int x = 0;
    ++x; 
    int &c [[asap::arg("Global")]] = C::getCount();
    c++; 
    global = 100.0; 
  }

