// RUN: %clang_cc1 -DASAP_GNU_SYNTAX -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
// RUN: %clang_cc1 -DASAP_CXX11_SYNTAX -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify

#ifdef ASAP_CXX11_SYNTAX
// Correct number of arg annotations for the annotated types
class 
  [[asap::param("P")]]
  [[asap::region("R, L, Links")]]
    C0 {

  int data [[asap::arg("P")]];

  C0 *left [[asap::arg("Links, P:L")]];

  C0 *right[[asap::arg("Links, P:R")]] ;

  C0 **last_visited_link[[asap::arg("Links, P, P:*")]];

  int *last_visited_data[[asap::arg("P, P:*")]];

  void memberFoo(C0 *p [[asap::arg("P")]]) {
    int *local_p1 [[asap::arg("P")]];
  }

  void memberFoo2(C0 *p [[asap::arg("Local, P")]]) { 
    int *local_p1 [[asap::arg("Local, P")]]; 
  }

};

// Too many arg annotations
class 
  [[asap::param("P")]]
  [[asap::region("R, L, Links")]]
    C1 {

  int data [[asap::arg("P:R, P, P:L")]];   // expected-warning{{superfluous region argument}}

  C1 *left[[asap::arg("P:L, Links, Links")]]; // expected-warning{{superfluous region argument}} 

  C1 *right  [[asap::arg("Links, P:R, Links")]];   // expected-warning{{superfluous region argument}}

  C1 **last_visited_link  [[asap::arg("Links, P, P:*, P")]]; // expected-warning{{superfluous region argument}} 

  int *last_visited_data [[asap::arg("P, P, P:*")]]; // expected-warning{{superfluous region argument}};

  void memberFoo(C0 *p [[asap::arg("Local, P, Local")]]) { // expected-warning{{superfluous region argument}};
    int *local_p1 [[asap::arg("Local, P, Local")]]; // expected-warning{{superfluous region argument}};
  }

};

// Too few arg annotations (the rest of them will use the defaults
// or will be inferred)
class 
  [[asap::param("P")]]
  [[asap::region("R, L, Links")]]
    C2 {

  C2 *left [[asap::arg("P:L")]]; // expected-warning{{missing region argument(s)}}
  C2 *right [[asap::arg("P:R")]]; // expected-warning{{missing region argument(s)}}
  C2 **last_visited_link [[asap::arg("P:*")]]; // expected-warning{{missing region argument(s)}}
  int *last_visited_data [[asap::arg("P:*")]]; // expected-warning{{missing region argument(s)}}

  void memberFoo(C0 *p) { // default-arg inserted
    int *local_p1;        // default-arg inserted
  }

};
#endif

#ifdef ASAP_GNU_SYNTAX
// Correct number of arg annotations for the annotated types
class 
  __attribute__((param("P")))
  __attribute__((region("R, L, Links")))
    C0 {

  int __attribute__((arg("P"))) data;

  C0 *left __attribute__((arg("P:L, Links")));

  C0 *right __attribute__((arg("P:R, Links")));

  C0 **last_visited_link __attribute__((arg("P:*, Links, P")));

  int *last_visited_data __attribute__((arg("P:*, P")));

};

// Too many arg annotations
class 
  __attribute__((param("P")))
  __attribute__((region("R, L, Links")))
    C1 {

  int data __attribute__((arg("P, P:R, P:L")));   // expected-warning{{superfluous region argument}}

  C1 *left __attribute__((arg("P:L, Links, Links"))); // expected-warning{{superfluous region argument}}

  C1 *right __attribute__((arg("P:R, Links, Links"))); // expected-warning{{superfluous region argument}}

  C1 **last_visited_link __attribute__((arg("P:*, Links, P, P")));  // expected-warning{{superfluous region argument}}

  int *last_visited_data __attribute__((arg("P:*, P, P"))); // expected-warning{{superfluous region argument}}

};

// Too few arg annotations (the rest of them will use the defaults
// or will be inferred)
class 
  __attribute__((param("P"))) 
  __attribute__((region("R, L, Links")))
    C2 {

  C2 __attribute__((arg("P:L"))) * left; // expected-warning{{missing region argument(s)}}
  C2 __attribute__((arg("P:R"))) * right; // expected-warning{{missing region argument(s)}}
  C2 __attribute__((arg("P:*"))) ** last_visited_link; // expected-warning{{missing region argument(s)}}
  int __attribute__((arg("P:*"))) *last_visited_data; // expected-warning{{missing region argument(s)}}

};

#endif
