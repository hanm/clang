// RUN: %clang_cc1 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//


/// Test Valid Region Names & Declared RPL elements
class 
__attribute__((region("1R1")))        // invalid region name
__attribute__((region_param("1P1")))  // invalid region parameter name
__attribute__((region("R2")))
__attribute__((region("R3")))
__attribute__((region("_R2")))
__attribute__((region("R_2")))
__attribute__((region("R_2c_")))
C0 { // expected-warning {{invalid region or parameter name}} expected-warning {{invalid region or parameter name}}
};

class
__attribute__((region("R2")))
//__attribute__((region("R3")))
__attribute__((region_param("P1")))
C1 { 
  // TODO: check that money is not annotated with an arg annotation
  int money __attribute__((in_region("P1:R3"))); // expected-warning {{RPL element was not declared}} 
  C1* __attribute__((region_arg("P1:R2"))) next __attribute__((in_region("P1")));

public:
  __attribute__((region_param("*")))
  C1 (): money(70) {} // expected-warning {{invalid region or parameter name}}

  __attribute__((region_param("Pc")))
  C1 (C1& __attribute__((region_arg("Pc"))) c)
    __attribute__((reads_effect("Pc:*")))
    __attribute__((writes_effect("P1:*")))
    :
    money(c.money) ,
    next(c.next)
  {}

  void set_money(int cash) // expected-warning {{RPL element was not declared}} expected-warning{{effect summary is not minimal}} expected-warning{{effect summary is not minimal}} expected-warning{{effect summary is not minimal}} expected-warning{{effect summary is not minimal}}
    __attribute__((reads_effect("P1")))
    __attribute__((reads_effect("P1:R2")))
    __attribute__((reads_effect("P1:R2:R3")))
    __attribute__((reads_effect("P1:R2:R2:R3"))) 
    __attribute__((reads_effect("P1:R3")))  
    __attribute__((atomic_reads_effect("P1:R3")))  
    __attribute__((atomic_writes_effect("P1:R3")))  
    __attribute__((writes_effect("P1:R3"))) 
    __attribute__((pure_effect)) 
  { 
    cash += next->money;        // reads P1, P1:R2:R3
    cash -= next->next->money;  // reads P1, P1:R2, P1:R2:R2:R3
    money = cash + 4 + cash;    // writes P1:R3
  }
  __attribute__((region("R3")))
  void add_money(int cash) {
    money += cash; // expected-warning {{effect not covered by effect summary}}
  }
  void subtract_money(int cash) __attribute__((reads_effect("P1:R3"))) { // expected-warning {{RPL element was not declared}}  
    money -= (cash) ? cash+3 : cash=3;  // expected-warning{{effect not covered by effect summary}}
    money -= cash;                      // expected-warning{{effect not covered by effect summary}}
  }

  bool insured __attribute__((in_region("P1:R3")));// expected-warning {{RPL element was not declared}} 
  char* name __attribute__((in_region("P1")));
};

