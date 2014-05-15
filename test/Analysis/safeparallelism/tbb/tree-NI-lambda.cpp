// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=param %s -verify
// XFAIL: *
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

  void growTree [[asap::reads("P:*:V"), asap::writes("P:*:L, P:*:R")]]
                (int depth) {
    if (depth<=0) return;
    tbb::parallel_invoke(
      [this, depth] () [[asap::reads("P:V"), asap::writes("P:L:*")]]{
        if (left==NULL) {
          left = new TreeNode(value+1);
          left->growTree(depth-1);
        }
      },
      [this, depth] [[asap::reads("P:V"), asap::writes("P:R:*")]] () {
        if (right==NULL) {
          int newValue = value + (1<<(depth));
          right = new TreeNode(newValue);
          right->growTree(depth-1);
        }
      });
  }

  void printTree [[asap::reads("P:*")]] () {
    printf("%d, ", value);
    if (left)
      left->printTree();
    if (right)
      right->printTree();
  }

}; // end class TreeNode

int main [[asap::region("MAIN"), asap::writes("MAIN:*")]]
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
    TreeNode *Tree [[asap::arg("MAIN")]] = new TreeNode(0);
    Tree->growTree(30);
    //Tree->printTree();
    printf("\n");

    //tbb::tick_count t1 = tbb::tick_count::now();
    //printf("Ticks = %f\n", (t1-t0).seconds());

    return 0;
}
