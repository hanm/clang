// RUN: %clang_cc1 -DASAP_GNU_SYNTAX %s -verify
// RUN: %clang_cc1 -DASAP_CXX11_SYNTAX -std=c++11 %s -verify

// expected-no-diagnostics
//
#ifdef ASAP_GNU_SYNTAX
class 
__attribute__((region("Links")))
__attribute__ ((param("P"))) 
Coo {
  int money __attribute__((arg("Roo")));

public:
  Coo (): money(70) {}

  int get_some() __attribute__ ((no_effect)){ 
    return money;
  }

  void set_money(int cash) __attribute__((writes("P:Links"))) {
    money = cash;
  }
};


__attribute__((region("Roo"))) 
int main (void) {
  Coo c __attribute__((arg("Roo")));
  c.set_money(42);
  return 0; 
}
#endif
 
#ifdef ASAP_CXX11_SYNTAX
class 
[[asap::region("Links")]]
[[asap::param("P")]]
Coo {
  int money [[asap::arg("P:Links")]];

public:
  Coo (): money(70) {}

  int get_some() [[asap::no_effect]] { 
    return money;
  }

  void set_money(int cash) [[asap::writes("P:Links")]] {
    money = cash;
  }
};

[[asap::region("Roo")]]
int main (void) {
  Coo c [[asap::arg("Roo")]]; 
  c.set_money(42);
  return 0; 
}
#endif
