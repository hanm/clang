// RUN: %clang_cc1 -DASP_GNU_SYNTAX %s -verify
// RUN: %clang_cc1 -DASP_CXX11_SYNTAX -std=c++11 %s -verify

#ifdef ASP_GNU_SYNTAX
class
__attribute__((region("Money")))
__attribute__ ((param("P"))) 
__attribute__((arg("P:Money"))) // expected-warning {{attribute only applies to fields}}
Coo {

  int __attribute__((arg("P:Money")))
      __attribute__ ((param("P")))  // expected-warning {{attribute only applies to classes and structs and functions}}
      __attribute__((region("Honey"))) money; // expected-warning {{attribute only applies to classes and structs and functions and globals}}

public:
  Coo () __attribute__((no_effect)) 
         __attribute__((region("Money"))) 
         __attribute__((arg("P:Money"))) 
         __attribute__((arg("P:Money"))) 
         __attribute__ ((param("P"))) 
        : money(0) {}

  Coo (int cash __attribute__((arg("P:Money"))) 
                __attribute__((region("Money"))) )  // expected-warning {{attribute only applies to classes and structs and functions and globals}}
       __attribute__((no_effect)) : money(cash) {}

  int get_some() __attribute__((reads("P:Money"))){ 
    return money;
  }

  void set_money(int cash __attribute__ ((param("P"))) )  // expected-warning {{attribute only applies to classes and structs and functions}} 
                          __attribute__((writes("P:Money"))) {
    money = cash ;
  }

  void add_money(int cash) __attribute__((atomic_writes("P:Money"))) {
    money += cash ;
  }
};



__attribute__((region("Roo"))) 
int main (void) {
  Coo __attribute__((arg("Roo"))) c;
  Coo __attribute__((arg("Rah"))) *d = new Coo __attribute__((arg("Root:Rah"))) (77);
  int five __attribute__((arg("Boo"))) = 4+1;
  int six  = 4+2;
  return 0;
}

#endif

#ifdef ASP_CXX11_SYNTAX
class
[[asap::region("Money")]]
[[asap::param("P")]]
[[asap::arg("P:Money")]] // expected-warning {{attribute only applies to fields}}
Coo {

  int [[asap::arg("P:Money")]]
      [[asap::param("P")]]  // expected-warning {{attribute only applies to classes and structs and functions}}
      [[asap::region("Honey")]] money ; // expected-warning {{attribute only applies to classes and structs and functions and globals}}

public:
  Coo () [[asap::no_effect]] 
         [[asap::region("Money")]]   
         [[asap::arg("P:Money")]] 
         [[asap::arg("P:Money")]] 
         [[asap::param("P")]] 
        : money(0) {}

  Coo (int cash [[asap::arg("P:Money")]] 
                [[asap::region("Money")]] )  // expected-warning {{attribute only applies to classes and structs and functions and globals}}
       [[asap::no_effect]] : money(cash) {}

  int get_some() [[asap::reads("P:Money")]]{ 
    return money;
  }

  void set_money(int cash [[asap::param("P")]] )  // expected-warning {{attribute only applies to classes and structs and functions}} 
                          [[asap::writes("P:Money")]] {
    money = cash ;
  }

  void add_money(int cash) [[asap::atomic_writes("P:Money")]] {
    money += cash ;
  }
};



[[asap::region("Roo")]] 
int main (void) {
  Coo [[asap::arg("Roo")]] c;
  Coo [[asap::arg("Rah")]] *d = new Coo [[asap::arg("Root:Rah")]] (77);
  int five [[asap::arg("Boo")]] = 4+1;
  int six  = 4+2;
  return 0;
}

#endif
  
