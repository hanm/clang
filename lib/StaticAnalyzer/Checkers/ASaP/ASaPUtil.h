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

  // Static Constant Strings
static const std::string PL_RgnName = "rgn_name";
static const std::string PL_RgnParam = "rgn_param";
static const std::string PL_HasEffSum = "has_effect_summary";
static const std::string PL_UnNamedDecl = "unnamed";

static const std::string PL_NoEffect = "pure";
static const std::string PL_ReadsEffect = "reads";
static const std::string PL_WritesEffect = "writes";
static const std::string PL_AtomicReadsEffect = "atomic_reads";
static const std::string PL_AtomicWritesEffect = "atomic_writes";
static const std::string PL_InvokesEffect = "invokes";

static const std::string PL_EffectSummary = "effect_summary";
static const std::string PL_EffectVar = "effect_var";
static const std::string PL_ConcreteRpl = "rpl";
static const std::string PL_VarRpl = "rpl";
static const std::string PL_RplDomain = "rpl_domain";
static const std::string PL_ParamSub = "param_sub";
static const std::string PL_SubstitutionSet = "subst_set";
static const std::string PL_NullDomain = "null_dom";

static const std::string PL_InferEffSumPredicate = "infer_es";
static const std::string PL_SolveAllPredicate = "solve_all";
static const std::string PL_HasValuePredicate = "has_value";

/// \brief Rpl Inclusion Constraint
static const std::string PL_RIConstraint = "ri_constraint";
/// \brief Effect Summary Inclusion Constraint
static const std::string PL_ESIConstraint = "esi_constraint";
/// \brief Effect Non Interference Constraint
static const std::string PL_ENIConstraint = "eni_constraint";

static const std::string PL_ConstraintPrefix = "constraint";


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

enum Trivalent {
  //False
  RK_FALSE = false,
  //True
  RK_TRUE = true,
  //Don't know
  RK_DUNNO
};

Trivalent boolToTrivalent(bool B);
Trivalent trivalentAND(Trivalent A, Trivalent B);

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
term_t buildPLEmptyList();

void buildSingleParamSubstitution(const FunctionDecl *Def,
                                  SymbolTable &SymT,
                                  ParmVarDecl *Param, Expr *Arg,
                                  const ParameterSet &ParamSet,
                                  SubstitutionSet &SubS);

void buildParamSubstitutions(const FunctionDecl *Def,
                             SymbolTable &SymT,
                             const FunctionDecl *CalleeDecl,
                             ExprIterator ArgI, ExprIterator ArgE,
                             const ParameterSet &ParamSet,
                             SubstitutionSet &SubS);

void tryBuildParamSubstitutions(const FunctionDecl *Def,
                                SymbolTable &SymT,
                                const FunctionDecl *CalleeDecl,
                                ExprIterator ArgI, ExprIterator ArgE,
                                SubstitutionSet &SubS);

Stmt *getBody(const FunctionDecl *D);

} // end namespace asap
} // end namespace clang

#endif
