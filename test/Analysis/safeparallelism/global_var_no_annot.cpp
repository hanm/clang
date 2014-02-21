// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//

int GlobalVar;

// Declare region.
namespace ASaP {
class
Globals {
public:
  // GlobalVar is in region R.
  static int GlobalVar;
  int FieldVar;
};
} // end namespace ASaP


class FooFunctor {
public:
  // function foo writes to region R.
  //void operator () () const {
  void operator () [[asap::writes("Global")]] () const {
    GlobalVar = 1;
    ASaP::Globals::GlobalVar = 1;
  }
}; // end class FooFunctor

class BarFunctor {
public:
  // function bar writes to region R.
  void operator () [[asap::writes("Global")]] () const {
    ASaP::Globals::GlobalVar = 2;
  }
}; // end class BarFunctor

class CallsBarFunctor {
public:
  // function bar writes to region R.
  void operator () [[asap::writes("Global")]] () const {
    BarFunctor bar;
    bar();
  }
}; // end class CallsBarFunctor

class ZooFunctor {
public:
  // funciton zoo reads region R.
  void operator () [[asap::reads("Global")]] () const {
    int x = ASaP::Globals::GlobalVar; 
  }
}; // end class ZooFunctor

class BadFunctor {
public:
  // funciton bad reads region ASaP::R.
  void operator () [[asap::reads("Global")]] () const {
    int x = ASaP::Globals::GlobalVar; 
  }
}; // end class BadFunctor

namespace tbb {
  template<typename Func0, typename Func1>
  void parallel_invoke
    //[[asap::invokes("f0 || f1")]]
    //[[asap::writes("ASaP::R, R")]] // until we support effect polymorphism
    [[asap::writes("Global")]]
    (const Func0 &f0, const Func1 &f1) {
    f0();
    f1();
  }
} // end namespace tbb

int main [[asap::writes("Global")]] () {
    ASaP::Globals::GlobalVar = 0;
    // No warning if they are invoked sequentially
    FooFunctor foo;
    BarFunctor bar;
    CallsBarFunctor callsBar;
    ZooFunctor zoo1, zoo2;
    callsBar();
    foo();
    bar();
    zoo1();
 
    // warning if they are forked as different tasks
    // (we don't support tbb fork syntax yet.)
    tbb::parallel_invoke(foo, bar); // expected-warning{{interfering effects}}
    // no warning here as zoo has ready only effect
    tbb::parallel_invoke(zoo1, zoo2);
    // warning: the effects of Z2 and B2 are interferring 
    // (if we had "joined" Z2 above, it would be safe)
    tbb::parallel_invoke(zoo1, callsBar); // expected-warning{{interfering effects}}

    return 0;
} //  end main

