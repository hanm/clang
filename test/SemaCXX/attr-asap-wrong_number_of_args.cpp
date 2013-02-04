// RUN: %clang_cc1 -DASAP_GNU_SYNTAX %s -verify
// RUN: %clang_cc1 -DASAP_CXX11_SYNTAX -std=c++11 %s -verify

#ifdef ASAP_GNU_SYNTAX

class 
__attribute__ ((param)) // expected-error {{attribute takes one argument}}
__attribute__((region)) // expected-error {{attribute takes one argument}}
Coo {
  int money __attribute__((arg)); // expected-error {{attribute takes one argument}}

public:
  Coo (): money(70) {}

  int get_some() __attribute__ ((no_effect)){ 
    return money;
  }

  void set_money(int cash) __attribute__((writes)) { // expected-error {{attribute takes one argument}}
    money = cash;
  }
};

class 
__attribute__ ((param("P", "P2")))  // expected-error {{attribute takes one argument}}
__attribute__((region("Links", "Butter"))) // expected-error {{attribute takes one argument}}
Gar {
  int money __attribute__((arg("P:Links", "Links:Butter"))); // expected-error {{attribute takes one argument}}

public:
  Gar (): money(70) {}

  int get_some() __attribute__ ((no_effect("Butter"))){  // expected-error {{attribute takes no argument}} 
    return money;
  }

  void set_money(int cash) __attribute__((writes("P:Links", "Links:Butter"))) { // expected-error {{attribute takes one argument}}
    money = cash;
  }
};



__attribute__((region("Roo")))  
__attribute__((region("Raa")))
int main (void) {
  Coo c __attribute__((arg)); // expected-error {{attribute takes one argument}}
  Gar g __attribute__((arg("Roo", "Raa"))); // expected-error {{attribute takes one argument}}
  g.set_money(17);
  return 0; 
}

#endif

#ifdef ASAP_CXX11_SYNTAX

class 
[[asap::param]] // expected-error {{attribute takes one argument}}
[[asap::region]] // expected-error {{attribute takes one argument}}
Coo {
  int money [[asap::arg]]; // expected-error {{attribute takes one argument}}

public:
  Coo (): money(70) {}

  int get_some [[asap::no_effect]] (){ 
    return money;
  }

  void set_money [[asap::writes]] (int cash) { // expected-error {{attribute takes one argument}}
    money = cash;
  }
};

class 
[[asap::param("P", "P2")]]  // expected-error {{attribute takes one argument}}
[[asap::region("Links", "Butter")]] // expected-error {{attribute takes one argument}}
Gar {
  int money [[asap::arg("P:Links", "Links:Butter")]]; // expected-error {{attribute takes one argument}}

public:
  Gar (): money(70) {}

  int get_some [[asap::no_effect("Butter")]] (){  // expected-error {{attribute takes no argument}} 
    return money;
  }

  void set_money [[asap::writes("P:Links", "Links:Butter")]] (int cash) { // expected-error {{attribute takes one argument}}
    money = cash;
  }
};



[[asap::region("Roo")]]  
[[asap::region("Raa")]]
int main (void) {
  Coo c [[asap::arg]]; // expected-error {{attribute takes one argument}}
  Gar g [[asap::arg("Roo", "Raa")]]; // expected-error {{attribute takes one argument}}
  g.set_money(17);
  return 0; 
}
#endif
  
