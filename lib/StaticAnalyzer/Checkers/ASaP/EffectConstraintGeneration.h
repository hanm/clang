//=== EffectConstraintGeneration.h - Effect Constraint Generator *- C++ -*===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This file defines the Effect Constraint Generation pass of the Safe
// Parallelism checker, which tries to generate effect constraints.
//
//===----------------------------------------------------------------===//

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_EFFECT_CONSTRAINT_GENERATION_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_EFFECT_CONSTRAINT_GENERATION_H

#include "clang/AST/StmtVisitor.h"

#include "ASaPFwdDecl.h"
#include "ASaPGenericStmtVisitor.h"
#include "EffectInclusionConstraint.h"
#include "Effect.h"

namespace clang {
namespace asap {

class EffectConstraintVisitor
    : public ASaPStmtVisitor<EffectConstraintVisitor> {
  typedef ASaPStmtVisitor<EffectConstraintVisitor> BaseClass;

  const CheckerBase *Checker;
  EffectVector *EffectsTmp;
  EffectInclusionConstraint* EC;

  /// True when visiting an expression that is being written to.
  bool HasWriteSemantics;
  /// True when visiting a base expression (e.g., B in B.f, or B->f).
  bool IsBase;

  int EffectCount;
  /// Count of number of dereferences on expression (values in [-1, 0, ...] ).
  int DerefNum;
  Trivalent IsCoveredBySummary;
  const EffectSummary *EffSummary;

  /// \brief Using Type with DerefNum perform substitution on all TmpEffects.
  void memberSubstitute(const ValueDecl *D);
  /// \brief Adds effects to TmpEffects and returns the number of effects added.
  int collectEffects(const ValueDecl *D, const Expr *exp);
    //checks that the effects in LHS are covered by RHS
  void checkEffectCoverage();
  void emitEffectNotCoveredWarning(const Stmt *S,
                                    const Decl *D,
                                    const StringRef &Str);

  void emitOverridenVirtualFunctionMustCoverEffectsOfChildren(
                                              const CXXMethodDecl *Parent,
                                              const CXXMethodDecl *Child);
  void emitUnsupportedConstructorInitializer(const CXXConstructorDecl *D);
  /// \brief Copy the effect summary of FunD and push it to the TmpEffects.
  int copyAndPushFunctionEffects(const FunctionDecl *FunD,
                                 const SubstitutionVector &SubV);

  void helperVisitAssignment(BinaryOperator *E);
  void helperVisitCXXConstructorDecl(const CXXConstructorDecl *D);

  void buildSingleParamSubstitution(ParmVarDecl *Param, Expr *Arg,
                                    const ParameterVector &ParamV,
                                    SubstitutionVector &SubV);
  void buildParamSubstitutions(const FunctionDecl *CalleeDecl,
                                ExprIterator ArgI, ExprIterator ArgE,
                                const ParameterVector &ParamV,
                                SubstitutionVector &SubV);

public:
  // Constructor
  EffectConstraintVisitor (const FunctionDecl* Def,
                           Stmt *S,
                           bool VisitCXXInitializer = false,
                           bool HasWriteSemantics = false );

  //~EffectConstraintVisitor();

  // Getters
  inline Trivalent getIsCoveredBySummary() { return IsCoveredBySummary; }

  // Visitors
  void VisitMemberExpr(MemberExpr *E);
  void VisitUnaryAddrOf(UnaryOperator *E);
  void VisitUnaryDeref(UnaryOperator *E);
  void VisitPrePostIncDec(UnaryOperator *E);
  void VisitUnaryPostInc(UnaryOperator *E);
  void VisitUnaryPostDec(UnaryOperator *E);
  void VisitUnaryPreInc(UnaryOperator *E);
  void VisitUnaryPreDec(UnaryOperator *E);
  void VisitReturnStmt(ReturnStmt *Ret);
  void VisitDeclRefExpr(DeclRefExpr *E);
  void VisitCXXThisExpr(CXXThisExpr *E);
  void VisitCompoundAssignOperator(CompoundAssignOperator *E);
  void VisitBinAssign(BinaryOperator *E);
  void VisitCallExpr(CallExpr *E);

  void VisitArraySubscriptExpr(ArraySubscriptExpr *E);
  void VisitCXXDeleteExpr(CXXDeleteExpr *E);
  void VisitCXXNewExpr(CXXNewExpr *E);
}; // End class EffectConstraintVisitor.
} // End namespace asap.
} // End namespace clang.

#endif
