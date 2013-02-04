// RUN: %clang_cc1 -DASAP_GNU_SYNTAX %s -verify
// RUN: %clang_cc1 -DASAP_CXX11_SYNTAX -std=c++11 %s -verify

#ifdef ASAP_GNU_SYNTAX
class
__attribute__((region("Money")))
__attribute__ ((param("P"))) 
__attribute__ ((atomic_reads("Lala"))) // expected-warning {{attribute only applies to functions}}
Coo {

  int money __attribute__((arg("P:Money")));

public:
  Coo () __attribute__((no_effect)) : money(0) {}

  Coo (int cash) __attribute__((no_effect)) : money(cash) {}

  int get_some() __attribute__((reads("P:Money"))){ 
    return money;
  }

  void set_money(int cash) __attribute__((writes("P:Money"))) {
    money = cash;
  }

  void add_money(int cash) __attribute__((atomic_writes("P:Money"))) {
    money += cash;
  }
};



__attribute__((region("Roo"))) 
int main (void) {
  Coo c __attribute__((arg("Roo"))) __attribute__((no_effect)); // expected-warning {{attribute only applies to functions}}
  int five __attribute__ ((reads("Roo"))) = 4+1; // expected-warning {{attribute only applies to functions}}
  int six __attribute__ ((no_effect)) = 4+2; // expected-warning {{attribute only applies to functions}}
  return 0; 
}

#endif

#ifdef ASAP_CXX11_SYNTAX
class
[[asap::region("Money")]]
[[asap::param("P")]]
[[asap::atomic_reads("Lala")]] // expected-warning {{attribute only applies to functions}}
Coo {

  int money [[asap::arg("P:Money")]];

public:
  [[asap::no_effect]] Coo() : money(0) {}

  [[asap::no_effect]] Coo(int cash) : money(cash) {}

  int get_some [[asap::reads("P:Money")]] () { 
    return money;
  }

  void set_money [[asap::writes("P:Money")]] (int cash) {
    money = cash;
  }

  void add_money [[asap::atomic_writes("P:Money")]] (int cash) {
    money += cash;
  }
};



[[asap::region("Roo")]] 
int main (void) {
  Coo c [[asap::arg("Roo")]] [[asap::no_effect]]; // expected-warning {{attribute only applies to functions}}
  int five [[asap::reads("Roo")]] = 4+1; // expected-warning {{attribute only applies to functions}}
  int six [[asap::no_effect]] = 4+2; // expected-warning {{attribute only applies to functions}}
  return 0; 
}

#endif
  
