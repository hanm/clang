// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=param %s -verify

class [[asap::param("P")]]
      C {
  int x [[asap::arg("P")]];
  int y [[asap::arg("P")]];
public:
  int foo [[asap::param("Pc"), asap::reads("P, Pc")]]
          (C c[[asap::arg("Pc")]]) { 
    if ( x == c.x)
      return 1;
    else
      return 0; 
  }

  // Constructor
  C(): x(0), y(0) {}

  // Copy Constructor
  [[asap::param("Pcc"), asap::reads("Pcc")]]
  C(const C& c[[asap::arg("Pcc")]]) : x(c.x), y(c.y) {}

  // Move Constructor
  [[asap::param("Pmc"), asap::reads("Pmc")]]
  C(const C&& c[[asap::arg("Pmc")]]) : x(c.x), y(c.y) {}

}; // end class C

void bar(void) {
  C c;
  c.foo(C()); // expected-warning{{region argument required but not yet supported in this syntax}}
  C b;
  c.foo(b);
}

/*
Many things are going wrong. 
1. c.foo(b);
   Calling foo implicitly call the copy constructor but right now we don't support inferring 
   the appropriate region parameter substitution. b is passed to the copy constructor which
   substitutes Pcc<-Local, but the constructor returns an object C<P> and (a) we are not 
   defaulting that to C<Local> and (b) we are not even detecting that we need a substitution 
   Pc<-Local
2. c.foo(C());
   similarly, we don't deal recognize that a CXXMaterializeTemporaryExpr in fact calls the
   move constructor resulting in pretty much the same shortcommings.
*/
