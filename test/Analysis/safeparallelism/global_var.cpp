// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
// expected-no-diagnostics

// Declare region.
namespace ASaP /*[[asap::arg("Roo")]]*/ {
//namespace [[asap::arg("Roo")]] ASaP {

class 
[[asap::region("R")]]
Globals {
public:
  // GlobalVar is in region R.
  static int GlobalVar [[asap::arg("R")]];
  int FieldVar [[asap::arg("R")]];
};
 
// function foo writes to region R.
void foo [[asap::writes("Globals::R")]] () {
    Globals::GlobalVar = 1;
}
 
// function bar writes to region R.
void bar [[asap::writes("R")]] () {
    Globals::GlobalVar = 2;
}
 
// function bar writes to region R.
void callsBar [[asap::writes("R")]] () {
    bar();
}
 
// funciton zoo reads region R.
void zoo [[asap::reads("R")]] () {
    int x = Globals::GlobalVar;
}

class Future {
private:
  void (*fun)();
public:
  Future(void (*fun)() ) : fun(fun) {}
  ~Future() { join(); }
  void fork() {};
  void join() {};
}; //end class Future


int main() {
    Globals::GlobalVar = 0;
    // No warning if they are invoked sequentially
    foo();
    bar();
    zoo();
 
    // warning if they are forked as different tasks
    // (we don't support tbb fork syntax yet.)
    Future F(&foo);
    F.fork();
    Future B(bar);
    B.fork();
    F.join();
    B.join();
    // no warning here as zoo has ready only effect
    Future Z1(zoo);
    Z1.fork();
    Future Z2(zoo);
    Z2.fork();
    Z1.join();
    // warning: the effects of Z2 and B2 are interferring 
    // (if we had "joined" Z2 above, it would be safe)
    Future B2(callsBar);
    B2.fork();

    return 0;
} //  end main

} // end namespace ASaP
