// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//
// expected-no-diagnostics
//    X
//  /   \
// A1   A2
//  \   /
//    B
//    |
//    C

[[asap::region("X, A1, A2, B, C")]];


class [[asap::param("ClassX")]] X {
    int x [[asap::arg("ClassX")]];
public:
    void setX [[asap::writes("ClassX")]] (int _x) { x = _x; };
};

class [[asap::param("ClassA1"), asap::base_arg("X", "ClassA1:X")]] A1 : public X {
    int a [[asap::arg("ClassA1")]];
public:
    void setA [[asap::writes("ClassA1")]] (int _a) { a = _a; };
    void set [[asap::writes("ClassA1, ClassA1:X")]] (int _x, int _a) {
      setX(_x);
      a = _a;
    }
};

class [[asap::param("ClassA2"), asap::base_arg("X", "ClassA2:X")]] A2 : public X {
    int a [[asap::arg("ClassA2")]];
public:
    void setA [[asap::writes("ClassA2")]] (int _a) { a = _a; };
    void set [[asap::writes("ClassA2, ClassA2:X")]] (int _x, int _a) { 
      setX(_x);
      a = _a; 
    };
};

class [[asap::param("ClassB"),
        asap::base_arg("A1", "ClassB:A1"),
        asap::base_arg("A2", "ClassB:A2")]] B : public A1, public A2 {

    int b [[asap::arg("ClassB")]];
public:
    void setB [[asap::writes("ClassB")]] (int _b) { b = _b; };
    void set [[asap::writes("ClassB:*")]] (int x1, int x2, int a1, int a2, int _b)
      { A1::set(x1, a1); 
        A2::set(x2, a2);
        b = _b;
      }
};

class [[asap::param("ClassC"),
        asap::base_arg("B", "ClassC")]] C : public B {

    int c [[asap::arg("ClassC")]];
public:
    inline void setC [[asap::writes("ClassC")]] (int _c) { c = _c; };
    void set [[asap::writes("ClassC:*")]] (int x1, int x2, int a1, int a2, int b, int _c) {
      B::set(x1, x2, a1, a2, b);
      setC(_c);
    }
    void set_v2 [[asap::writes("ClassC:*")]] (int _a1, int _a2, int _b, int _c) {
      B::A1::setA(_a1);
      B::A2::setA( _a2);
      B::setB(_b);
      setC(_c);
    }
};

