// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//
// expected-no-diagnostics

// A1   A2
//  \   /
//    B
//    |
//    C

[[asap::region("A1, A2, B, C")]];

class [[asap::param("ClassA1")]] A1 {
    int a [[asap::arg("ClassA1")]];
public:
    void setA [[asap::writes("ClassA1")]] (int _a) { a = _a; };
};

class [[asap::param("ClassA2")]] A2 {
    int a [[asap::arg("ClassA2")]];
public:
    void setA [[asap::writes("ClassA2")]] (int _a) { a = _a; };
};

class [[asap::param("ClassB"),
        asap::base_arg("A1", "ClassB:A1"),
        asap::base_arg("A2", "ClassB:A2")]] B : public A1, public A2 {

    int b [[asap::arg("ClassB")]];
public:
    void setB [[asap::writes("ClassB")]] (int _b) { b = _b; };
    void set [[asap::writes("ClassB, ClassB:A1, ClassB:A2")]] (int _a1, int _a2, int _b)
      { A1::setA(_a1); 
        A2::setA(_a2);
        b = _b;
      }
};

class [[asap::param("ClassC"),
        asap::base_arg("B", "ClassC")]] C : public B {

    int c [[asap::arg("ClassC")]];
public:
    inline void setC [[asap::writes("ClassC")]] (int _c) { c = _c; };
    void set [[asap::writes("ClassC:*")]] (int _a1, int _a2, int _b, int _c) {
      B::set(_a1, _a2, _b);
      setC(_c);
    }
    void set_v2 [[asap::writes("ClassC:*")]] (int _a1, int _a2, int _b, int _c) {
      B::A1::setA(_a1);
      B::A2::setA( _a2);
      B::setB(_b);
      setC(_c);
    }
};

