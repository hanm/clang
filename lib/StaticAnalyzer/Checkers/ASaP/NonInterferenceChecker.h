//=== NonInterferenceChecker.h - Safe Parallelism checker -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This file defines the Non-Interference Checker pass of the Safe
// Parallelism checker, which tries to prove the safety of parallelism
// given region and effect annotations.
//
// The Non-Interference checker (this pass) makes sure that parallel tasks
// does not interfere with each other. This is the last pass of the ASaP
// checker -- it runs after the type-checker and the effect checker.
//
//===----------------------------------------------------------------===//

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_NON_INTERFERENCE_CHECKER_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_NON_INTERFERENCE_CHECKER_H

#include "clang/AST/StmtVisitor.h"

#include "ASaPFwdDecl.h"
#include "ASaPGenericStmtVisitor.h"



namespace clang {
namespace asap {

class NonInterferenceChecker
    : public ASaPStmtVisitor<NonInterferenceChecker> {
  // Private Types
  typedef ASaPStmtVisitor<NonInterferenceChecker> BaseClass;

public:
  // Constructor
  NonInterferenceChecker (const FunctionDecl* Def, Stmt *S,
                          bool VisitCXXInitializer = false);

  // Visitors
  void VisitCallExpr(CallExpr *E);

}; // End class NonInterferenceChecker.
} // End namespace asap.
} // End namespace clang.

#endif
