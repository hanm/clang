//=== ASaPUtil.h - Safe Parallelism checker -----*- C++ -*-----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This file defines various utility methods used by the Safe Parallelism
// checker, which tries to prove the safety of parallelism given region
// and effect annotations.
//
//===----------------------------------------------------------------===//

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_UTIL_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_UTIL_H

#include <SWI-Prolog.h>

#include "clang/AST/Decl.h"

#include "ASaPFwdDecl.h"

namespace clang {
namespace asap {

#define ASAP_DEBUG
#define ASAP_DEBUG_VERBOSE2
extern raw_ostream &os;
extern raw_ostream &OSv2;


struct VisitorBundle {
  const CheckerBase *Checker;
  BugReporter *BR;
  ASTContext *Ctx;
  AnalysisManager *Mgr;
  AnalysisDeclContext *AC;
  raw_ostream *OS;
};

enum Trivalent{
  //True
  RK_TRUE,
  //False
  RK_FALSE,
  //Don't know
  RK_DUNNO
};

/// \brief Issues Warning: '<str>': <bugName> on Declaration.
void helperEmitDeclarationWarning(const CheckerBase *Checker,
                                  BugReporter &BR,
                                  const Decl *D,
                                  const StringRef &Str,
                                  const StringRef &BugName,
                                  bool AddQuotes = true);

/// \brief Issues Warning: '<str>': <bugName> on Attribute.
void helperEmitAttributeWarning(const CheckerBase *Checker,
                                BugReporter &BR,
                                const Decl *D,
                                const Attr *A,
                                const StringRef &Str,
                                const StringRef &BugName,
                                bool AddQuotes = true);

/// \brief Issues Warning: '<str>' <bugName> on Statement
void helperEmitStatementWarning(const CheckerBase *Checker,
                                BugReporter &BR,
                                AnalysisDeclContext *AC,
                                const Stmt *S,
                                const Decl *D,
                                const StringRef &Str,
                                const StringRef &BugName,
                                bool AddQuotes = true);

void helperEmitInvalidAssignmentWarning(const CheckerBase *Checker,
                                        BugReporter &BR,
                                        AnalysisDeclContext *AC,
                                        ASTContext &Ctx,
                                        const Stmt *S,
                                        const ASaPType *LHS,
                                        const ASaPType *RHS,
                                        StringRef &BugName);

const Decl *getDeclFromContext(const DeclContext *DC);

void assertzTermProlog(term_t Fact, StringRef ErrMsg = "");

void buildTypeSubstitution(const SymbolTable &SymT,
                           const RecordDecl *ClassD,
                           const ASaPType *Typ,
                           SubstitutionVector &SubV);

void buildSingleParamSubstitution(const FunctionDecl *Def,
                                  SymbolTable &SymT,
                                  ParmVarDecl *Param, Expr *Arg,
                                  const ParameterSet &ParamSet,
                                  SubstitutionVector &SubV);

void buildParamSubstitutions(const FunctionDecl *Def,
                             SymbolTable &SymT,
                             const FunctionDecl *CalleeDecl,
                             ExprIterator ArgI, ExprIterator ArgE,
                             const ParameterSet &ParamSet,
                             SubstitutionVector &SubV);

void tryBuildParamSubstitutions(
        const FunctionDecl *Def,
        SymbolTable &SymT,
        const FunctionDecl *CalleeDecl,
        ExprIterator ArgI, ExprIterator ArgE,
        SubstitutionVector &SubV);

} // end namespace asap
} // end namespace clang

#endif
