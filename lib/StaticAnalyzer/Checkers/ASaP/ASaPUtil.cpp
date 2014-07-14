//=== ASaPUtil.cpp - Safe Parallelism checker -----*- C++ -*---------===//
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

#include "llvm/Support/raw_ostream.h"

#include "clang/AST/Attr.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Expr.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugReporter.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"

#include "ASaPSymbolTable.h"
#include "ASaPType.h"
#include "ASaPUtil.h"
#include "Rpl.h"
#include "Substitution.h"
#include "TypeChecker.h"

namespace clang {
namespace asap {

static const StringRef BugCategory = "Safe Parallelism";

#ifdef ASAP_DEBUG
raw_ostream &os = llvm::errs();
#else
raw_ostream &os = llvm::nulls();
#endif

#ifdef ASAP_DEBUG_VERBOSE2
raw_ostream &OSv2 = llvm::errs();
#else
raw_ostream &OSv2 = llvm::nulls();
#endif

using llvm::raw_string_ostream;

Trivalent Bool2Trivalent(bool B) {
  return B==true ? RK_TRUE : RK_FALSE;
}

void helperEmitDeclarationWarning(const CheckerBase *Checker,
                                  BugReporter &BR,
                                  const Decl *D,
                                  const StringRef &Str,
                                  const StringRef &BugName,
                                  bool AddQuotes) {
  std::string Description;
  raw_string_ostream DescrOS(Description);
  DescrOS << (AddQuotes ? "'" : "") << Str
          << (AddQuotes ? "'" : "") << ": " << BugName;

  StringRef BugStr = DescrOS.str();
  assert(D && "Expected non-null Decl pointer");
  PathDiagnosticLocation VDLoc(D->getLocation(), BR.getSourceManager());
  BR.EmitBasicReport(D, Checker, BugName, BugCategory,
                     BugStr, VDLoc, D->getSourceRange());
}

void helperEmitAttributeWarning(const CheckerBase *Checker,
                                BugReporter &BR,
                                const Decl *D,
                                const Attr *Attr,
                                const StringRef &Str,
                                const StringRef &BugName,
                                bool AddQuotes) {
  std::string Description;
  raw_string_ostream DescrOS(Description);
  DescrOS << (AddQuotes ? "'" : "") << Str
          << (AddQuotes ? "'" : "") << ": " << BugName;

  StringRef BugStr = DescrOS.str();
  PathDiagnosticLocation VDLoc(Attr->getLocation(), BR.getSourceManager());
  BR.EmitBasicReport(D, Checker, BugName, BugCategory,
                     BugStr, VDLoc, Attr->getRange());
}


void helperEmitStatementWarning(const CheckerBase *Checker,
                                BugReporter &BR,
                                AnalysisDeclContext *AC,
                                const Stmt *S,
                                const Decl *D,
                                const StringRef &Str,
                                const StringRef &BugName,
                                bool AddQuotes) {
  std::string Description;
  raw_string_ostream DescrOS(Description);
  DescrOS << (AddQuotes ? "'" : "") << Str
          << (AddQuotes ? "'" : "") << ": " << BugName;

  StringRef BugStr = DescrOS.str();

  PathDiagnosticLocation VDLoc =
      PathDiagnosticLocation::createBegin(S, BR.getSourceManager(), AC);

  BR.EmitBasicReport(D, Checker, BugName, BugCategory,
                     BugStr, VDLoc, S->getSourceRange());
}

void helperEmitInvalidAssignmentWarning(const CheckerBase *Checker,
                                        BugReporter &BR,
                                        AnalysisDeclContext *AC,
                                        ASTContext &Ctx,
                                        const Stmt *S,
                                        const ASaPType *LHS,
                                        const ASaPType *RHS,
                                        StringRef &BugName) {
  std::string Description;
  raw_string_ostream DescrOS(Description);

  DescrOS << "The RHS type [" << (RHS ? RHS->toString(Ctx) : "")
          << "] is not assignable to the LHS type ["
          << (LHS ? LHS->toString(Ctx) : "")
          << "] " << BugName << ": ";
  S->printPretty(DescrOS, 0, Ctx.getPrintingPolicy());

  StringRef BugStr = DescrOS.str();

  PathDiagnosticLocation VDLoc =
      PathDiagnosticLocation::createBegin(S, BR.getSourceManager(), AC);

  ento::BugType *BT = new ento::BugType(Checker, BugName, BugCategory);
  ento::BugReport *R = new ento::BugReport(*BT, BugStr, VDLoc);
  BR.emitReport(R);
}

const Decl *getDeclFromContext(const DeclContext *DC) {
  assert(DC);
  const Decl *D = 0;
  if (DC->isFunctionOrMethod())
    D = dyn_cast<FunctionDecl>(DC);
  else if (DC->isRecord())
    D = dyn_cast<RecordDecl>(DC);
  else if (DC->isNamespace())
    D = dyn_cast<NamespaceDecl>(DC);
  else if (DC->isTranslationUnit())
    D = dyn_cast<TranslationUnitDecl>(DC);
  return D;
}

void assertzTermProlog(term_t Fact, StringRef ErrMsg) {
  predicate_t AssertzP = PL_predicate("assertz",1,"user");
  int Rval = PL_call_predicate(NULL, PL_Q_NORMAL, AssertzP, Fact);
  assert(Rval && ErrMsg.data());
}

void buildTypeSubstitution(const SymbolTable &SymT,
                           const RecordDecl *ClassD,
                           const ASaPType *Typ,
                           SubstitutionVector &SubV) {
  if (!ClassD || !Typ)
    return;

  // Set up Substitution Vector
  const ParameterVector *PV = SymT.getParameterVector(ClassD);
  SubV.add(Typ, PV);
}

void buildSingleParamSubstitution(
        const FunctionDecl *Def,
        SymbolTable &SymT,
        ParmVarDecl *Param, Expr *Arg,
        const ParameterSet &ParamSet, // Set of fn & class region params
        SubstitutionVector &SubV) {
  *SymbolTable::VB.OS << "DEBUG::  buildSingleParamSubstitution BEGIN\n";
  // if the function parameter has region argument that is a region
  // parameter, infer a substitution based on the type of the function argument
  const ASaPType *ParamType = SymT.getType(Param);
  if (!ParamType)
    return;
  const RplVector *ParamArgV = ParamType->getArgV();
  if (!ParamArgV)
    return;
  TypeBuilderVisitor TBV(Def, Arg);
  const ASaPType *ArgType = TBV.getType();
  if (!ArgType)
    return;
  const RplVector *ArgArgV = ArgType->getArgV();
  if (!ArgArgV)
    return;
  // For each element of ArgV if it's a simple arg, check if it's
  // a function region param
  for(RplVector::const_iterator
        ParamI = ParamArgV->begin(), ParamE = ParamArgV->end(),
        ArgI = ArgArgV->begin(), ArgE = ArgArgV->end();
      ParamI != ParamE && ArgI != ArgE; ++ParamI, ++ArgI) {

    assert(isa<ConcreteRpl>(*ParamI) && "ParamI should be of type ConcreteRpl");
    const ConcreteRpl *ParamR = dyn_cast<ConcreteRpl>(*ParamI);

    assert(ParamR && "RplVector should not contain null Rpl pointer");
    if (ParamR->length() < 1)
      continue;
    if (ParamR->length() > 1)
      // In this case, we need to implement type unification
      // of ParamR and ArgR = *ArgI or allow explicitly giving
      // the substitution in an annotation at the call-site
      continue;
    const RplElement *Elmt = ParamR->getFirstElement();
    assert(Elmt && "Rpl should not contain null RplElement pointer");
    if (!ParamSet.hasElement(Elmt))
      continue;
    // Ok find the argument
    Substitution Sub(Elmt, *ArgI);
    *SymbolTable::VB.OS << "DEBUG::buildSingleParamSubstitution: adding Substitution = "
       << Sub.toString() << "\n";
    SubV.push_back(&Sub);
    //OS << "DEBUG:: added function param sub: " << Sub.toString() << "\n";
  }
  *SymbolTable::VB.OS << "DEBUG::  DONE buildSingleParamSubstitution \n";
}

void buildParamSubstitutions(
        const FunctionDecl *Def,
        SymbolTable &SymT,
        const FunctionDecl *CalleeDecl,
        ExprIterator ArgI, ExprIterator ArgE,
        const ParameterSet &ParamSet,
        SubstitutionVector &SubV) {
  assert(CalleeDecl);
  FunctionDecl::param_const_iterator ParamI, ParamE;
  *SymbolTable::VB.OS << "DEBUG:: buildParamSUbstitutions... BEGIN!\n";

  for(ParamI = CalleeDecl->param_begin(), ParamE = CalleeDecl->param_end();
      ArgI != ArgE && ParamI != ParamE; ++ArgI, ++ParamI) {
    Expr *ArgExpr = *ArgI;
    ParmVarDecl *ParamDecl = *ParamI;
    buildSingleParamSubstitution(Def, SymT, ParamDecl, ArgExpr, ParamSet, SubV);
  }
  *SymbolTable::VB.OS << "DEBUG:: DONE buildParamSUbstitutions\n";
}

void tryBuildParamSubstitutions(
        const FunctionDecl *Def,
        SymbolTable &SymT,
        const FunctionDecl *CalleeDecl,
        ExprIterator ArgI, ExprIterator ArgE,
        SubstitutionVector &SubV) {
  assert(CalleeDecl);
  ParameterSet *ParamSet = new ParameterSet();
  assert(ParamSet);
  // Build SubV for function region params
  const ParameterVector *ParamV = SymT.getParameterVector(CalleeDecl);
  if (ParamV && ParamV->size() > 0) {
    ParamV->addToParamSet(ParamSet);
  }
  // if isa<CXXMethodDecl>CalleeDecl -> add Class parameters to set
  if (const CXXMethodDecl *CXXCalleeDecl = dyn_cast<CXXMethodDecl>(CalleeDecl)) {
    const CXXRecordDecl *Rec = CXXCalleeDecl->getParent();
    ParamV = SymT.getParameterVector(Rec);
    if (ParamV && ParamV->size() > 0) {
      ParamV->addToParamSet(ParamSet);
    }
  }
  if (ParamSet->size() > 0) {
    buildParamSubstitutions(Def, SymT, CalleeDecl, ArgI, ArgE, *ParamSet, SubV);
  }
  delete ParamSet;
}

} // end namespace asap
} // end namespace clang
