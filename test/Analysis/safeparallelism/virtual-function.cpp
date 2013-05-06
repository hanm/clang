// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//
//
class [[asap::param("Class")]] base {
  int x [[asap::arg("Class")]];
public:
  virtual void do_something  [[asap::reads("Class")]] () = 0; // expected-warning{{overridden virtual function does not cover the effects}} expected-warning{{overridden virtual function does not cover the effects}}
  //virtual void do_something  [[asap::reads("Class")]] () = 0;
};

class [[asap::param("Class"), asap::base_arg("base", "Class")]] derived : public base {
  void do_something [[asap::writes("Class")]] () {} 
}; // end class derived 

class [[asap::param("Class"), asap::base_arg("base", "Class")]] derived1 : public base {
}; // end class derived 

class [[asap::param("Class"), asap::base_arg("derived1", "Class")]] derived2 : public derived1 {
  void do_something [[asap::writes("Class")]] () {} 
}; // end class derived 

