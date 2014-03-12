// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=inference %s -verify
//

struct [[asap::param("P")]] Point {
  double x[[asap::arg("P")]], y[[asap::arg("P")]];
};

Point *Origin1 = new Point(); // the pointer resides and points to Global
Point *Origin2 [[asap::arg("Local")]] = new Point(); // the pointer resides in Global and points to Local
Point *Origin3 [[asap::arg("Local, Global")]] = new Point();

void foo (bool local) {
  if (!local) {
    double tmp = Origin2->x;
    Origin2->x = Origin1->y; 
    Origin1->y = tmp; // expected-warning{{effect not covered}}
    Origin3->x = Origin3->y; // expected-warning{{effect not covered}}
  }
}
