// RUN: %clang_cc1 -DASP_GNU_SYNTAX %s -verify
// RUN: %clang_cc1 -DASP_CXX11_SYNTAX -std=c++11 %s -verify

#ifdef ASP_GNU_SYNTAX
class
__attribute__((region("Money")))
__attribute__ ((region_param("P"))) 
__attribute__((in_region("P:Money"))) // expected-warning {{attribute only applies to fields}}
Coo {

  int money __attribute__((in_region("P:Money")))
            __attribute__ ((region_param("P")))  // expected-warning {{attribute only applies to classes and structs and functions}}
            __attribute__((region("Honey"))); // expected-warning {{attribute only applies to classes and structs and functions and globals}}

public:
  Coo () __attribute__((pure_effect)) 
         __attribute__((region("Money")))   
         __attribute__((in_region("P:Money"))) // expected-warning {{attribute only applies to fields}}
         __attribute__ ((region_param("P"))) 
        : money(0) {}

  Coo (int cash __attribute__((in_region("P:Money"))) // expected-warning {{attribute only applies to fields}}
                __attribute__((region("Money"))) )  // expected-warning {{attribute only applies to classes and structs and functions and globals}}
       __attribute__((pure_effect)) : money(cash) {}

  int get_some() __attribute__((reads_effect("P:Money"))){ 
    return money;
  }

  void set_money(int cash __attribute__ ((region_param("P"))) )  // expected-warning {{attribute only applies to classes and structs and functions}} 
                          __attribute__((writes_effect("P:Money"))) {
    money = cash ;
  }

  void add_money(int cash) __attribute__((atomic_writes_effect("P:Money"))) {
    money += cash ;
  }
};



__attribute__((region("Roo"))) 
int main (void) {
  Coo __attribute__((region_arg("Roo"))) c;
  Coo __attribute__((region_arg("Rah"))) *d = new Coo __attribute__((region_arg("Root:Rah"))) (77);
  int five __attribute__((region_arg("Boo"))) = 4+1;
  int six  = 4+2;
  return 0;
}

#endif

#ifdef ASP_CXX11_SYNTAX
class
[[asp::region("Money")]]
[[asp::region_param("P")]]
[[asp::in_region("P:Money")]] // expected-warning {{attribute only applies to fields}}
Coo {

  int money [[asp::in_region("P:Money")]]
            [[asp::region_param("P")]]  // expected-warning {{attribute only applies to classes and structs and functions}}
            [[asp::region("Honey")]]; // expected-warning {{attribute only applies to classes and structs and functions and globals}}

public:
  Coo () [[asp::pure_effect]] 
         [[asp::region("Money")]]   
         [[asp::in_region("P:Money")]] // expected-warning {{attribute only applies to fields}}
         [[asp::region_param("P")]] 
        : money(0) {}

  Coo (int cash [[asp::in_region("P:Money")]] // expected-warning {{attribute only applies to fields}}
                [[asp::region("Money")]] )  // expected-warning {{attribute only applies to classes and structs and functions and globals}}
       [[asp::pure_effect]] : money(cash) {}

  int get_some() [[asp::reads_effect("P:Money")]]{ 
    return money;
  }

  void set_money(int cash [[asp::region_param("P")]] )  // expected-warning {{attribute only applies to classes and structs and functions}} 
                          [[asp::writes_effect("P:Money")]] {
    money = cash ;
  }

  void add_money(int cash) [[asp::atomic_writes_effect("P:Money")]] {
    money += cash ;
  }
};



[[asp::region("Roo")]] 
int main (void) {
  Coo [[asp::region_arg("Roo")]] c;
  Coo [[asp::region_arg("Rah")]] *d = new Coo [[asp::region_arg("Root:Rah")]] (77);
  int five [[asp::region_arg("Boo")]] = 4+1;
  int six  = 4+2;
  return 0;
}

#endif
  
