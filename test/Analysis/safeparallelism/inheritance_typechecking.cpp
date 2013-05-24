// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
// expected-no-diagnostics
//

class
[[asap::param("P")]] Point {
  double x [[asap::arg("P")]];
  double y [[asap::arg("P")]];
};

[[asap::region("Point")]];

class [[asap::param("PB")]] B {
protected:
  Point *p [[asap::arg("PB, PB:Point")]];  

public:
  void setP [[asap::writes("PB")]] (Point *_p [[asap::arg("PB:Point")]]) 
  { p = _p; }

  Point *getP [[asap::arg("PB:Point"), asap::reads("PB")]] () { return p; }

};


class [[asap::param("PD"), asap::base_arg("B", "PD")]] D : public B {

public:
  void setP [[asap::writes("PD")]] (Point *_p[[asap::arg("PD:Point")]]) 
  { B::setP(_p); }

  void setP_v2 [[asap::writes("PD")]] (Point *_p[[asap::arg("PD:Point")]]) 
  { p = _p; }

  Point *getP [[asap::arg("PD:Point"), asap::reads("PD")]] () { return p; }
  
  Point *getP_v2 [[asap::arg("PD:Point"), asap::reads("PD")]] () { 
    return B::getP(); 
  }

};
