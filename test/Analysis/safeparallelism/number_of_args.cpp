// RUN: %clang_cc1 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//

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

  int data;
  C2 __attribute__((arg("P:L"))) * left;
  C2 __attribute__((arg("P:R"))) * right;
  C2 __attribute__((arg("P:*"))) ** last_visited_link;
  int __attribute__((arg("P:*"))) *last_visited_data;

};
