// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//
// expected-no-diagnostics


class [[asap::param("ClassA")]] A {
    int a [[asap::arg("ClassA")]];
public:
      inline void setA [[asap::writes("ClassA")]] (int _a) { a = _a; };
};

class [[asap::param("ClassB"), asap::base_arg("A", "ClassB")]] B : public A {
    int b [[asap::arg("ClassB")]];
public:
      inline void setB [[asap::writes("ClassB")]] (int _b) { b = _b;};
      void set [[asap::writes("ClassB")]] (int _a, int _b) {
        setA(_a);
        b = _b;
      }
};

class [[asap::param("ClassC"), asap::base_arg("B", "ClassC")]] C : public B {
    int c [[asap::arg("ClassC")]];
public:
      inline void setC [[asap::writes("ClassC")]] (int _c) { c = _c;};
      void set [[asap::writes("ClassC")]] (int _a, int _b, int _c) {
        setA(_a);
        setB(_b);
        c = _c;
      }
};


[[asap::region("R, Ra, Rb, Rc")]];
void func [[asap::writes("R:*")]] (C *x [[asap::arg("Local, R:Rc")]]) {
    A a [[asap::arg("R:Ra")]];
    a.setA(2);
    B b [[asap::arg("R:Rb")]];
    b.setB(3);
    b.set(2,3);

    x->setC(4);
    x->set(2,3,4);
}

