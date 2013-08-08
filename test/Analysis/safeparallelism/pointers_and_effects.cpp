// RUN: %clang_cc1 -DASAP_CXX11_SYNTAX -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
// RUN: %clang_cc1 -DASAP_GNU_SYNTAX -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//


#ifdef ASAP_CXX11_SYNTAX
namespace Shapes /* FIXME: is this not in the C++11 standard ? [[asap::region("Pool")]]*/ 
{
#if 0
class A {};
class B: public A {

  A *pA;
  B *pB;

  B(B *p) {
    pB = p;        // OK
    pA = p;        // OK
    pA = pB = p;   // OK
    //pB = pA = p; // error. Type of assignment is type of LHS
  }
};
#endif

class
[[asap::region("X, Y")]]
[[asap::param("Pp")]]
Point {
public:
  int x [[asap::arg("Pp:X")]];
  int y [[asap::arg("Pp:Y")]];
}; // end Point

class
//[[asap::param("P, P<=Pool")]]
[[asap::param("Pr")]]
[[asap::region("R1, R2, Next, Links, Pool")]]
Rectangle {

  // Fields
  Point p1 [[asap::arg("Pr:R1")]];
  Point p2 [[asap::arg("Pr:R2")]];

  Point 
    * pp [[asap::arg("Pr:Links, Pool:*")]],
    **ppp [[asap::arg("Links, Pr:Links, Pool:*")]];

  Point
    * ppstar [[asap::arg("Pr:Links, Pr:*")]],
    * ppstar1 [[asap::arg("Pr:Links, Pr:*:R1")]],
    **pppstar [[asap::arg("Links, Pr:*:Links, Pr:*")]];

  Rectangle
    * loop [[asap::arg("Pr:Links, *")]],
    * next [[asap::arg("Pr:Links, Pr:Next")]],
    ** pnext [[asap::arg("Links, Pr:Links, Pr:Next")]];

  Rectangle ** pnextstar [[asap::arg("Links, Pr:Links, Pr:*:Next")]];

  void do_pointer_stuff
    [[asap::writes("Pr:R1:Point::X, Pr:R1:Point::Y, Pr:R2:*, Pr:*:Links, Links")]]
    [[asap::reads("Pr:Next")]]
    (int _x, int _y, bool b)
  {
    ppstar = ppstar1 = &p1;
    ppstar1 = ppstar = &p1; // expected-warning{{invalid assignment:}}

    //(b ? ppstar : ppstar1) = &p1;

    p1.x = _x;          // writes Pr:R1:X
    p1.y = _y;          // writes Pr:R1:Y
    p2.x = _x + 5;      // writes Pr:R2:X
    p2.y = _y + 5;      // writes Pr:R2:Y
    p1.x = next->p1.x;
    pp = &*&p1;         // writes Pr:Links   expected-warning{{invalid assignment:}}
    ppstar = &*&p1;     // writes Pr:Links
    ppp = &(*&pp);      // writes Links
    *ppp = pp = &p2;         // reads Links, writes Pr:Links   expected-warning{{invalid assignment:}}
    *ppp = ppstar = &p2;         // reads Links, writes Pr:Links   expected-warning{{invalid assignment:}}
    ppstar = &p2;         // reads Links, writes Pr:Links
    ppstar1 = &p1;
    ppstar1 = &p2;      // expected-warning{{invalid assignment:}}
    pppstar = &ppstar1;
  
    *ppp = *&pp;        // reads Links, writes Pr:Links + reads Pr:Links
    *ppp = next->pp;    // reads Links, writes Pr:Links + reads Pr:Links, Pr:Next:Links
    *ppp = *&(next->pp);    // reads Links, writes Pr:Links + reads Pr:Links, Pr:Next:Links
    ppp = &next->pp;        // writes Links + reads Pr:Links   expected-warning{{invalid assignment:}}
    pppstar = &next->ppstar;        // writes Links + reads Pr:Links
    *ppp = *(*&next->ppp);  // reads Links, writes Pr:Links + reads Links, Pr:Links, Pr:Next:Links
    *ppp = &*pp;            // reads Links, writes Pr:Links + reads Pr:Links
    next = this;        // writes Pr:Links (the 'this' pointer is immutable, so   expected-warning{{invalid assignment:}}
                        // write effects are not possible and read effects on it 
                        // can be discarded
    loop = this;
    loop = next;

    pnext = &next;              // writes Links 
    *pnext = next->next;        // reads Links, writes Pr:Links + reads Pr:Links, Pr:Next:Links   expected-warning{{invalid assignment:}}
    *pnext = *next->pnext;      // reads Links, writes Pr:Links + reads Pr:Links, Links, Pr:Next:Links  expected-warning{{invalid assignment:}}
    *pnext = *(*pnext)->pnext;  // reads Links, writes Pr:Links + reads Links, Pr:Links, Pr:Next:Links  expected-warning{{invalid assignment:}}
    *pnextstar = next->next;        // reads Links, writes Pr:Links + reads Pr:Links, Pr:Next:Links
    *pnextstar = *next->pnext;      // reads Links, writes Pr:Links + reads Pr:Links, Links, Pr:Next:Links
    *pnextstar = *(*pnext)->pnext;  // reads Links, writes Pr:Links + reads Links, Pr:Links, Pr:Next:Links
  } 

  // TODO 
  void do_reference_stuff() ;

}; // end class Rectangle

} // end namespace Pool
#endif

