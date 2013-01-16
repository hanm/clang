// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify


/// Unbalanced Sorted tree
/// expected-no-diagnostics
class [[asap::param("P"), asap::region("R1")]] 
      [[asap::region("Left"), asap::region("Right")]] 
                                                      TreeNode {

private:
  long Value [[asap::arg("P")]];
  TreeNode *LeftChild [[asap::arg("P"), asap::arg("P:Left")]],
           // TODO make implicit asap::arg("P:LeftChild") and P:RightChild
           *RightChild [[asap::arg("P"), asap::arg("P:Right")]];

public:
  /// Constructor
  TreeNode (long V) : Value(V), LeftChild(0), RightChild(0) {}

  /// \brief returns true if Value added, false if it already existed
  /// TODO move implementation outside declaration. I.e., TreeNode::add...
  bool add(long V) [[asap::writes("P:Left:*")]] [[asap::writes("P:Right:*")]] [[asap::writes("P")]] //[[asap::writes("P:*")]]
  {
    if (V == Value) return false;
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
  
  bool operator +=(long V) [[asap::writes("P:Left:*")]] [[asap::writes("P:Right:*")]] [[asap::writes("P")]]
  {
    return add(V);
  }

};
