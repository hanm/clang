// RUN: %clang_cc1 -DASP_GNU_SYNTAX %s -verify
// RUN: %clang_cc1 -DASP_CXX11_SYNTAX -std=c++11 %s -verify

#ifdef ASP_GNU_SYNTAX

class 
__attribute__ ((param)) // expected-error {{attribute takes one argument}}
__attribute__((region)) // expected-error {{attribute takes one argument}}
Coo {
  int __attribute__((arg)) money; // expected-error {{attribute takes one argument}}

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
  int __attribute__((arg("P:Links", "Links:Butter"))) money; // expected-error {{attribute takes one argument}}

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
  Coo __attribute__((arg)) c; // expected-error {{attribute takes one argument}}
  Gar __attribute__((arg("Roo", "Raa"))) g; // expected-error {{attribute takes one argument}}
  g.set_money(17);
  return 0; 
}

#endif

#ifdef ASP_CXX11_SYNTAX

class 
[[asap::param]] // expected-error {{attribute takes one argument}}
[[asap::region]] // expected-error {{attribute takes one argument}}
Coo {
  int [[asap::arg]] money; // expected-error {{attribute takes one argument}}

public:
  Coo (): money(70) {}

  int get_some() [[asap::no_effect]]{ 
    return money;
  }

  void set_money(int cash) [[asap::writes]] { // expected-error {{attribute takes one argument}}
    money = cash;
  }
};

class 
[[asap::param("P", "P2")]]  // expected-error {{attribute takes one argument}}
[[asap::region("Links", "Butter")]] // expected-error {{attribute takes one argument}}
Gar {
  int [[asap::arg("P:Links", "Links:Butter")]] money; // expected-error {{attribute takes one argument}}

public:
  Gar (): money(70) {}

  int get_some() [[asap::no_effect("Butter")]]{  // expected-error {{attribute takes no argument}} 
    return money;
  }

  void set_money(int cash) [[asap::writes("P:Links", "Links:Butter")]] { // expected-error {{attribute takes one argument}}
    money = cash;
  }
};



[[asap::region("Roo")]]  
[[asap::region("Raa")]]
int main (void) {
  Coo [[asap::arg]] c; // expected-error {{attribute takes one argument}}
  Gar [[asap::arg("Roo", "Raa")]] g; // expected-error {{attribute takes one argument}}
  g.set_money(17);
  return 0; 
}
#endif
  
