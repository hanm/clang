// RUN: %clang_cc1 %s -verify

class 
__attribute__ ((region_param("P1"))) __attribute__ ((region_param("P2"))) // expected-warning {{duplicate attribute not allowed:}}
__attribute__((region("R"))) 
Coo {
  int money __attribute__((in_region("P1:R"))) __attribute__((in_region("P2:R"))); // expected-warning {{duplicate attribute not allowed:}}

public:
  Coo (): money(70) {}

  int get_some() __attribute__ ((pure_effect)){ 
    return money;
  }

  void set_money(int cash) __attribute__((writes_effect("P2:R"))) { 
    money = cash;
  }
};


__attribute__((region("Roo")))  
__attribute__((region("Raa")))
int main (void) {
  Coo __attribute__((region_arg("Roo"))) __attribute__((region_arg("Raa"))) c; // expected-warning {{duplicate attribute not allowed:}}
  c.set_money(17);
  return 0; 
}
  
