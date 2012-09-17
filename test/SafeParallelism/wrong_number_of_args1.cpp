// RUN: %clang_cc1 %s -verify

// #include <iostream>
//using namespace std;

class 
__attribute__ ((region_param)) // expected-error {{attribute takes one argument}}
__attribute__((region)) // expected-error {{attribute takes one argument}}
	Coo {
    int money __attribute__((in_region)); // expected-error {{attribute takes one argument}}

public:
    Coo (): money(70) {}

    int get_some() __attribute__ ((pure_effect)){ 
        return money;
    }

    void set_money(int cash) __attribute__((writes_effect)) { // expected-error {{attribute takes one argument}}
        money = cash;
    }
};

class 
__attribute__ ((region_param("P", "P2")))  // expected-error {{attribute takes one argument}}
__attribute__((region("Links", "Butter"))) // expected-error {{attribute takes one argument}}
	Gar {
    int money __attribute__((in_region("P:Links", "Links:Butter"))); // expected-error {{attribute takes one argument}}

public:
    Gar (): money(70) {}

    int get_some() __attribute__ ((pure_effect("Butter"))){  // expected-error {{attribute takes no argument}} 
        return money;
    }

    void set_money(int cash) __attribute__((writes_effect("P:Links", "Links:Butter"))) { // expected-error {{attribute takes one argument}}
        money = cash;
    }
};



__attribute__((region("Roo")))  
__attribute__((region("Raa")))
int main (void) {
    Coo __attribute__((region_arg)) c; // expected-error {{attribute takes one argument}}
    Gar __attribute__((region_arg("Roo", "Raa"))) g; // expected-error {{attribute takes one argument}}
    g.set_money(17);
    //cout << "Got " << c.get_some() << " dollyz and " << g.get_some() << ")\n";
    return 0; 
}
  
