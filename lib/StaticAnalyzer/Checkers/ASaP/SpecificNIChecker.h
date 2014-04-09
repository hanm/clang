//=== ASaPGenericStmtVisitor.h - Safe Parallelism checker -*- C++ -*-===//
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

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_SPECIFIC_NI_CHECKER_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_SPECIFIC_NI_CHECKER_H


#include "ASaPFwdDecl.h"

namespace clang {
namespace asap {

/// \brief abstract base class of all NIChecker classes
class SpecificNIChecker {
public:
  virtual ~SpecificNIChecker() {};
  virtual bool check(CallExpr *E, const FunctionDecl *Def) const = 0;
}; // end class SpecificNIChecker

/// \brief abstract base class of all TBB related NIChecker classes
class TBBSpecificNIChecker : public SpecificNIChecker {
public:
  virtual bool check(CallExpr *E, const FunctionDecl *Def) const;
}; // end class TBBSpecificNIChecker

/// \brief base class for all TBB parallel_for related NIChecker classes
class TBBParallelForNIChecker : public TBBSpecificNIChecker {
}; // end class TBBParallelForChecker

/// \brief class for checking TBB parallel_for with Range iterator
class TBBParallelForRangeNIChecker : public TBBParallelForNIChecker {
public:
  virtual bool check(CallExpr *E, const FunctionDecl *Def) const;
}; // end class TBBParallelForRangeChecker

/// \brief class for checking TBB parallel_for with Iterator
class TBBParallelForIndexNIChecker : public TBBParallelForNIChecker {
public:
  virtual bool check(CallExpr *E, const FunctionDecl *Def) const;
}; // end class TBBParallelForRangeChecker

/// \brief class for checking TBB parallel_invoke
class TBBParallelInvokeNIChecker : public TBBSpecificNIChecker {
public:
  virtual bool check(CallExpr *E, const FunctionDecl *Def) const;
}; // end class TBBParallelForRangeChecker


} // end namespace asap
} // end namespace clang

#endif

