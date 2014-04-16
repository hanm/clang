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
    ++x; 
    ++count; 
    global = 100.0; 
    }
  
  static int &getCount() { return count; } // expected-warning{{invalid return type}}
  
  static int &getCount2 [[asap::arg("Global")]] () { return count; }
}; 

int C::count = 0;

void funk [[asap::writes("Global")]] () 
{
  static int x = 0;
  ++x; 
  int &c=C::getCount2();
  c++;
  global = 100.0;
}
