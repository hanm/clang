// RUN: %clang_cc1 -DASAP_GNU_SYNTAX %s -verify
// RUN: %clang_cc1 -DASAP_CXX11_SYNTAX -std=c++11 %s -verify

#ifdef ASAP_GNU_SYNTAX
class 
__attribute__ ((param("P1"), param("P2"))) // expected-warning {{duplicate attribute not allowed:}}
__attribute__((region("R"))) 
Coo {
  int money __attribute__((arg("P1:R"))) __attribute__((arg("P2:R"))); // expected-warning {{duplicate attribute not allowed:}}

public:
  Coo (): money(70) {}

  int get_some() __attribute__ ((no_effect)) { 
    return money;
  }

  void set_money(int cash) __attribute__((writes("P2:R"))) { 
    money = cash;
  }
};


__attribute__((region("Roo, Raa")))  
int main (void) {
  Coo c __attribute__((arg("Roo"))) __attribute__((arg("Raa"))); // expected-warning {{duplicate attribute not allowed:}}
  c.set_money(17);
  return 0; 
}
#endif
 
#ifdef ASAP_CXX11_SYNTAX
class 
[[asap::param("P1")]] [[asap::param("P2")]] // expected-warning {{duplicate attribute not allowed:}}
[[asap::region("R")]] 
Coo {
  int money [[asap::arg("P1:R")]] [[asap::arg("P2:R")]]; // expected-warning {{duplicate attribute not allowed:}}

public:
  Coo (): money(70) {}

  int get_some [[asap::no_effect]] () { 
    return money;
  }

  void set_money [[asap::writes("P2:R")]] (int cash) { 
    money = cash;
  }
};


[[asap::region("Roo, Raa")]] 
int main (void) {
  Coo c [[asap::arg("Roo")]] [[asap::arg("Raa")]]; // expected-warning {{duplicate attribute not allowed:}}
  c.set_money(17);
  return 0; 
}
#endif
