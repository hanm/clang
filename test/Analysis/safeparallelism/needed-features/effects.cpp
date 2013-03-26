// RUN: %clang_cc1 -DCLANG_VERIFIER -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify

[[asap::region("BimBam")]];

class [[asap::param("P"), asap::region("Next, Links")]] Point {
public:
  // Fields
  double x[[asap::arg("P")]];
  double y[[asap::arg("P")]];
  Point *next[[asap::arg("P:Links, P:Next")]];
  // Constructor
  Point(double x_, double y_) : x(x_), y(y_) {}
  // Functions
  double getXVal[[asap::reads("P")]] () { return x; }
  double &getXRef[[asap::arg("P")]] () { return x; }
  Point *getNext[[asap::arg("P:Next"), asap::reads("P:Links")]] () {
    return next;
  }
  void setX[[asap::writes("P")]] (double x_) { x = x_; }
  void setNext[[asap::writes("P:Links")]] (Point *next_[[asap::arg("P:Next")]]) {
    next = next_;
  }
};

// Support simple inferable region parameters on functions.
Point *getSelf [[asap::param("Pf"), asap::arg("Pf"), asap::reads("Local")]]
               (Point *arg[[asap::arg("Local, Pf")]]) {
  return arg;
}


#ifdef CLANG_VERIFIER
  #define printf(X, Y)
#else
  #include <stdio.h>
#endif

int main [[asap::reads("BimBam:Point::Next"), asap::writes("BimBam, BimBam:Point::Links")]](void) {

  Point point[[asap::arg("BimBam")]](0.0,0.0);
  Point next[[asap::arg("BimBam:Point::Next")]](1.0, 1.0);
  point.setNext(&next);
  point.setNext(0);
  Point *pnext[[asap::arg("Local, BimBam:Point::Next")]] = point.getNext();
  printf("Point.X = %f\n", point.getXVal()); // expect 0.0
  point.getXRef() = pnext->getXVal();
  printf("Point.X = %f\n", point.getXVal()); // expect 1.0
  point.setX(0.0);
  printf("Point.X = %f\n", point.getXVal()); // expect 0.0
  printf("Point.next.x = %f\n", point.getNext()->getXVal()); // expect 1.0
  point.getXRef() = pnext->getXRef();
  printf("Point.X = %f\n", point.getXVal()); // expect 1.0
  point.setX(0.0);
  printf("Point.X = %f\n", point.getXVal()); // expect 0.0
  printf("Point.next.x = %f\n", point.getNext()->getXVal()); // expect 1.0 
  point.getXRef() = point.getNext()->getXVal();
  printf("Point.X = %f\n", point.getXVal()); // expect 1.0
  getSelf(&point)->getXRef() = point.getXVal();
  getSelf(&point)->getXRef() = point.getNext()->getXVal();
  getSelf(&point)->getXRef() = getSelf(&point)->getNext()->getXVal();
  point.setNext(&point); // expected-warning{{invalid argument}}
  getSelf(&point)->setNext(&point); // expected-warning{{invalid argument}}
  point.setNext(getSelf(&point)); // expected-warning{{invalid argument}}
  getSelf(&point)->setNext(getSelf(&point)); // expected-warning{{invalid argument}}
  point.setNext(&next);
  getSelf(&point)->setNext(&next);
  point.setNext(getSelf(&next));
  getSelf(&point)->setNext(getSelf(&next));
  getSelf(&point)->setNext(getSelf(&point)->getNext());
  getSelf(&point)->setNext(getSelf(&point)->next);
  return 0;
}
