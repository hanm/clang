//=== Effect.h - Safe Parallelism checker -----*- C++ -*--------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This file defines the Effect and EffectSummary classes used by the Safe
// Parallelism checker, which tries to prove the safety of parallelism
// given region and effect annotations.
//
//===----------------------------------------------------------------===//

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_GENERIC_STMT_VISITOR_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_GENERIC_STMT_VISITOR_H

#include "clang/AST/StmtVisitor.h"
#include "clang/AST/ASTContext.h"

#include "ASaPUtil.h"


namespace clang {
namespace asap {

/// \brief Generic statement visitor that wraps different customized
/// check pass.
template<typename CustomCheckerTy>
class ASaPStmtVisitor
: public StmtVisitor<CustomCheckerTy> {
  //typedef StmtVisitor<ASaPStmtVisitor<CustomCheckerTy> > BaseClass;
  typedef StmtVisitor<CustomCheckerTy> BaseClass;

protected:
  /// Fields
  VisitorBundle &VB;

  BugReporter &BR;
  ASTContext &Ctx;
  AnalysisManager &Mgr;
  AnalysisDeclContext *AC;
  raw_ostream &OS;
  SymbolTable &SymT;

  const FunctionDecl *Def;

  bool FatalError;

public:
  /// Constructor
  explicit ASaPStmtVisitor(
      VisitorBundle &VB,
      const FunctionDecl *Def)
      : VB(VB),
        BR(VB.BR),
        Ctx(VB.Ctx),
        Mgr(VB.Mgr),
        AC(VB.AC),
        OS(VB.OS),
        SymT(VB.SymT),
        Def(Def),
        FatalError(false) {
    OS << "DEBUG:: ******** INVOKING Generic STMT Visitor...\n";
    //Def->print(OS, Ctx.getPrintingPolicy());
    //S->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
  }

  /// Getters
  inline bool encounteredFatalError() { return FatalError; }

  /// Visitors
  void VisitChildren(Stmt *S) {
    for (Stmt::child_iterator I = S->child_begin(), E = S->child_end();
         I!=E; ++I)
      if (Stmt *child = *I) {
        BaseClass::Visit(child);
        //static_cast<CustomCheckerTy*>(this)->Visit(child);
      }
  }

  void VisitStmt(Stmt *S) {
    OS << "DEBUG:: GENERIC:: Visiting Stmt/Expr = \n";
    S->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    VisitChildren(S);
  }

}; // End class ASaPStmtVisitor.


} // end namespace asap
} // end namespace clang

#endif
