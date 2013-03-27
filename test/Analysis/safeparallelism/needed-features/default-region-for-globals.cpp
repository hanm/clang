// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//
int global = 0;

class [[asap::param("class")]] C  {
  double x [[asap::arg("class")]];
public:
  static int count;
 
  void func () const
    {
    //Expected warning because of these write
    ++count;
    global = 100.0; 
    }
}; 

int C::count = 0;
