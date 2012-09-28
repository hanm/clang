// RUN: %clang_cc1 -DASP_GNU_SYNTAX %s -verify
// RUN: %clang_cc1 -DASP_CXX11_SYNTAX -std=c++11 %s -verify

#ifdef ASP_GNU_SYNTAX
class 
__attribute__((region("Links")))
__attribute__ ((region_param("P"))) 
Coo {
  int money __attribute__((in_region("P:Links")));

public:
  Coo (): money(70) {}

  int get_some() __attribute__ ((pure_effect)){ 
    return money;
  }

  void set_money(int cash) __attribute__((writes_effect("P:Links"))) {
    money = cash;
  }
};


__attribute__((region("Roo"))) 
int main (void) {
  Coo __attribute__((region_arg("Roo"))) c;
  c.set_money(42);
  return 0; 
}
#endif
 
#ifdef ASP_CXX11_SYNTAX
class 
[[asp::region("Links")]]
[[asp::region_param("P")]]
Coo {
  int money [[asp::in_region("P:Links")]];

public:
  Coo (): money(70) {}

  int get_some() [[asp::pure_effect]] { 
    return money;
  }

  void set_money(int cash) [[asp::writes_effect("P:Links")]] {
    money = cash;
  }
};

[[asp::region("Roo")]]
int main (void) {
  Coo [[asp::region_arg("Roo")]] c; 
  c.set_money(42);
  return 0; 
}
#endif
