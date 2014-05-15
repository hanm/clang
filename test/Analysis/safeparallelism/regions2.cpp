// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
// 

/// Test Valid Region Names & Declared RPL elements
class 
[[asap::region("1R1")]]        //expected-warning {{invalid region name}}
[[asap::param("1P1")]]  // expected-warning {{invalid region parameter name}} 
[[asap::region("R1, R2, R3, _R2, R_2, R_2_c")]]
C0 { 
};

class
[[asap::region("R2,R3")]]
[[asap::param("P1")]]
C1 { 
  // TODO: check that money is not annotated with an arg annotation
  int money0 [[asap::arg("P1:R2")]]; 
  int money [[asap::arg("P1:R3")]]; 
  C1 *next [[asap::arg("P1:R2, P1:R2")]];

public:
  [[asap::param("*")]] // expected-warning {{invalid region parameter name}}
  C1 (): money(70) {} 

  [[asap::writes("P1:*")]]
  C1 (C1& c [[asap::arg("P1")]])
    :
    money(c.money) ,
    next(c.next)
  {}

  int get_money [[asap::reads("P1:R3")]] ()
  { return money; }

  int get_next_money [[asap::reads("P1:R2, P1:R2:R3")]] ()
  { return next->money; }

  void set_money [[asap::writes("P1:R3")]] (int cash)
  { money = cash; }

  int close_account [[asap::reads("P1:R3")]] () {
    int balance = money;
    set_money(0); // expected-warning{{effect not covered by effect summary}}
    return balance;
  }

  void set_next_money [[asap::reads("P1:R2"), asap::writes("P1:R2:R3")]]
                      (int cash) 
  { next->money = cash; }

  void set_next_money_to_this_money 
    [[asap::writes("P1:R2:R3")]]
    [[asap::reads("P1:R2, P1:R3")]] ()
  { next->money = money; }

  void do_stuff
    [[asap::reads("P1:R2, P1:R2:R2, P1:R2:R3, P1:R2:R2:R3, P1:R3")]] // expected-warning{{effect summary is not minimal}}
    [[asap::atomic_reads("P1:R3")]]  // expected-warning{{effect summary is not minimal}}
    [[asap::atomic_writes("P1:R3")]] // expected-warning{{effect summary is not minimal}}
    [[asap::writes("P1:R3")]] 
    (int cash)                 
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
  [[asap::region("R3")]]
  void add_money(int cash) {
    money += cash; // expected-warning {{effect not covered by effect summary}}
  }
  void subtract_money [[asap::reads("P1:R2")]] (int cash) { 
    money -= (cash) ? cash+3 : cash=3;  // expected-warning{{effect not covered by effect summary}}
    money -= cash;                      // expected-warning{{effect not covered by effect summary}}
  }

  bool insured [[asap::arg("P1:R2")]];
  char* name [[asap::arg("P1,P1")]];
};

