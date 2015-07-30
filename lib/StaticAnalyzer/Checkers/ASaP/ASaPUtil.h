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
#include "llvm/ADT/SmallPtrSet.h"
#include "clang/AST/Expr.h"

#include "ASaPFwdDecl.h"

namespace clang {

typedef CallExpr::arg_iterator ExprIterator;

namespace asap {

#ifndef NUM_OF_CONSTRAINTS
  #define NUM_OF_CONSTRAINTS 100
#endif
typedef llvm::SmallPtrSet<const Constraint*, NUM_OF_CONSTRAINTS> ConstraintsSetT;
typedef llvm::SmallPtrSet<const VarRpl*, NUM_OF_CONSTRAINTS> VarRplSetT;
typedef llvm::SmallPtrSet<const VarEffectSummary*, NUM_OF_CONSTRAINTS> VarEffectSummarySetT;


// Static Constant Strings
static const std::string PL_RulesFile = "/opt/lib/asap.pl";
static const std::string PL_ConstraintsFile = "constraints.pl";
static const std::string PL_ConstraintsStatsFile = "constraint-stats.txt";

static const std::string PL_RgnName = "util:rgn_name";
static const std::string PL_RgnParam = "util:rgn_param";
static const std::string PL_HasEffSum = "util:has_effect_summary";
static const std::string PL_HeadVarRpl = "util:head_rpl_var";
static const std::string PL_RplDomain = "util:rpl_domain";
/// \brief Rpl Inclusion Constraint
static const std::string PL_RIConstraint = "util:ri_constraint";
/// \brief Effect Summary Inclusion Constraint
static const std::string PL_ESIConstraint = "util:esi_constraint";
/// \brief Effect Non Interference Constraint
static const std::string PL_ENIConstraint = "util:eni_constraint";
static const std::string PL_HasValuePredicate = "util:has_value";
static const std::string PL_ReadHasValuePredicate = "has_value";

static const int PL_RgnNameArity = 1;
static const int PL_RgnParamArity = 1;
static const int PL_HeadVarRplArity = 2;
static const int PL_HasEffSumArity = 2;
static const int PL_RplDomainArity = 4;
static const int PL_RIConstraintArity = 3;
static const int PL_ESIConstraintArity = 3;
static const int PL_ENIConstraintArity = 3;

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
static const std::string PL_ParamSub = "param_sub";
static const std::string PL_SubstitutionSet = "subst_set";
static const std::string PL_NullDomain = "null_dom";

static const std::string PL_InferEffSumPredicate = "infer_es";
static const std::string PL_SolveAllPredicate = "solve_all";

static const std::string PL_RIConstraintPrefix = "ri_cons";
static const std::string PL_ESIConstraintPrefix = "esi_cons";
static const std::string PL_ENIConstraintPrefix = "eni_cons";

static const std::string PL_SimplifyBasic = "simplify_basic";
static const std::string PL_SimplifyDisjointjWrites = "simplify_disjoint_writes";
static const std::string PL_SimplifyRecursiveWrites = "simplify_recursive_writes";

static const std::string DOT_RIConstraintColor  = "green";
static const std::string DOT_ESIConstraintColor = "blue";
static const std::string DOT_ENIConstraintColor = "red";
static const std::string DOT_LHSEdgeColor = "blue";
static const std::string DOT_RHSEdgeColor = "red";


extern raw_ostream *OS;
extern raw_ostream *OSv2;
extern raw_fd_ostream &OS_Stat;
extern raw_fd_ostream &OS_PL;


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
void consultProlog(StringRef FileName);
void setupSimplifyLevel(int SimplifyLvl);
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
std::string getPLNormalizedName(const NamedDecl &Decl);

VarRplSetT *mergeRVSets(VarRplSetT *&LHS, VarRplSetT *&RHS);
VarEffectSummarySetT *mergeESVSets(VarEffectSummarySetT *&LHS, VarEffectSummarySetT *&RHS);

} // end namespace asap
} // end namespace clang

#endif
