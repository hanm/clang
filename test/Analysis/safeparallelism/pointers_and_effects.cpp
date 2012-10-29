// RUN: %clang_cc1 -DASAP_GNU_SYNTAX -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
// RUN: %clang_cc1 -DASAP_CXX11_SYNTAX -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//

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
__attribute__((region("R1")))
__attribute__((region("R2")))
__attribute__((region("Next")))
__attribute__((region("Links")))
//__attribute__((region("Pool")))
Rectangle {

// Fields
Point __attribute__((arg("Pr:R1"))) p1;
Point __attribute__((arg("Pr:R2"))) p2;
Point __attribute__((arg("Pool:*"))) 
  * pp __attribute__((arg("Pr:Links")));
Point __attribute__((arg("Pool:*"))) 
  **ppp __attribute__((arg("Links"))) 
          __attribute__((arg("Pr:Links")));

Rectangle __attribute__((arg("Pr:Next"))) 
  * next __attribute__((arg("Pr:Links")));

Rectangle __attribute__((arg("Pr:Next"))) 
  ** pnext __attribute((arg("Links")))  __attribute__((arg("Pr:Links")));

void do_pointer_stuff(int _x, int _y) 
  __attribute__((writes("Pr:R1:*")))
  //__attribute__((writes("P:R1:Y"))) // Y not declared here TODO: support it
  __attribute__((writes("Pr:R2:*")))
  __attribute__((writes("Pr:Links")))
  __attribute__((writes("Links")))
  __attribute__((reads("Pr:Next:Links")))
  __attribute__((reads("Pr:Next")))
{
  p1.x = _x;          // writes Pr:R1:X
  p1.y = _y;          // writes Pr:R1:Y
  p2.x = _x + 5;      // writes Pr:R2:X
  p2.y = _y + 5;      // writes Pr:R2:Y
  pp = &*&p1;         // writes Pr:Links
  ppp = &(*&pp);      // writes Links
  *ppp = &p2;         // reads Links, writes Pr:Links
  *ppp = *&pp;        // reads Links, writes Pr:Links + reads Pr:Links
  *ppp = next->pp;    // reads Links, writes Pr:Links + reads Pr:Links, Pr:Next:Links
  *ppp = *&(next->pp);    // reads Links, writes Pr:Links + reads Pr:Links, Pr:Next:Links
  ppp = &next->pp;        // writes Links + reads Pr:Links
  *ppp = *(*&next->ppp);  // reads Links, writes Pr:Links + reads Links, Pr:Links, Pr:Next:Links
  *ppp = &*pp;            // reads Links, writes Pr:Links + reads Pr:Links
  next = this;        // writes Pr:Links (the 'this' pointer is immutable, so 
                      // write effects are not possible and read effects on it 
                      // can be discarded

  pnext = &next;      // writes Links 
  *pnext = next->next;        // reads Links, writes Pr:Links + reads Pr:Links, Pr:Next:Links
  *pnext = *next->pnext;      // reads Links, writes Pr:Links + reads Pr:Links, Links, Pr:Next:Links
  *pnext = *(*pnext)->pnext;  // reads Links, writes Pr:Links + reads Links, Pr:Links, Pr:Next:Links
}

// TODO 
void do_reference_stuff() ;

}; // end class Rectangle

} // end namespace Pool
#endif

#ifdef ASAP_CXX11_SYNTAX

#endif
