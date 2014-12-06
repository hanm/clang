// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=param %s -verify
// expected-no-diagnostics
//

class [[asap::region("Rc0"), asap::param("R")]]
      OctTreeNode {
public:
  int type [[asap::arg("R")]]; // CELL or BODY

  OctTreeNode() : type(0) {}

  OctTreeNode *child0 [[asap::arg("R:Rc0, R:Rc0")]];

  void m() {
    OctTreeNode **childi [[asap::arg("Local, Root:*, Root:*")]] = 0;
    childi = &child0;
  }
}; // end class OctTreeNode

