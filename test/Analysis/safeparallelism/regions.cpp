// RUN: %clang_cc1 -DASAP_CXX11_SYNTAX -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
// RUN: %clang_cc1 -DASAP_GNU_SYNTAX -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify

#ifdef ASAP_CXX11_SYNTAX
/// Test Valid Region Names & Declared RPL elements
class 
[[asap::region("1R1")]]        //expected-warning {{invalid region name}}
[[asap::param("1P1")]]  // expected-warning {{invalid region parameter name}} 
[[asap::region("R1, R2, R3, _R2, R_2, R_2_c")]]
C0 { 
};

class
[[asap::region("R2, R3")]]
[[asap::param("P1")]]
C1 { 
  int money [[asap::arg("P1:R3")]];
  //int *money_p [[asap::arg("R2"), asap::arg("P1:R3")]];  
  //C1 *next [[asap::arg("P1"), asap::arg("P1:R2")]];
  int *money_p [[asap::arg("R2, P1:R3")]];  
  C1 *next [[asap::arg("P1, P1:R2")]];

public:
  [[asap::param("*")]] // expected-warning {{invalid region parameter name}}
  C1 (): money(70) {} 

  [[asap::param("Pc")]]
  [[asap::reads("Pc:R3")]]
  C1 (C1& ccc [[asap::arg("Pc")]])
    :
    money(ccc.money),
    next(0)
  {}

  int get_money [[asap::reads("P1:R3")]] ()
  { return money; }

  int *get_money_p [[asap::arg("P1:R3"), asap::reads("P1:R3")]] ()
  { return &money; }

  int get_next_money [[asap::reads("P1")]] [[asap::reads("P1:R2:R3")]] ()
  { return next->money; }

  void set_money
    [[asap::writes("P1:R3")]] (int cash0)
  { money = cash0; }

  int close_account [[asap::reads("P1:R3")]] () {
    int balance = money;
    set_money(0); // writes P1:R3  expected-warning{{'Writes Effect on P1:R3': effect not covered by effect summary}}
    next->set_money(0); // reads R1, writes P1:R2:R3  expected-warning{{'Reads Effect on P1': effect not covered by effect summary}}  expected-warning{{'Writes Effect on P1:R2:R3': effect not covered by effect summary}}
    return balance;
  }

  void set_next_money 
    [[asap::reads("P1")]]
    [[asap::writes("P1:R2:R3")]] (int cash)
  { next->money = cash; }

  void set_next_money_to_this_money
    [[asap::writes("P1:R2:R3")]]
    [[asap::reads("P1, P1:R3")]] ()
  { next->money = money; }

  void do_stuff
    [[asap::reads("P1, P1:R2, P1:R2:R3, P1:R2:R2:R3, P1:R3")]]  // expected-warning{{effect summary is not minimal}}
    [[asap::atomic_reads("P1:R3")]]  // expected-warning{{effect summary is not minimal}}
    [[asap::atomic_writes("P1:R3")]] // expected-warning{{effect summary is not minimal}}
    [[asap::writes("P1:R3")]] 
    [[asap::no_effect]]              // expected-warning{{no_effect is illegal in non-empty effect summary}}
    (int cash)
  {
    money = 42;                 // writes P1:R3 
    cash += next->money;        // reads P1, P1:R2:R3
    cash -= next->next->money;  // reads P1, P1:R2, P1:R2:R2:R3
    money++;                    // writes P1:R3
    money--;                    // writes P1:R3
    ++money;                    // writes P1:R3
    --money;                    // writes P1:R3
    money = cash + 4 + cash;    // writes P1:R3
    !cash ? (cash = money) : money++;   // writes P1:R3 or reads P1:R3
    cash ? money++ : (cash = money = 42);   // writes P1:R3 or reads P1:R3
    cash ? money++ : (cash = money = cash);   // writes P1:R3 or reads P1:R3
  }

  [[asap::region("R3")]]
  void add_money(int cash) {
    money += cash; // expected-warning {{effect not covered by effect summary}}
  }

  //char* name [[asap::arg("P1"), asap::arg("P1")]];
  char* name [[asap::arg("P1, P1")]];
};
#endif

#ifdef ASAP_GNU_SYNTAX
/// Test Valid Region Names & Declared RPL elements
class 
__attribute__((region("1R1")))        //expected-warning {{invalid region name}}
__attribute__((param("1P1")))  // expected-warning {{invalid region parameter name}} 
__attribute__((region("R1, R2, R3, _R2, R_2, R_2_c")))
C0 { 
};

class
__attribute__((region("R2, R3"), param("P1")))
C1 { 
  // TODO: check that money is not annotated with an arg annotation
  int money __attribute__((arg("P1:R3"))); 
  C1 *next __attribute__((arg("P1, P1:R2")));

public:
  __attribute__((param("*"))) // expected-warning {{invalid region parameter name}}
  C1 (): money(70) {} 

  __attribute__((param("Pc")))
  C1 (C1& __attribute__((arg("Pc"))) c)
    __attribute__((reads("Pc:*")))
    :
    money(c.money),
    next(0)
  {}

  int get_money() __attribute__((reads("P1:R3"))) 
  { return money; }

  int get_next_money() __attribute__((reads("P1"))) __attribute__((reads("P1:R2:R3")))
  { return next->money; }

  void set_money(int cash) 
    __attribute__((writes("P1:R3")))
  { money = cash; }

  int close_account() __attribute__((reads("P1:R3"))) {
    int balance = money;
    set_money(0); // expected-warning{{'Writes Effect on P1:R3': effect not covered by effect summary}}
    return balance;
  }

  void set_next_money(int cash) 
    __attribute__((reads("P1")))
    __attribute__((writes("P1:R2:R3")))
  { next->money = cash; }

  void set_next_money_to_this_money () 
    __attribute__((writes("P1:R2:R3")))
    __attribute__((reads("P1, P1:R3")))
  { next->money = money; }

  void do_stuff(int cash) 
    __attribute__((reads("P1, P1:R2, P1:R2:R3, P1:R2:R2:R3, P1:R3")))  // expected-warning{{effect summary is not minimal}}
    __attribute__((atomic_reads("P1:R3")))  // expected-warning{{effect summary is not minimal}}
    __attribute__((atomic_writes("P1:R3"))) // expected-warning{{effect summary is not minimal}}
    __attribute__((writes("P1:R3"))) 
    __attribute__((no_effect))              // expected-warning{{no_effect is illegal in non-empty effect summary}}
  { 
    cash += next->money;        // reads P1, P1:R2:R3
    cash -= next->next->money;  // reads P1, P1:R2, P1:R2:R2:R3
    money++;                    // writes P1:R3
    money--;                    // writes P1:R3
    ++money;                    // writes P1:R3
    --money;                    // writes P1:R3
    money = cash + 4 + cash;    // writes P1:R3
    !cash ? (cash = money) : money++;   // writes P1:R3 or reads P1:R3
    cash ? money++ : (cash = money = 42);   // writes P1:R3 or reads P1:R3
  }

  __attribute__((region("R3")))
  void add_money(int cash) {
    money += cash; // expected-warning {{effect not covered by effect summary}}
  }

  char* name __attribute__((arg("P1, P1")));
};
#endif

