//=== TypeChecker.h - Safe Parallelism checker -----*- C++ -*------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This file defines the Type Checker pass of the Safe Parallelism
// checker, which tries to prove the safety of parallelism given region
// and effect annotations.
//
//===----------------------------------------------------------------===//

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_TYPE_CHECKER_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_TYPE_CHECKER_H

#include "clang/AST/StmtVisitor.h"

#include "ASaPFwdDecl.h"
#include "ASaPGenericStmtVisitor.h"
#include "ASaPType.h"

namespace clang {
namespace asap {

//////////////////////////////////////////////////////////////////////////
// class AssignmentCheckerVisitor
//
// Find assignments and call Typechecking on them. Assignments include
// * simple assignments: a = b
// * complex assignments: a = b (where a and b are not scalars) TODO
// * assignment of actuals to formals: f(a)
// * return statements assigning expr to formal return type
// * ...stay tuned, more to come
class AssignmentCheckerVisitor
    : public ASaPStmtVisitor<AssignmentCheckerVisitor> {

  typedef ASaPStmtVisitor<AssignmentCheckerVisitor> BaseClass;

  ASaPType *Type;

public:
  AssignmentCheckerVisitor (
    const FunctionDecl *Def,
    Stmt *S,
    bool VisitCXXInitializer = false  // true when called on the function,
                                      // false when called recursively (default).
    );

  ~AssignmentCheckerVisitor();

  inline ASaPType *getType() { return Type; }
  ASaPType *stealType();

  void VisitBinAssign(BinaryOperator *E);
  void VisitReturnStmt(ReturnStmt *Ret);
  void VisitCXXConstructExpr(CXXConstructExpr *E);
  void VisitCallExpr(CallExpr *E);
  void VisitMemberExpr(MemberExpr *Exp);
  void VisitDesignatedInitExpr(DesignatedInitExpr *E);
  void VisitCXXScalarValueInitExpr(CXXScalarValueInitExpr *E);
  void VisitInitListExpr(InitListExpr *E);
  void VisitDeclStmt(DeclStmt *S);

private:
  Trivalent typecheck(const ASaPType *LHSType,
                      const ASaPType *RHSType,
                      bool IsInit = false);
  // These typecheck and build functions below should be static but then
  // would not be able to print debug information...
  bool typecheckSingleParamAssignment(ParmVarDecl *Param, Expr *Arg,
                                      const SubstitutionVector &SubV,
                                      SubstitutionSet &SubS);
  void typecheckParamAssignments(FunctionDecl *CalleeDecl,
                                 ExprIterator ArgI,
                                 ExprIterator ArgE,
                                 const SubstitutionVector &SubV,
                                 SubstitutionSet &SubS);
  void typecheckCallExpr(CallExpr *Exp);
  void typecheckCXXConstructExpr(VarDecl *D, CXXConstructExpr *Exp);

  void helperTypecheckDeclWithInit(const ValueDecl *VD, Expr *Init);

  void helperEmitInvalidArgToFunctionWarning(const Stmt *S,
                                                  const ASaPType *LHS,
                                                  const ASaPType *RHS);
  void helperEmitInvalidExplicitAssignmentWarning(const Stmt *S,
                                                  const ASaPType *LHS,
                                                  const ASaPType *RHS);
  void helperEmitInvalidReturnTypeWarning(const Stmt *S,
                                          const ASaPType *LHS,
                                          const ASaPType *RHS);
  void helperEmitInvalidInitializationWarning(const Stmt *S,
                                              const ASaPType *LHS,
                                              const ASaPType *RHS);
  void helperEmitUnsupportedConstructorInitializer(const CXXConstructorDecl *D);
  /// \brief Typecheck Constructor initialization lists.
  void helperVisitCXXConstructorDecl(const CXXConstructorDecl *D);
}; // end class AssignmentCheckerVisitor


//////////////////////////////////////////////////////////////////////////
// class TypeBuilder
class TypeBuilderVisitor
    : public ASaPStmtVisitor<TypeBuilderVisitor> {

  typedef ASaPStmtVisitor<TypeBuilderVisitor> BaseClass;

  /// true when visiting a base expression (e.g., B in B.f, or B->f)
  bool IsBase;
  /// count of number of dereferences on expression (values in [-1, 0, ...] )
  int DerefNum;
  ASaPType *Type;
  QualType RefQT;

  bool WarnUnsafeCasts;


  /// \brief substitute region parameters in Type with arguments.
  void memberSubstitute(const ValueDecl *D);
  void memberSubstitute(const ASaPType *T);
  /// \brief collect the region arguments for a field.
  void setType(const ValueDecl *D);
  void setType(const ASaPType *T);
  /// \brief Visit Logical Unary or Binary Expression
  void helperVisitLogicalExpression(Expr *Exp);
  void helperBinAddSub(BinaryOperator* Exp);
  void clearType() { delete Type; Type =0; }

public:
  TypeBuilderVisitor (const FunctionDecl *Def, Expr *E);
  virtual ~TypeBuilderVisitor();

  inline bool encounteredFatalError() { return FatalError; }

  inline ASaPType *getType() { return Type; }
  ASaPType *stealType();

  // Visitors
  void VisitUnaryAddrOf(UnaryOperator *Exp);
  void VisitUnaryDeref(UnaryOperator *Exp);
  void VisitUnaryLNot(UnaryOperator *Exp);
  void VisitDeclRefExpr(DeclRefExpr *Exp);
  void VisitCXXThisExpr(CXXThisExpr *Exp);
  void VisitMemberExpr(MemberExpr *Exp);
  void VisitBinaryOperator(BinaryOperator *Exp);
  void VisitConditionalOperator(ConditionalOperator *Exp);
  void VisitBinaryConditionalOperator(BinaryConditionalOperator *Exp);
  void VisitCXXConstructExpr(CXXConstructExpr *Exp);
  void VisitCallExpr(CallExpr *Exp);
  void VisitArraySubscriptExpr(ArraySubscriptExpr *Exp);
  void VisitReturnStmt(ReturnStmt *Ret);
  void VisitCastExpr(CastExpr *Exp);
  void VisitExplicitCastExpr(ExplicitCastExpr *Exp);
  void VisitImplicitCastExpr(ImplicitCastExpr *Exp);
  void VisitVAArgExpr(VAArgExpr *Exp);
  void VisitCXXNewExpr(CXXNewExpr *Exp);
  void VisitAtomicExpr(AtomicExpr *Exp);
  void VisitUnaryExprOrTypeTraitExpr(UnaryExprOrTypeTraitExpr *Exp);
}; // End class TypeBuilderVisitor.


//////////////////////////////////////////////////////////////////////////
// class BaseTypeBuilderVisitor
class BaseTypeBuilderVisitor
    : public ASaPStmtVisitor<BaseTypeBuilderVisitor> {
  typedef ASaPStmtVisitor<BaseTypeBuilderVisitor> BaseClass;

  ASaPType *Type;
  QualType RefQT;

public:
  BaseTypeBuilderVisitor (const FunctionDecl *Def, Expr *E);

  virtual ~BaseTypeBuilderVisitor();

  inline bool encounteredFatalError() { return FatalError; }
  inline ASaPType *getType() { return Type; }
  ASaPType *stealType();

  void VisitMemberExpr(MemberExpr *Exp);
}; // end class BaseTypeBuilderVisitor
} // End namespace asap.
} // End namespace clang.

#endif
