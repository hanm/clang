// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//

class
[[asap::param("P1"), asap::region("R2, R3")]]
C1 { 
  int money [[asap::arg("P1:R1")]]; // expected-warning {{RPL element was not declared}} 
  int money1 [[asap::arg("P1:R3")]]; 
  int money2 [[asap::arg("R2:P1")]]; // expected-warning {{Misplaced Region Parameter}} 
  int money3 [[asap::arg("R1:P2")]]; // expected-warning {{RPL element was not declared}}  expected-warning {{RPL element was not declared}} 
  C1 *next [[asap::arg("P1, P1:R2")]];

  void subtract_money [[asap::reads("P1:R1")]] (int cash) { // expected-warning {{RPL element was not declared}}
    money -= (cash) ? cash+3 : cash=3;
    money -= cash;
  }

  bool insured [[asap::arg("P1:R1")]]; // expected-warning {{RPL element was not declared}}

};
