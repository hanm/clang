// RUN: %clang_cc1 -DASAP_GNU_SYNTAX -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
// RUN: %clang_cc1 -DASAP_CXX11_SYNTAX -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify

#ifdef ASAP_CXX11_SYNTAX
// Correct number of arg annotations for the annotated types
class 
  [[asap::param("P")]]
  [[asap::region("R")]]
  [[asap::region("L")]]
  [[asap::region("Links")]]
    C0 {

  int data [[asap::arg("P")]];

  C0 *left [[asap::arg("Links"), asap::arg("P:L")]];

  C0 *right[[asap::arg("Links"), asap::arg("P:R")]] ;

  C0 **last_visited_link[[asap::arg("Links"),
                          asap::arg("P"),
                          asap::arg("P:*")]];

  int *last_visited_data[[asap::arg("P"), asap::arg("P:*")]];

};

// Too many arg annotations
class 
  [[asap::param("P")]]
  [[asap::region("R")]]
  [[asap::region("L")]]
  [[asap::region("Links")]]
    C1 {

  int data [[asap::arg("P:R")]]
           [[asap::arg("P")]]    // expected-warning{{superfluous region argument}} 
           [[asap::arg("P:L")]];   // expected-warning{{superfluous region argument}}

  C1 *left[[asap::arg("P:L")]]   
      [[asap::arg("Links")]]
      [[asap::arg("Links")]]; // expected-warning{{superfluous region argument}} 

  C1 *right  [[asap::arg("Links")]]
             [[asap::arg("P:R")]]
             [[asap::arg("Links")]];   // expected-warning{{superfluous region argument}}

  C1 **last_visited_link  [[asap::arg("Links")]] 
                          [[asap::arg("P")]] 
                          [[asap::arg("P:*")]]   
                          [[asap::arg("P")]]; // expected-warning{{superfluous region argument}} 

  int *last_visited_data [[asap::arg("P")]]
                         [[asap::arg("P")]]
                         [[asap::arg("P:*")]]; // expected-warning{{superfluous region argument}};

};

// Too few arg annotations (the rest of them will use the defaults
// or will be inferred)
class 
  [[asap::param("P")]]
  [[asap::region("R")]]
  [[asap::region("L")]]
  [[asap::region("Links")]]
    C2 {

  int data; // expected-warning{{missing region argument(s)}}
  C2 *left [[asap::arg("P:L")]]; // expected-warning{{missing region argument(s)}}
  C2 *right [[asap::arg("P:R")]]; // expected-warning{{missing region argument(s)}}
  C2 **last_visited_link [[asap::arg("P:*")]]; // expected-warning{{missing region argument(s)}}
  int *last_visited_data [[asap::arg("P:*")]]; // expected-warning{{missing region argument(s)}}

};
#endif

#ifdef ASAP_GNU_SYNTAX
// Correct number of arg annotations for the annotated types
class 
  __attribute__((param("P")))
  __attribute__((region("R")))
  __attribute__((region("L")))
  __attribute__((region("Links")))
    C0 {

  int __attribute__((arg("P"))) data;

  C0 __attribute__((arg("P:L"))) 
      * __attribute__((arg("Links"))) left;

  C0 __attribute__((arg("P:R"))) 
      * __attribute__((arg("Links"))) right;

  C0 __attribute__((arg("P:*"))) 
      * __attribute__((arg("Links"))) 
      * __attribute__((arg("P"))) last_visited_link;

  int __attribute__((arg("P:*")))
      * __attribute__((arg("P"))) last_visited_data;

};

// Too many arg annotations
class 
  __attribute__((param("P")))
  __attribute__((region("R")))
  __attribute__((region("L")))
  __attribute__((region("Links")))
    C1 {

  int __attribute__((arg("P")))    // expected-warning{{superfluous region argument}}
    data __attribute__((arg("P:R"))) 
    __attribute__((arg("P:L")));   // expected-warning{{superfluous region argument}}

  C1 __attribute__((arg("P:L")))   // expected-warning{{superfluous region argument}}
      * __attribute__((arg("Links"))) left 
      __attribute__((arg("Links"))); 

  C1 __attribute__((arg("P:R")))   // expected-warning{{superfluous region argument}}
      * __attribute__((arg("Links"))) right 
      __attribute__((arg("Links")));

  C1 __attribute__((arg("P:*")))   // expected-warning{{superfluous region argument}}
      * __attribute__((arg("Links"))) 
      * __attribute__((arg("P"))) last_visited_link
      __attribute__((arg("P")));

  int __attribute__((arg("P:*")))  // expected-warning{{superfluous region argument}}
      * __attribute__((arg("P"))) last_visited_data
      __attribute__((arg("P")));

};

// Too few arg annotations (the rest of them will use the defaults
// or will be inferred)
class 
  __attribute__((param("P"))) 
  __attribute__((region("R")))
  __attribute__((region("L")))
  __attribute__((region("Links")))
    C2 {

  int data; // expected-warning{{missing region argument(s)}}
  C2 __attribute__((arg("P:L"))) * left; // expected-warning{{missing region argument(s)}}
  C2 __attribute__((arg("P:R"))) * right; // expected-warning{{missing region argument(s)}}
  C2 __attribute__((arg("P:*"))) ** last_visited_link; // expected-warning{{missing region argument(s)}}
  int __attribute__((arg("P:*"))) *last_visited_data; // expected-warning{{missing region argument(s)}}

};

#endif
