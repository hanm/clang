//=== TypeChecker.h - Safe Parallelism checker -----*- C++ -*------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This files defines the Type Checker pass of the Safe Parallelism
// checker, which tries to prove the safety of parallelism given region
// and effect annotations.
//
//===----------------------------------------------------------------===//

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_TYPE_CHECKER_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_TYPE_CHECKER_H

#include "clang/AST/ASTContext.h"
#include "clang/AST/Type.h"
#include "clang/AST/StmtVisitor.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugReporter.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/AnalysisManager.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"
#include "ASaPSymbolTable.h"

namespace clang {

class FunctionDecl;
class Stmt;
class Expr;
class AnalysisDeclContext;

namespace asap {

class ASaPType;
class SubstitutionVector;

/// Find assignments and call Typechecking on them. Assignments include
/// * simple assignments: a = b
/// * complex assignments: a = b (where a and b are not scalars) TODO
/// * assignment of actuals to formals: f(a)
/// * return statements assigning expr to formal return type
/// * ...stay tuned, more to come

class AssignmentCheckerVisitor
    : public StmtVisitor<AssignmentCheckerVisitor> {

  ento::BugReporter &BR;
  ASTContext &Ctx;
  ento::AnalysisManager &Mgr;
  AnalysisDeclContext *AC;
  llvm::raw_ostream &OS;
  SymbolTable &SymT;
  const FunctionDecl *Def;
  bool FatalError;
  ASaPType *Type;

public:
  AssignmentCheckerVisitor (
    ento::BugReporter &BR,
    ASTContext &Ctx,
    ento::AnalysisManager &Mgr,
    AnalysisDeclContext *AC,
    raw_ostream &OS,
    SymbolTable &SymT,
    const FunctionDecl *Def,
    Stmt *S
    );
  ~AssignmentCheckerVisitor();

  inline bool encounteredFatalError() { return FatalError; }
  ASaPType *stealType();

  void VisitChildren(Stmt *S);
  void VisitStmt(Stmt *S);

  /// FIXME: Declarations only. Implementation later to support mutual recursion
  /// with TypeChecker.
  void VisitBinAssign(BinaryOperator *E);
  void VisitReturnStmt(ReturnStmt *Ret);

  void VisitCXXMemberCallExpr(CXXMemberCallExpr *Exp);
  void VisitCXXOperatorCallExpr(CXXOperatorCallExpr *Exp);
  void VisitMemberExpr(MemberExpr *Exp);
  void VisitDesignatedInitExpr(DesignatedInitExpr *Exp);
  void VisitCXXScalarValueInitExpr(CXXScalarValueInitExpr *Exp);
  void VisitInitListExpr(InitListExpr *Exp);
  void VisitDeclStmt(DeclStmt *S);

private:
  bool typecheck(const ASaPType *LHSType,
                 const ASaPType *RHSType,
                 bool IsInit = false);
  bool typecheckParamAssignment(ParmVarDecl *Param, Expr *Arg,
                                SubstitutionVector *SubV = 0);
  void typecheckCall(FunctionDecl *CalleeDecl,
                     ExprIterator ArgI,
                     ExprIterator ArgE,
                     SubstitutionVector *SubV = 0);
  void typecheckCallExpr(CallExpr *Exp);
  void typecheckCXXConstructExpr(VarDecl *D, CXXConstructExpr *Exp);
  void helperTypecheckDeclWithInit(const ValueDecl *VD, Expr *Init);
  /// \brief Issues Warning: '<str>' <bugName> on Declaration.
  void helperEmitDeclarationWarning(const Decl *D,
                                    const llvm::StringRef &Str,
                                    std::string BugName,
                                    bool AddQuotes = true);
  void helperEmitInvalidAliasingModificationWarning(Stmt *S, Decl *D,
                                                    const llvm::StringRef &Str);
  void helperEmitInvalidAssignmentWarning(const Stmt *S,
                                          const ASaPType *LHS,
                                          const ASaPType *RHS,
                                          llvm::StringRef BugName);
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

class TypeBuilderVisitor
    : public StmtVisitor<TypeBuilderVisitor, void> {
  ento::BugReporter &BR;
  ASTContext &Ctx;
  ento::AnalysisManager &Mgr;
  AnalysisDeclContext *AC;
  llvm::raw_ostream &OS;
  SymbolTable &SymT;
  const FunctionDecl *Def;
  bool FatalError;
  /// true when visiting a base expression (e.g., B in B.f, or B->f)
  bool IsBase;
  /// count of number of dereferences on expression (values in [-1, 0, ...] )
  int DerefNum;
  ASaPType *Type;
  QualType RefQT;

  /// \brief substitute region parameters in Type with arguments.
  void memberSubstitute(const ValueDecl *D);
  /// \brief collect the region arguments for a field.
  void setType(const ValueDecl *D);

public:
  TypeBuilderVisitor (
    ento::BugReporter &BR,
    ASTContext &Ctx,
    ento::AnalysisManager &Mgr,
    AnalysisDeclContext *AC,
    llvm::raw_ostream &OS,
    SymbolTable &SymT,
    const FunctionDecl *Def,
    Expr *E
    );
  virtual ~TypeBuilderVisitor();

  inline bool encounteredFatalError() { return FatalError; }
  inline ASaPType *getType() { return Type; }
  ASaPType *stealType();

  void VisitChildren(Stmt *S);
  void VisitStmt(Stmt *S);
  void VisitUnaryAddrOf(UnaryOperator *Exp);
  void VisitUnaryDeref(UnaryOperator *E);
  void VisitDeclRefExpr(DeclRefExpr *E);
  void VisitCXXThisExpr(CXXThisExpr *E);
  void VisitMemberExpr(MemberExpr *Exp);
  void helperBinAddSub(Expr *LHS, Expr *RHS);
  void VisitBinSub(BinaryOperator *Exp);
  void VisitBinAdd(BinaryOperator *Exp);
  void VisitBinAssign(BinaryOperator *Exp);
  void VisitConditionalOperator(ConditionalOperator *Exp);
  void VisitBinaryConditionalOperator(BinaryConditionalOperator *Exp);
  void VisitCXXNewExpr(CXXNewExpr *Exp);
  void VisitCallExpr(CallExpr *Exp);
  void VisitReturnStmt(ReturnStmt *Ret);
}; // End class TypeBuilderVisitor.

class BaseTypeBuilderVisitor
    : public StmtVisitor<BaseTypeBuilderVisitor, void> {
  ento::BugReporter &BR;
  ASTContext &Ctx;
  ento::AnalysisManager &Mgr;
  AnalysisDeclContext *AC;
  llvm::raw_ostream &OS;
  SymbolTable &SymT;
  const FunctionDecl *Def;
  bool FatalError;
  ASaPType *Type;
  QualType RefQT;

public:
  BaseTypeBuilderVisitor (
    ento::BugReporter &BR,
    ASTContext &Ctx,
    ento::AnalysisManager &Mgr,
    AnalysisDeclContext *AC,
    raw_ostream &OS,
    SymbolTable &SymT,
    const FunctionDecl *Def,
    Expr *E
    );

  virtual ~BaseTypeBuilderVisitor();

  inline bool encounteredFatalError() { return FatalError; }
  inline ASaPType *getType() { return Type; }
  ASaPType *stealType();

  void VisitChildren(Stmt *S);
  void VisitStmt(Stmt *S);
  void VisitMemberExpr(MemberExpr *Exp);
}; // end class BaseTypeBuilderVisitor
} // End namespace asap.
} // End namespace clang.

#endif
