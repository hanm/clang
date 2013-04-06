// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//
[[asap::region("Name")]];
[[asap::region("Name")]]; // expected-warning{{region name already declared at this scope}}
[[asap::region("Local")]]; // expected-warning{{invalid region name}}
[[asap::region("Global")]]; // expected-warning{{invalid region name}}
[[asap::region("Immutable")]]; // expected-warning{{invalid region name}}
[[asap::region("Root")]]; // expected-warning{{invalid region name}}
[[asap::region("*")]]; // expected-warning{{invalid region name}}

class [[asap::param("Local")]] A { }; // expected-warning{{invalid region parameter name}}
class [[asap::param("Global")]] B { }; // expected-warning{{invalid region parameter name}}
class [[asap::param("Immutable")]] c { }; // expected-warning{{invalid region parameter name}}
class [[asap::param("Root")]] D { }; // expected-warning{{invalid region parameter name}}
class [[asap::param("*")]] E { }; // expected-warning{{invalid region parameter name}}
class [[asap::param("P, P")]] F { }; // expected-warning{{region parameter already declared at this scope}}