#ifdef ASAP_GNU_SYNTAX
namespace Shapes __attribute__((region("Pool"))) {

class
__attribute__((region("X, Y")))
__attribute__((param("Pp")))
Point {
public:
  int __attribute__((arg("Pp:X"))) x;
  int __attribute__((arg("Pp:Y"))) y;
}; // end Point

class
//__attribute__((region_param("P, P<=Pool")))
__attribute__((param("Pr")))
__attribute__((region("R1, R2, Next, Links")))
Rectangle {

  // Fields
  Point p1 __attribute__((arg("Pr:R1")));
  Point p2 __attribute__((arg("Pr:R2")));

  Point
    * pp __attribute__((arg("Pr:Links, Pool:*"))),
    **ppp __attribute__((arg("Links, Pr:Links, Pool:*")));

  Point
    * ppstar __attribute__((arg("Pr:Links, Pr:*"))),
    * ppstar1 __attribute__((arg("Pr:Links, Pr:*:R1"))),
    **pppstar __attribute__((arg("Links, Pr:*:Links, Pr:*")));

  Rectangle
    * loop __attribute__((arg("Pr:Links, *"))),
    * next __attribute__((arg("Pr:Links, Pr:Next"))),
    ** pnext __attribute((arg("Links, Pr:Links, Pr:Next")));

  Rectangle ** pnextstar __attribute((arg("Links, Pr:Links, Pr:*:Next")));

  void do_pointer_stuff(int _x, int _y) 
    __attribute__((writes("Pr:R1:*, Pr:R2:*")))
    __attribute__((writes("Pr:*:Links")))
    __attribute__((writes("Links")))
    //__attribute__((writes("P:R1:Y"))) // Y not declared here TODO: support it
    __attribute__((reads("Pr:Next")))
  {
    p1.x = _x;          // writes Pr:R1:X
    p1.y = _y;          // writes Pr:R1:Y
    p2.x = _x + 5;      // writes Pr:R2:X
    p2.y = _y + 5;      // writes Pr:R2:Y
    p1.x = next->p1.x;
    pp = &*&p1;         // writes Pr:Links   expected-warning{{invalid assignment:}}
    ppstar = &*&p1;     // writes Pr:Links
    ppp = &(*&pp);      // writes Links
    *ppp = pp = &p2;         // reads Links, writes Pr:Links    expected-warning{{invalid assignment:}}
    *ppp = ppstar = &p2;         // reads Links, writes Pr:Links   expected-warning{{invalid assignment:}}
    ppstar = &p2;         // reads Links, writes Pr:Links
    ppstar1 = &p1;
    ppstar1 = &p2;      // expected-warning{{invalid assignment:}}
    pppstar = &ppstar1;

    *ppp = *&pp;        // reads Links, writes Pr:Links + reads Pr:Links
    *ppp = next->pp;    // reads Links, writes Pr:Links + reads Pr:Links, Pr:Next:Links
    *ppp = *&(next->pp);    // reads Links, writes Pr:Links + reads Pr:Links, Pr:Next:Links
    ppp = &next->pp;        // writes Links + reads Pr:Links    expected-warning{{invalid assignment:}}
    pppstar = &next->ppstar;        // writes Links + reads Pr:Links
    *ppp = *(*&next->ppp);  // reads Links, writes Pr:Links + reads Links, Pr:Links, Pr:Next:Links
    *ppp = &*pp;            // reads Links, writes Pr:Links + reads Pr:Links
    next = this;        // writes Pr:Links (the 'this' pointer is immutable, so   expected-warning{{invalid assignment:}}
                        // write effects are not possible and read effects on it
                        // can be discarded
    loop = this;
    loop = next;

    pnext = &next;              // writes Links
    *pnext = next->next;        // reads Links, writes Pr:Links + reads Pr:Links, Pr:Next:Links         expected-warning{{invalid assignment:}}
    *pnext = *next->pnext;      // reads Links, writes Pr:Links + reads Pr:Links, Links, Pr:Next:Links  expected-warning{{invalid assignment:}}
    *pnext = *(*pnext)->pnext;  // reads Links, writes Pr:Links + reads Links, Pr:Links, Pr:Next:Links  expected-warning{{invalid assignment:}}
    *pnextstar = next->next;        // reads Links, writes Pr:Links + reads Pr:Links, Pr:Next:Links
    *pnextstar = *next->pnext;      // reads Links, writes Pr:Links + reads Pr:Links, Links, Pr:Next:Links
    *pnextstar = *(*pnext)->pnext;  // reads Links, writes Pr:Links + reads Links, Pr:Links, Pr:Next:Links
  }

}; // end class Rectangle

} // end namespace Pool
#endif

