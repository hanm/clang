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

raw_ostream *OS = &llvm::nulls();
raw_ostream *OSv2 = &llvm::nulls();

using llvm::raw_string_ostream;

Trivalent boolToTrivalent(bool B) {
  return B==true ? RK_TRUE : RK_FALSE;
}

Trivalent trivalentAND(Trivalent A, Trivalent B) {
  if (A == RK_FALSE || B == RK_FALSE)
    return RK_FALSE;
  // else
  else if (A == RK_DUNNO || B == RK_DUNNO)
    return RK_DUNNO;
  else
    return RK_TRUE;
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
  predicate_t AssertzP = PL_predicate("assertz", 1, "user");
  int Rval = PL_call_predicate(NULL, PL_Q_NORMAL, AssertzP, Fact);
  assert(Rval && ErrMsg.data());
}

term_t buildPLEmptyList() {
  term_t Result = PL_new_term_ref();
  PL_put_nil(Result);
  return Result;
}

void buildSingleParamSubstitution(
        const FunctionDecl *Def,
        SymbolTable &SymT,
        ParmVarDecl *Param, Expr *Arg,
        const ParameterSet &ParamSet, // Set of fn & class region params
        SubstitutionSet &SubS) {
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
    if (const ConcreteRpl *ParamR = dyn_cast<ConcreteRpl>(*ParamI)) {
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
      if (SubS.hasBase(*Elmt))
        continue; // SubS already has a substitution for that base
      // TODO: check that the two inferred substitutions are equal.

      // TODO *ArgI might have a substitution vector
      //ArgIwoSubV
      //ArgSubV
      //Sub(Elmt, ArgIwoSubV)
      //SubS(Sub)
      //NewSubV(SubS)
      //NewSubV.push_back(ArgSubV)
      Substitution Sub(Elmt, *ArgI);
      *SymbolTable::VB.OS << "DEBUG::buildSingleParamSubstitution: adding Substitution = "
        << Sub.toString() << "\n";
      SubS.insert(&Sub);
      //OS << "DEBUG:: added function param sub: " << Sub.toString() << "\n";
    }
  }
  *SymbolTable::VB.OS << "DEBUG::  DONE buildSingleParamSubstitution \n";
}

void buildParamSubstitutions(
        const FunctionDecl *Def,
        SymbolTable &SymT,
        const FunctionDecl *CalleeDecl,
        ExprIterator ArgI, ExprIterator ArgE,
        const ParameterSet &ParamSet,
        SubstitutionSet &SubS) {
  assert(CalleeDecl);
  FunctionDecl::param_const_iterator ParamI, ParamE;
  *SymbolTable::VB.OS << "DEBUG:: buildParamSubstitutions... BEGIN!\n";
  *OSv2 << "DEBUG: SubS = " << SubS.toString() << "\n";

  for(ParamI = CalleeDecl->param_begin(), ParamE = CalleeDecl->param_end();
      ArgI != ArgE && ParamI != ParamE; ++ArgI, ++ParamI) {
    Expr *ArgExpr = *ArgI;
    ParmVarDecl *ParamDecl = *ParamI;
    buildSingleParamSubstitution(Def, SymT, ParamDecl, ArgExpr, ParamSet, SubS);
    *OSv2 << "DEBUG: SubS = " << SubS.toString() << "\n";
  }
  *SymbolTable::VB.OS << "DEBUG:: DONE buildParamSubstitutions\n";
}

void tryBuildParamSubstitutions(
        const FunctionDecl *Def,
        SymbolTable &SymT,
        const FunctionDecl *CalleeDecl,
        ExprIterator ArgI, ExprIterator ArgE,
        SubstitutionSet &SubS) {
  assert(CalleeDecl);
  ParameterSet *ParamSet = new ParameterSet();
  assert(ParamSet);
  // Build SubS for function region params
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
  *OSv2 << "DEBUG:: ParamSet = " << ParamSet->toString() << "\n";
  if (ParamSet->size() > 0) {
    buildParamSubstitutions(Def, SymT, CalleeDecl, ArgI, ArgE, *ParamSet, SubS);
    *SymbolTable::VB.OS << "DEBUG:: DONE buildParamSubstitutions\n";
    *OSv2 << "DEBUG: SubS = " << SubS.toString() << "\n";
  }
  delete ParamSet;
  *SymbolTable::VB.OS << "DEBUG:: DONE delete ParamSet\n";
}

Stmt *getBody(const FunctionDecl *D) {
  const FunctionDecl *Definition;
  if (D->hasBody(Definition)) {
    return Definition->getBody();
  } else {
    return 0;
  }
}

/// \brief True if Str only contains characters and underscores and digits
bool isSimpleIdentifier(const llvm::StringRef& Str) {
  if (Str.size() <= 0)
    return true;
  // must start with [_a-zA-Z]
  const char c = Str.front();
  if (c != '_' &&
    !( c >= 'a' && c <= 'z') &&
    !( c >= 'A' && c <= 'Z'))
    return false;
  // all remaining characters must be in [_a-zA-Z0-9]
  for (size_t I = 0; I < Str.size(); I++) {
    const char C = Str[I];
    if (C != '_' &&
      !( C >= 'a' && C <= 'z') &&
      !( C >= 'A' && C <= 'Z') &&
      !( C >= '0' && C <= '9'))
      return false;
  }
  return true;
}


StringRef getPLNormalizedName(const NamedDecl &Dec) {
  StringRef Name = Dec.getNameAsString();
  if (Name.size() <= 0)
    return "UnNamed";
  if (isSimpleIdentifier(Name)) {
    return Name;
  }
  //else
  *OSv2 << "DEBUG:: getPLNormalizedName:: Name = " << Name << "\n";
  if (Name.startswith("operator")) {
    StringRef Op = Name.drop_front(8);
    *OSv2 << "DEBUG:: getPLNormalizedName:: operator is '" << Op << "'\n";
    if (Op.equals("()"))
      return "operatorCall";
    if (Op.equals("+"))
      return "operatorPlus";
    if (Op.equals("-"))
      return "operatorMinus";
    if (Op.equals("*"))
      return "operatorTimes";
    if (Op.equals("/"))
      return "operatorDiv";
    if (Op.equals("="))
      return "operatorAssign";
    if (Op.equals("=="))
      return "operatorEquals";
    if (Op.equals("+="))
      return "operatorPlusEq";
    if (Op.equals("-="))
      return "operatorMinusEq";
    if (Op.equals("*="))
      return "operatorTimesEq";
    if (Op.equals("/="))
      return "operatorDivEq";
    // TODO: rest
  }
  return "";
}

} // end namespace asap
} // end namespace clang
