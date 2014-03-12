// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=inference %s -verify
// expected-no-diagnostics

//#include <stdio.h>
//#include <stdlib.h>
#define NULL 0
int atoi(char*);

//#include "tbb/task_scheduler_init.h"
//#include "tbb/parallel_invoke.h"
//#include "tbb/tick_count.h"
int printf ( const char * format, ... );

#include "parallel_invoke_fake.h"

#ifndef TIMES
#define TIMES 1
#endif

class TreeNode;
[[asap::region("ReadOnly")]];

class [[asap::param("P_gtl")]] GrowTreeLeft {
  TreeNode *Node [[asap::arg("ReadOnly, P_gtl")]];
  int Depth [[asap::arg("ReadOnly")]];

public:
  GrowTreeLeft(TreeNode *n[[asap::arg("P_gtl")]], int depth) : Node(n), Depth(depth) {}

  void operator() [[asap::reads("ReadOnly, P_gtl:TreeNode::V"), 
                    asap::writes("P_gtl:TreeNode::L:*")]]
                  () const;

}; // end class GrowTreeLeft

class [[asap::param("P_gtr")]] GrowTreeRight {
  TreeNode *Node [[asap::arg("ReadOnly, P_gtr")]];
  int Depth [[asap::arg("ReadOnly")]];

public:
  GrowTreeRight(TreeNode *n[[asap::arg("P_gtr")]], int depth) : Node(n), Depth(depth) {}

  void operator() [[asap::reads("ReadOnly, P_gtr:TreeNode::V"), 
                    asap::writes("P_gtr:TreeNode::R:*")]]
                  () const;

}; // end class GrowTreeRight

class [[asap::param("P"), asap::region("L, R, V, Links")]] TreeNode {
  friend class GrowTreeLeft;
  friend class GrowTreeRight;

  TreeNode *left [[asap::arg("P:L, P:L")]];  // left child
  TreeNode *right [[asap::arg("P:R, P:R")]]; // right child
  int value [[asap::arg("P:V")]];

public:
  TreeNode(int v = 0) : left(NULL), right(NULL), value(v) {}

  void addLeftChild [[asap::writes("P:L")]] 
                    (TreeNode *N [[asap::arg("P:L")]]) {
    left = N;
  }

  void addRightChild [[asap::writes("P:R")]]
                     (TreeNode *N [[asap::arg("P:R")]]) {
    right = N;
  }

  void growTree [[asap::reads("ReadOnly"), 
                  asap::writes("P:*")]]
                (int depth) {
    if (depth<=0) return;
#ifdef SEQUENTIAL
    // INVARIANT: depth >= 1
    if (left==NULL) {
      left = new TreeNode(value+1);
      left->growTree(depth-1);
    }
    if (right==NULL) {
      int newValue = value + (1<<(depth));
      right = new TreeNode(newValue);
      right->growTree(depth-1);
    }
#else
    GrowTreeLeft Left [[asap::arg("P")]] (this, depth);
    GrowTreeRight Right [[asap::arg("P")]] (this, depth);
    tbb::parallel_invoke(Left,   // reads ReadOnly, P:V, writes P:L:*
                         Right); // reads ReadOnly, P:V  writes P:R:*
#endif      
  }

  void printTree [[asap::reads("P:*")]] () {
    printf("%d, ", value);
    if (left)
      left->printTree();
    if (right)
      right->printTree();
  }

}; // end class TreeNode

void GrowTreeLeft::operator() () const {
  if (Node->left==NULL) {
    Node->left = new TreeNode(Node->value+1);
    Node->left->growTree(Depth-1);
  }
}

void GrowTreeRight::operator() () const {
  if (Node->right==NULL) {
    int newValue = Node->value + (1<<(Depth));
    Node->right = new TreeNode(newValue);
    Node->right->growTree(Depth-1);
  }
}

int main [[asap::region("MAIN"), asap::reads("ReadOnly"), asap::writes("MAIN:*")]]
         (int argc, char *argv [[asap::arg("Local, Local")]] [])
{
    int nth=-1; // number of (hardware) threads (-1 means undefined)
    if (argc > 1) { //command line with parameters
        if (argc > 2) {
            printf("ERROR: wrong use of command line arguments. Usage %s <#threads>\n", argv[0]);
            return 1;
        } else {
            nth = atoi( argv[1] );
        }
    }
    int defth = tbb::task_scheduler_init::default_num_threads();
    if (nth<0) nth=defth;
    printf("Default #Threads=%d. Using %d threads\n", defth, nth);
    //tbb::task_scheduler_init init(nth);


    int j,k;
    //tbb::tick_count t0 = tbb::tick_count::now();

    long result = -1;
    TreeNode *Tree [[asap::arg("MAIN:*")]] = new TreeNode(0);
    Tree->growTree(30);
    //Tree->printTree();
    printf("\n");

    //tbb::tick_count t1 = tbb::tick_count::now();
    //printf("Ticks = %f\n", (t1-t0).seconds());

    return 0;
}
