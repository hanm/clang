// RUN: %clang_cc1 -DASAP_GNU_SYNTAX -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
// RUN: %clang_cc1 -DASAP_CXX11_SYNTAX -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//


#ifdef ASAP_CXX11_SYNTAX
namespace Shapes /* FIXME: is this not in the C++11 standard ? [[asap::region("Pool")]]*/ 
{

class
[[asap::region("X")]]
[[asap::region("Y")]]
[[asap::param("Pp")]]
Point {
public:
  int x [[asap::arg("Pp:X")]];
  int y [[asap::arg("Pp:Y")]];
}; // end Point

class
//[[asap::region_param("P, P<=Pool")]]
[[asap::param("Pr")]]
[[asap::region("R1"), asap::region("R2"), 
  asap::region("Next"), asap::region("Links"), asap::region("Pool")]]
Rectangle {

// Fields
Point p1 [[asap::arg("Pr:R1")]];
Point p2 [[asap::arg("Pr:R2")]];

Point 
  * pp [[asap::arg("Pr:Links"), asap::arg("Pool:*")]],
  **ppp [[asap::arg("Links"), asap::arg("Pr:Links"), asap::arg("Pool:*")]];

Point
  * ppstar [[asap::arg("Pr:Links"), asap::arg("Pr:*")]],
  * ppstar1 [[asap::arg("Pr:Links"), asap::arg("Pr:*:R1")]],
  **pppstar [[asap::arg("Links"), asap::arg("Pr:*:Links"), asap::arg("Pr:*")]];

Rectangle
  * loop [[asap::arg("Pr:Links"), asap::arg("*")]],
  * next [[asap::arg("Pr:Links"), asap::arg("Pr:Next")]],
  ** pnext [[asap::arg("Links"), asap::arg("Pr:Links"), asap::arg("Pr:Next")]];

Rectangle ** pnextstar [[asap::arg("Links"), 
                         asap::arg("Pr:Links"), 
                         asap::arg("Pr:*:Next")]];

void do_pointer_stuff(int _x, int _y) 
  [[asap::writes("Pr:R1:*")]]
  [[asap::writes("Pr:R2:*")]]
  //[[asap::writes("P:R1:Y")]] // Y not declared here TODO: support it
  [[asap::writes("Pr:*:Links")]]
  [[asap::writes("Links")]]
  [[asap::reads("Pr:Next")]]
{
  p1.x = _x;          // writes Pr:R1:X
  p1.y = _y;          // writes Pr:R1:Y
  p2.x = _x + 5;      // writes Pr:R2:X
  p2.y = _y + 5;      // writes Pr:R2:Y
  p1.x = next->p1.x;
  pp = &*&p1;         // writes Pr:Links                                                              expected-warning{{invalid assignment}}
  ppstar = &*&p1;     // writes Pr:Links
  ppp = &(*&pp);      // writes Links
  *ppp = pp = &p2;         // reads Links, writes Pr:Links              expected-warning{{RHS region 'Pr:R2' is not included in LHS region 'Pool:*' invalid assignment}}
  *ppp = ppstar = &p2;         // reads Links, writes Pr:Links          expected-warning{{RHS region 'Pr:*' is not included in LHS region 'Pool:*' invalid assignment}}
  ppstar = &p2;         // reads Links, writes Pr:Links
  ppstar1 = &p1;
  ppstar1 = &p2;      // expected-warning{{invalid assignment}}
  pppstar = &ppstar1;

  *pppstar = &p2;     // reads Links, writes Pr:Links                   expected-warning{{cannot modify aliasing through pointer to partly specified region}}

  *ppp = *&pp;        // reads Links, writes Pr:Links + reads Pr:Links
  *ppp = next->pp;    // reads Links, writes Pr:Links + reads Pr:Links, Pr:Next:Links
  *ppp = *&(next->pp);    // reads Links, writes Pr:Links + reads Pr:Links, Pr:Next:Links
  ppp = &next->pp;        // writes Links + reads Pr:Links                                            expected-warning{{invalid assignment}}
  pppstar = &next->ppstar;        // writes Links + reads Pr:Links
  *ppp = *(*&next->ppp);  // reads Links, writes Pr:Links + reads Links, Pr:Links, Pr:Next:Links
  *ppp = &*pp;            // reads Links, writes Pr:Links + reads Pr:Links
  next = this;        // writes Pr:Links (the 'this' pointer is immutable, so   expected-warning{{RHS region 'Pr' is not included in LHS region 'Pr:Next' invalid assignment}}
                      // write effects are not possible and read effects on it 
                      // can be discarded
  loop = this;
  loop = next;

  pnext = &next;              // writes Links 
  *pnext = next->next;        // reads Links, writes Pr:Links + reads Pr:Links, Pr:Next:Links         expected-warning{{invalid assignment}}
  *pnext = *next->pnext;      // reads Links, writes Pr:Links + reads Pr:Links, Links, Pr:Next:Links  expected-warning{{invalid assignment}}
  *pnext = *(*pnext)->pnext;  // reads Links, writes Pr:Links + reads Links, Pr:Links, Pr:Next:Links  expected-warning{{invalid assignment}}
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
__attribute__((region("X")))
__attribute__((region("Y")))
__attribute__((param("Pp")))
Point {
public:
  int __attribute__((arg("Pp:X"))) x;
  int __attribute__((arg("Pp:Y"))) y;
}; // end Point

class
//__attribute__((region_param("P, P<=Pool")))
__attribute__((param("Pr")))
__attribute__((region("R1"), region("R2"), region("Next"), region("Links") ))
Rectangle {

// Fields
Point p1 __attribute__((arg("Pr:R1")));
Point p2 __attribute__((arg("Pr:R2")));

Point 
  * pp __attribute__((arg("Pr:Links"), arg("Pool:*"))),
  **ppp __attribute__((arg("Links"), arg("Pr:Links"), arg("Pool:*")));

Point
  * ppstar __attribute__((arg("Pr:Links"), arg("Pr:*"))),
  * ppstar1 __attribute__((arg("Pr:Links"), arg("Pr:*:R1"))),
  **pppstar __attribute__((arg("Links"), arg("Pr:*:Links"), arg("Pr:*")));

Rectangle
  * loop __attribute__((arg("Pr:Links"), arg("*"))),
  * next __attribute__((arg("Pr:Links"), arg("Pr:Next"))),
  ** pnext __attribute((arg("Links"), arg("Pr:Links"), arg("Pr:Next")));

Rectangle ** pnextstar __attribute((arg("Links"), 
                                    arg("Pr:Links"), 
                                    arg("Pr:*:Next")));

void do_pointer_stuff(int _x, int _y) 
  __attribute__((writes("Pr:R1:*")))
  __attribute__((writes("Pr:R2:*")))
  //__attribute__((writes("P:R1:Y"))) // Y not declared here TODO: support it
  __attribute__((writes("Pr:*:Links")))
  __attribute__((writes("Links")))
  __attribute__((reads("Pr:Next")))
{
  p1.x = _x;          // writes Pr:R1:X
  p1.y = _y;          // writes Pr:R1:Y
  p2.x = _x + 5;      // writes Pr:R2:X
  p2.y = _y + 5;      // writes Pr:R2:Y
  p1.x = next->p1.x;
  pp = &*&p1;         // writes Pr:Links                                                              expected-warning{{invalid assignment}}
  ppstar = &*&p1;     // writes Pr:Links
  ppp = &(*&pp);      // writes Links
  *ppp = pp = &p2;         // reads Links, writes Pr:Links              expected-warning{{RHS region 'Pr:R2' is not included in LHS region 'Pool:*' invalid assignment}}
  *ppp = ppstar = &p2;         // reads Links, writes Pr:Links          expected-warning{{RHS region 'Pr:*' is not included in LHS region 'Pool:*' invalid assignment}}
  ppstar = &p2;         // reads Links, writes Pr:Links
  ppstar1 = &p1;
  ppstar1 = &p2;      // expected-warning{{invalid assignment}}
  pppstar = &ppstar1;

  *pppstar = &p2;     // reads Links, writes Pr:Links                   expected-warning{{cannot modify aliasing through pointer to partly specified region}}

  *ppp = *&pp;        // reads Links, writes Pr:Links + reads Pr:Links
  *ppp = next->pp;    // reads Links, writes Pr:Links + reads Pr:Links, Pr:Next:Links
  *ppp = *&(next->pp);    // reads Links, writes Pr:Links + reads Pr:Links, Pr:Next:Links
  ppp = &next->pp;        // writes Links + reads Pr:Links                                            expected-warning{{invalid assignment}}
  pppstar = &next->ppstar;        // writes Links + reads Pr:Links
  *ppp = *(*&next->ppp);  // reads Links, writes Pr:Links + reads Links, Pr:Links, Pr:Next:Links
  *ppp = &*pp;            // reads Links, writes Pr:Links + reads Pr:Links
  next = this;        // writes Pr:Links (the 'this' pointer is immutable, so   expected-warning{{RHS region 'Pr' is not included in LHS region 'Pr:Next' invalid assignment}}
                      // write effects are not possible and read effects on it 
                      // can be discarded
  loop = this;
  loop = next;

  pnext = &next;              // writes Links 
  *pnext = next->next;        // reads Links, writes Pr:Links + reads Pr:Links, Pr:Next:Links         expected-warning{{invalid assignment}}
  *pnext = *next->pnext;      // reads Links, writes Pr:Links + reads Pr:Links, Links, Pr:Next:Links  expected-warning{{invalid assignment}}
  *pnext = *(*pnext)->pnext;  // reads Links, writes Pr:Links + reads Links, Pr:Links, Pr:Next:Links  expected-warning{{invalid assignment}}
  *pnextstar = next->next;        // reads Links, writes Pr:Links + reads Pr:Links, Pr:Next:Links
  *pnextstar = *next->pnext;      // reads Links, writes Pr:Links + reads Pr:Links, Links, Pr:Next:Links
  *pnextstar = *(*pnext)->pnext;  // reads Links, writes Pr:Links + reads Links, Pr:Links, Pr:Next:Links
}

// TODO 
void do_reference_stuff() ;

}; // end class Rectangle

} // end namespace Pool
#endif


