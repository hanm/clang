//=== ASaPGenericStmtVisitor.cpp - Safe Parallelism checker -*- C++ -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This file defines the ASaPStmtVisitor template classe used by the Safe
// Parallelism checker, which tries to prove the safety of parallelism
// given region and effect annotations.
//
// ASaPStmtVisitor extends clang::StmtVisitor and it includes common
// functionality shared by the ASaP passes that visit statements,
// including TypeBuilder, AssignmentChecker, EffectCollector, and more.
//
//===----------------------------------------------------------------===//
#include "clang/AST/Decl.h"

#include "SpecificNIChecker.h"

namespace clang {
namespace asap {

bool SpecificNIChecker::check(CallExpr *E) const {
  return false;
}
//TBBParallelForRangeNIChecker::
//TBBParallelForRangeNIChecker(const VarDecl *Body, const VarDecl *Range) /*:
//  Body(Body), Range(Range)*/ { this->Body = Body; this->Range = Range; }

/*TBBParallelForRangeNIChecker::
TBBParallelForRangeNIChecker(const VarDecl *Body, const VarDecl *Range) :
  TBBParallelForNIChecker(Body), Range(Range) { }*/

// Methods
bool TBBParallelInvokeNIChecker::check(CallExpr *Exp) const {
  // Find Body

  // Compute Effects of Body::operator() (or use its effect summary)

  // Find induction variables

  return true;
}

} // end namespace asap
} // end namespace clang
