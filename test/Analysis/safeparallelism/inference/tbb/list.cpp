// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=inference %s -verify
// expected-no-diagnostics

#include "parallel_invoke_fake.h"

class ListNode;

class [[asap::param("P_t")]] 
      SetThisFunctor {
  ListNode *n [[asap::arg("P_t, P_t")]];
  int v [[asap::arg("P_t")]];
public:
  SetThisFunctor(ListNode *n [[asap::arg("P_t")]], int v) : n(n), v(v) {}

  void operator () [[asap::reads("P_t"), asap::writes("P_t:ListNode::Value")]] 
                   () const;
}; // end class SetThisFunctor

class [[asap::param("P_r")]]
      SetRestFunctor {
  ListNode *n [[asap::arg("P_r, P_r")]];
  int v [[asap::arg("P_r")]];
public:
  SetRestFunctor(ListNode *n [[asap::arg("P_r")]], int v) : n(n), v(v) {}
  void operator () [[asap::reads("P_r, P_r:*:ListNode::Next, P_r:*:ListNode::Link")
                     asap::writes("P_r:ListNode::Next:*:ListNode::Value")]]
                   () const;
}; // end class SetRestFunctor

/////////////////////////////////////////////////////////////////////////////
class [[asap::param("P"), asap::region("Link, Next, Value")]] 
      ListNode {
  friend class SetThisFunctor;
  friend class SetRestFunctor;

  int value [[asap::arg("P:Value")]];
  ListNode *next [[asap::arg("P:Link, P:Next")]];
public:
  void setAllTo [[asap::reads("P, P:*:Next, P:*:Link"), 
                  asap::writes("P:*:Value")]] (int x) {
    SetThisFunctor SetThis [[asap::arg("P")]] (this, x);
    SetRestFunctor SetRest [[asap::arg("P")]] (this, x);
    tbb::parallel_invoke(SetThis, SetRest);
  }
}; // end class List<region P>

void SetThisFunctor::operator () () const {
  n->value = v;
}

void SetRestFunctor::operator () () const {
  if (n->next)
    n->next->setAllTo(v);
}
