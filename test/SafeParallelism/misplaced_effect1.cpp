// RUN: %clang_cc1 -DASP_GNU_SYNTAX %s -verify
// RUN: %clang_cc1 -DASP_CXX11_SYNTAX -std=c++11 %s -verify

#ifdef ASP_GNU_SYNTAX
class
__attribute__((region("Money")))
__attribute__ ((region_param("P"))) 
__attribute__ ((atomic_reads_effect("Lala"))) // expected-warning {{attribute only applies to functions}}
Coo {

  int money __attribute__((in_region("P:Money")));

public:
  Coo () __attribute__((pure_effect)) : money(0) {}

  Coo (int cash) __attribute__((pure_effect)) : money(cash) {}

  int get_some() __attribute__((reads_effect("P:Money"))){ 
    return money;
  }

  void set_money(int cash) __attribute__((writes_effect("P:Money"))) {
    money = cash;
  }

  void add_money(int cash) __attribute__((atomic_writes_effect("P:Money"))) {
    money += cash;
  }
};



__attribute__((region("Roo"))) 
int main (void) {
  Coo __attribute__((region_arg("Roo"))) c __attribute__((pure_effect)); // expected-warning {{attribute only applies to functions}}
  int five __attribute__ ((reads_effect("Roo"))) = 4+1; // expected-warning {{attribute only applies to functions}}
  int six __attribute__ ((pure_effect)) = 4+2; // expected-warning {{attribute only applies to functions}}
  return 0; 
}

#endif

#ifdef ASP_CXX11_SYNTAX
class
[[asp::region("Money")]]
[[asp::region_param("P")]]
[[asp::atomic_reads_effect("Lala")]] // expected-warning {{attribute only applies to functions}}
Coo {

  int money [[asp::in_region("P:Money")]];

public:
  Coo () [[asp::pure_effect]] : money(0) {}

  Coo (int cash) [[asp::pure_effect]] : money(cash) {}

  int get_some() [[asp::reads_effect("P:Money")]] { 
    return money;
  }

  void set_money(int cash) [[asp::writes_effect("P:Money")]] {
    money = cash;
  }

  void add_money(int cash) [[asp::atomic_writes_effect("P:Money")]] {
    money += cash;
  }
};



[[asp::region("Roo")]] 
int main (void) {
  Coo [[asp::region_arg("Roo")]] c [[asp::pure_effect]]; // expected-warning {{attribute only applies to functions}}
  int five [[asp::reads_effect("Roo")]] = 4+1; // expected-warning {{attribute only applies to functions}}
  int six [[asp::pure_effect]] = 4+2; // expected-warning {{attribute only applies to functions}}
  return 0; 
}

#endif
  
