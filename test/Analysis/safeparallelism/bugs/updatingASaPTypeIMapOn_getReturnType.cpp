// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
// 
// expected-no-diagnostics

class A {
  int x;
};

class B : public A {
  int y;
public:
  B *getSelf() {
    return this;
  }
};

void foo() {
  B b;
  B* bp = b.getSelf();
  A* ap1 = bp;
  A* ap2 = b.getSelf(); // ASaPType::getReturnType is called here which should 
                        // properly update the Inheritance map of the return 
                        // type.
}
