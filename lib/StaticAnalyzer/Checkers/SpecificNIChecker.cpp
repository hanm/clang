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
#include "clang/AST/Expr.h"

#include "ASaPSymbolTable.h"
#include "SpecificNIChecker.h"

namespace clang {
namespace asap {

void emitNICheckNotImplemented(const Stmt *S, const FunctionDecl *FunD) {
  StringRef BugName = "Non-interference check not implemented";
  std::string Name;
  if (FunD)
    Name = FunD->getNameInfo().getAsString();
  else
    Name = "";
  StringRef Str(Name);
  helperEmitStatementWarning(*SymbolTable::VB.BR,
                             SymbolTable::VB.AC,
                             S, FunD, Str, BugName, false);

}

bool TBBSpecificNIChecker::check(CallExpr *E) const {
  emitNICheckNotImplemented(E, 0);
  return false;
}

bool TBBParallelInvokeNIChecker::check(CallExpr *Exp) const {
  // TODO
  return true;
}

} // end namespace asap
} // end namespace clang
