// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify


/// Unbalanced Sorted tree
/// expected-no-diagnostics
class [[asap::param("P"), asap::region("R1, Left, Right")]] 
                                                      TreeNode {

private:
  long Value [[asap::arg("P")]];
  TreeNode *LeftChild [[asap::arg("P, P:Left")]],
           // TODO make implicit asap::arg("P:LeftChild") and P:RightChild
           *RightChild [[asap::arg("P, P:Right")]];

public:
  /// Constructor
  TreeNode (long V) : Value(V), LeftChild(0), RightChild(0) {}

  /// \brief returns true if Value added, false if it already existed
  /// TODO move implementation outside declaration. I.e., TreeNode::add...
  bool add [[asap::writes("P, P:Right:*, P:Left:*")]] (long V) 
  {
    if (V == Value) 
      return false;
    else if (V < Value) 
      if (LeftChild) 
        return LeftChild->add(V);
      else {
        LeftChild = new TreeNode(V);
        return true;
      }
    else // (V > Value)
      if (RightChild)
        return RightChild->add(V);
      else {
        RightChild = new TreeNode(V);
        return true;
      }
  }
  
  bool operator += [[asap::writes("P, P:Right:*, P:Left:*")]] 
    (long V)
  {
    return add(V);
  }

};
