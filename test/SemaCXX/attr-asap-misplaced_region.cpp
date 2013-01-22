// RUN: %clang_cc1 -DASAP_GNU_SYNTAX %s -verify

// FIXME: disable cxx11 test until the main trunk is patched.
//  %clang_cc1 -DASAP_CXX11_SYNTAX -std=c++11 %s -verify

#ifdef ASAP_GNU_SYNTAX
class
__attribute__((region("Money")))
__attribute__ ((param("P"))) 
__attribute__((arg("P:Money"))) // expected-warning {{attribute only applies to fields}}
Coo {

  int money __attribute__((arg("P:Money")))
            __attribute__ ((param("P")))      // expected-warning {{attribute only applies to classes and structs and functions}}
            __attribute__((region("Honey"))); // expected-warning {{attribute only applies to classes and structs and functions and namespaces}}

public:
  Coo () __attribute__((no_effect)) 
         __attribute__((region("Money"))) 
         __attribute__((arg("P:Money"))) 
         __attribute__((arg("P:Money"))) 
         __attribute__ ((param("P"))) 
        : money(0) {}

  Coo (int cash __attribute__((arg("P:Money"))) 
                __attribute__((region("Money"))) )  // expected-warning {{attribute only applies to classes and structs and functions and namespaces}}
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
  Coo c __attribute__((arg("Roo")));
  Coo *d __attribute__((arg("Rah"))) = new Coo __attribute__((arg("Root:Rah"))) (77);
  int five __attribute__((arg("Boo"))) = 4+1;
  int six  = 4+2;
  return 0;
}

#endif

#ifdef ASAP_CXX11_SYNTAX
class
[[asap::region("Money")]]
[[asap::param("P")]]
[[asap::arg("P:Money")]] // expected-warning {{attribute only applies to fields}}
Coo {

  int money [[asap::arg("P:Money")]]
            [[asap::param("P")]]        // expected-warning {{attribute only applies to classes and structs and functions}}
            [[asap::region("Honey")]] ; // expected-warning {{attribute only applies to classes and structs and functions and namespaces}}

public:
  Coo () [[asap::no_effect]] 
         [[asap::region("Money")]]   
         [[asap::arg("P:Money")]] 
         [[asap::arg("P:Money")]] 
         [[asap::param("P")]] 
        : money(0) {}

  Coo (int cash [[asap::arg("P:Money")]] 
                [[asap::region("Money")]] )  // expected-warning {{attribute only applies to classes and structs and functions and namespaces}}
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
  Coo c [[asap::arg("Roo")]];
  // Coo *d [[asap::arg("Rah")]] = new Coo [[asap::arg("Root:Rah")]] (77);
  int five [[asap::arg("Boo")]] = 4+1;
  int six  = 4+2;
  return 0;
}

#endif
  
