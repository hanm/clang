// RUN: %clang_cc1 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//


/// Test Valid Region Names & Declared RPL elements
class 
__attribute__((region("1R1")))        //expected-warning {{invalid region name}}
__attribute__((param("1P1")))  // expected-warning {{invalid region parameter name}} 
__attribute__((region("R1"), region("R2"), region("R3"), 
               region("_R2"), region("R_2"), region("R_2_c")))
C0 { 
};

class
__attribute__((region("R2")))
__attribute__((region("R3")))
__attribute__((param("P1")))
C1 { 
  // TODO: check that money is not annotated with an arg annotation
  int money0 __attribute__((arg("P1:R1"))); // expected-warning {{RPL element was not declared}} 
  int money __attribute__((arg("P1:R3"))); 
  C1 __attribute__((arg("P1:R2"))) * __attribute__((arg("P1"))) next;

public:
  __attribute__((param("*"))) // expected-warning {{invalid region parameter name}}
  C1 (): money(70) {} 

  __attribute__((param("Pc")))
  C1 (C1& __attribute__((arg("Pc"))) c)
    __attribute__((reads("Pc:*")))
    __attribute__((writes("P1:*")))
    :
    money(c.money) ,
    next(c.next)
  {}

  int get_money() __attribute__((reads("P1:R3"))) 
  { return money; }

  int get_next_money() __attribute__((reads("P1"))) __attribute__((reads("P1:R2:R3")))
  { return next->money; }

  void set_money(int cash) 
    __attribute__((writes("P1:R3")))
  { money = cash; }

  void set_next_money(int cash) 
    __attribute__((reads("P1")))
    __attribute__((writes("P1:R2:R3")))
  { next->money = cash; }

  void set_next_money_to_this_money () 
    __attribute__((writes("P1:R2:R3")))
    __attribute__((reads("P1")))
    __attribute__((reads("P1:R3")))
  { next->money = money; }

  void do_stuff(int cash) 
    __attribute__((reads("P1")))
    __attribute__((reads("P1:R2")))
    __attribute__((reads("P1:R2:R3")))
    __attribute__((reads("P1:R2:R2:R3"))) 
    __attribute__((reads("P1:R3")))         // expected-warning{{effect summary is not minimal}}
    __attribute__((atomic_reads("P1:R3")))  // expected-warning{{effect summary is not minimal}}
    __attribute__((atomic_writes("P1:R3"))) // expected-warning{{effect summary is not minimal}}
    __attribute__((writes("P1:R3"))) 
    __attribute__((no_effect))                   // expected-warning{{effect summary is not minimal}}
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
  void subtract_money(int cash) __attribute__((reads("P1:R1"))) { // expected-warning {{RPL element was not declared}}  
    money -= (cash) ? cash+3 : cash=3;  // expected-warning{{effect not covered by effect summary}}
    money -= cash;                      // expected-warning{{effect not covered by effect summary}}
  }

  bool insured __attribute__((arg("P1:R1")));// expected-warning {{RPL element was not declared}} 
  char* name __attribute__((arg("P1")));
};

