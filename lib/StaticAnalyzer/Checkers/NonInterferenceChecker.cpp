//=== NonInterferenceChecker.cpp - Safe Parallelism checker -*- C++ -*-=//
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

#include "clang/StaticAnalyzer/Core/BugReporter/BugReporter.h"

#include "NonInterferenceChecker.h"

namespace clang {
namespace asap {

NonInterferenceChecker::NonInterferenceChecker (
  VisitorBundle &VB,
  const FunctionDecl* Def,
  Stmt *S,
  bool VisitCXXInitializer) : BaseClass(VB, Def) {
  OS << "DEBUG:: ******** INVOKING NonInterferenceChecker ...\n";

  if (!BR.getSourceManager().isInMainFile(Def->getLocation())) {
    OS << "DEBUG::EffectChecker::Skipping Declaration that is not in main compilation file\n";
    return;
  }

  //Def->print(OS, Ctx.getPrintingPolicy());
  //S->printPretty(OS, 0, Ctx.getPrintingPolicy());
  //OS << "\n";
  // TODO
  Visit(S);
  OS << "DEBUG:: ******** DONE INVOKING NonInterferenceChecker ***\n";
}

void NonInterferenceChecker::VisitCallExpr(CallExpr *Exp) {
  if (Exp->getType()->isDependentType())
    return; // Do not visit if this is dependent type

  OS << "DEBUG:: VisitCallExpr\n";
  if (!isa<CXXPseudoDestructorExpr>(Exp->getCallee())) {
    Decl *D = Exp->getCalleeDecl();
    assert(D);
    OS << "DEBUG:: CaleeDecl(" << D << "):\n";
    D->print(OS, Ctx.getPrintingPolicy());
    OS << "\n";

    D->dump(OS);
    OS << "\n";

    const FunctionDecl *FD = dyn_cast<FunctionDecl>(D);
    const VarDecl *VarD = dyn_cast<VarDecl>(D);
    assert(VarD || FD); // The callee should be a function or a fn-pointer



  }
  return;
}

} // end namespace asap
} // end namespace clang
