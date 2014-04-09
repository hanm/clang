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

#include "clang/AST/DeclTemplate.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugReporter.h"

#include "ASaPSymbolTable.h"
#include "NonInterferenceChecker.h"
#include "SpecificNIChecker.h"

namespace clang {
namespace asap {

NonInterferenceChecker::NonInterferenceChecker (
  const FunctionDecl* Def,
  Stmt *S,
  bool VisitCXXInitializer) : BaseClass(Def) {
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
    DeclaratorDecl *DeclD = dyn_cast<DeclaratorDecl>(D);
    assert(DeclD);
    StringRef Name = DeclD->getQualifiedNameAsString();
    OS << "DEBUG:: CalleeDecl(" << D << "). Name = " << Name << "\n";
    D->print(OS, Ctx.getPrintingPolicy());
    OS << "\n";

    D->dump(OS);
    OS << "\n";

    const FunctionDecl *FunD = dyn_cast<FunctionDecl>(D);
    const VarDecl *VarD = dyn_cast<VarDecl>(D);
    assert(VarD || FunD); // The callee should be a function or a fn-pointer
    if (FunD) {
      // if it is a template, we want to get its generic form,
      // not the specialized form at the call-site.
      if(FunD->getTemplatedKind() ==
         FunctionDecl::TK_FunctionTemplateSpecialization) {
        FunD = FunD->getPrimaryTemplate()->getTemplatedDecl();
      }
      const SpecificNIChecker *SNIC = SymT.getNIChecker(FunD);
      if (SNIC) {
        SNIC->check(Exp, Def);
      } // otherwise there's nothing to check
    } else { // VarD
      // TODO
    }
  }
  return;
}

} // end namespace asap
} // end namespace clang
