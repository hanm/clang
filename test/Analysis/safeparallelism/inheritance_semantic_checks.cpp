// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//

class [[asap::param("ClassB")]] base {
  int x [[asap::arg("ClassB")]];
public:
  void do_something [[asap::reads("ClassB")]] () {};
};

// A. SINGLE INHERITANCE
// A.1 missing, wrong and duplicate base_arg annotations
class [[asap::param("Class")]] derived1 : public base { 
};

class [[asap::param("Class"), asap::base_arg("base", "Class")]] derived2
: public base {
};

class [[asap::param("Class"),
        asap::base_arg("case", "Class"), // expected-warning{{first argument must refer to direct base class}}
        asap::base_arg("base", "Class")]]
derived3 : public base {
};

class [[asap::base_arg("case", "Class")]]   // expected-warning{{first argument must refer to direct base class}} 
derived4 : public base { // expected-warning{{missing base_arg attribute}}
};

class [[asap::param("Class"),
        asap::base_arg("base", "Class"),
        asap::base_arg("base", "Class")]] // expected-warning{{duplicate attribute for single base class specifier}}
derived5 : public base {
};

// A.2 Wrong number of RPLs to base_arg attribute
class [[asap::param("Class"),
        asap::base_arg("base", "Class, Class")]] // expected-warning{{superfluous region argument(s)}}
derived6 : public base {
};

class [[asap::param("Class"),
        asap::base_arg("base", "")]] // expected-warning{{the empty string is not a valid RPL}} 
derived7 : public base {
};

// B. MULTIPLE INHERITANCE
class [[asap::param("ClassB2")]] base2 {
  int y [[asap::arg("ClassB2")]];
public:
  void do_something [[asap::reads("ClassB2")]] () {};
};

class [[asap::param("Class"),
        asap::base_arg("base", "Class"),
        asap::base_arg("base2", "Class")]]
derived8 : public base, public base2 {
};

class [[asap::param("Class"),
        asap::base_arg("base", "Class"),
        asap::base_arg("base2", "Class"),
        asap::base_arg("base3", "Class")]] // expected-warning{{first argument must refer to direct base class}}
derived9 : public base, public base2 {
};

class [[asap::param("Class"), asap::region("R"),
        asap::base_arg("base", "Class"),
        asap::base_arg("base2", "Class"),
        asap::base_arg("base2", "R")]] // expected-warning{{duplicate attribute for single base class specifier}}
derived10 : public base, public base2 {
};

