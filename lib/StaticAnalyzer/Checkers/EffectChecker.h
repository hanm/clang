//=== EffectChecker.h - Safe Parallelism checker -----*- C++ -*----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This files defines the Effect Checker pass of the Safe Parallelism
// checker, which tries to prove the safety of parallelism given region
// and effect annotations.
//
//===----------------------------------------------------------------===//

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_EFFECT_CHECKER_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_EFFECT_CHECKER_H

#include "clang/AST/ASTContext.h"
#include "clang/AST/StmtVisitor.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugReporter.h"
// FIXME: this header should be removed, and AnalysisManager should be removed from interface.
#include "clang/StaticAnalyzer/Core/PathSensitive/AnalysisManager.h"
#include "llvm/Support/raw_ostream.h"
#include "ASaPSymbolTable.h"
#include "Effect.h"

namespace clang {

class AnalysisDeclContext;
class FunctionDecl;
class Stmt;

namespace asap {

class EffectCollectorVisitor
    : public StmtVisitor<EffectCollectorVisitor> {
  ento::BugReporter &BR;
  ASTContext &Ctx;
  AnalysisDeclContext *AC;
  raw_ostream &OS;
  SymbolTable &SymT;
  const FunctionDecl *Def;
  bool FatalError;
  EffectVector EffectsTmp;
  /// True when visiting an expression that is being written to.
  bool HasWriteSemantics;
  /// True when visiting a base expression (e.g., B in B.f, or B->f).
  bool IsBase;
  /// Count of number of dereferences on expression (values in [-1, 0, ...] ).
  int DerefNum;
  bool IsCoveredBySummary;
  const EffectSummary *EffSummary;

  /// \brief Using Type with DerefNum perform substitution on all TmpEffects.
  void memberSubstitute(const ValueDecl *D);
  /// \brief Adds effects to TmpEffects and returns the number of effects added.
  int collectEffects(const ValueDecl *D);
  /// \brief Issues Warning: '<str>' <bugName> on Declaration.
  void helperEmitDeclarationWarning(const Decl *D,
                                    const StringRef &Str,
                                    std::string BugName,
                                    bool AddQuotes = true);
  void helperEmitEffectNotCoveredWarning(const Stmt *S,
                                         const Decl *D,
                                         const StringRef &Str);
  /// \brief Copy the effect summary of FunD and push it to the TmpEffects.
  int copyAndPushFunctionEffects(FunctionDecl *FunD);
  /// \brief Check that the 'N' last effects are covered by the summary.
  bool checkEffectCoverage(const Expr *Exp, const Decl *D, int N);

  void helperVisitAssignment(BinaryOperator *E);
  void helperEmitUnsupportedConstructorInitializer(const CXXConstructorDecl *D);
  void helperVisitCXXConstructorDecl(const CXXConstructorDecl *D);

public:
  /// Constructor
  EffectCollectorVisitor (
    ento::BugReporter &BR,
    ASTContext &Ctx,
    ento::AnalysisManager &Mgr,
    AnalysisDeclContext *AC,
    raw_ostream &OS,
    SymbolTable &SymT,
    const FunctionDecl* Def,
    Stmt *S
    );

  /// Getters
  inline bool getIsCoveredBySummary() { return IsCoveredBySummary; }
  inline bool encounteredFatalError() { return FatalError; }

  void VisitChildren(Stmt *S);
  void VisitStmt(Stmt *S);
  void VisitMemberExpr(MemberExpr *Exp);
  void VisitUnaryAddrOf(UnaryOperator *E);
  void VisitUnaryDeref(UnaryOperator *E);
  void VisitPrePostIncDec(UnaryOperator *E);
  void VisitUnaryPostInc(UnaryOperator *E);
  void VisitUnaryPostDec(UnaryOperator *E);
  void VisitUnaryPreInc(UnaryOperator *E);
  void VisitUnaryPreDec(UnaryOperator *E);
  void VisitReturnStmt(ReturnStmt *Ret);
  void VisitDeclRefExpr(DeclRefExpr *Exp);
  void VisitCXXThisExpr(CXXThisExpr *E);
  void VisitCompoundAssignOperator(CompoundAssignOperator *E);
  void VisitBinAssign(BinaryOperator *E);
  void VisitCallExpr(CallExpr *E);
  /// \brief Visit non-static C++ member function call.
  void VisitCXXMemberCallExpr(CXXMemberCallExpr *Exp);
  /// \brief Visits a C++ overloaded operator call where the operator
  /// is implemented as a non-static member function.
  void VisitCXXOperatorCallExpr(CXXOperatorCallExpr *Exp);
}; // End class StmtVisitor.
} // End namespace asap.
} // End namespace clang.

#endif
