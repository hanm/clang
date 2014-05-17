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

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_GENERIC_STMT_VISITOR_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_GENERIC_STMT_VISITOR_H

#include "clang/AST/StmtVisitor.h"
#include "clang/AST/ASTContext.h"

#include "ASaPSymbolTable.h"
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
  const CheckerBase *Checker;
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
  explicit ASaPStmtVisitor(const FunctionDecl *Def)
      : Checker(SymbolTable::VB.Checker),
        BR(*SymbolTable::VB.BR),
        Ctx(*SymbolTable::VB.Ctx),
        Mgr(*SymbolTable::VB.Mgr),
        AC(SymbolTable::VB.AC),
        OS(*SymbolTable::VB.OS),
        SymT(*SymbolTable::Table),
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
    OS << "DEBUG:: GENERIC:: Visiting Stmt/Expr = ";
    S->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    VisitChildren(S);
  }

}; // End class ASaPStmtVisitor.


} // end namespace asap
} // end namespace clang

#endif
