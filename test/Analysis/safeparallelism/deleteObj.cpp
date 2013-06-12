// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//

class [[asap::param("P"), asap::region("R,L,D,Ptr")]] TreeNode {
  TreeNode *left  [[asap::arg("P:Ptr, P:L")]];
  TreeNode *right [[asap::arg("P:Ptr, P:R")]];
  double data [[asap::arg("P:D")]];
public:
  ~TreeNode() /*[[asap::writes("P:Ptr, P:D")]]*/ 
  { }
}; // end class TreeNode

[[asap::region("R1, R2")]];
class scoped_delete {
public:
  double *ptr [[asap::arg("R1, R2")]];

  void func [[asap::reads("R1")]] () {
    delete ptr; // reads R1 
  }
  void funcErr () {
    delete ptr; // expected-warning{{effect not covered}}
  }
};

