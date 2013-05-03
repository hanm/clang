//=== TypeChecker.cpp - Safe Parallelism checker -----*- C++ -*------===//
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

#include "TypeChecker.h"
#include "ASaPType.h"
#include "Effect.h"
#include "Rpl.h"
#include "Substitution.h"

#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"

using namespace clang;
using namespace clang::asap;
using namespace clang::ento;
using namespace llvm;

AssignmentCheckerVisitor::AssignmentCheckerVisitor(
  ento::BugReporter &BR,
  ASTContext &Ctx,
  AnalysisManager &Mgr,
  AnalysisDeclContext *AC,
  raw_ostream &OS,
  SymbolTable &SymT,
  const FunctionDecl *Def,
  Stmt *S,
  bool VisitCXXInitializer
  ) : BR(BR),
  Ctx(Ctx),
  Mgr(Mgr),
  AC(AC),
  OS(OS),
  SymT(SymT),
  Def(Def),
  FatalError(false), Type(0), SubV(0) {

    OS << "DEBUG:: ******** INVOKING AssignmentCheckerVisitor...\n";
    OS << "DEBUG:: Stmt:";
    S->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\nDEBUG:: Def:\n";
    Def->print(OS, Ctx.getPrintingPolicy());
    OS << "\n";
    Def->dump(OS);
    OS << "\n";

    if (VisitCXXInitializer) {
      if (const CXXConstructorDecl *D = dyn_cast<CXXConstructorDecl>(Def)) {
        // Also visit initialization lists
        helperVisitCXXConstructorDecl(D);
      }
    }
    Visit(S);
    OS << "DEBUG:: ******** DONE INVOKING AssignmentCheckerVisitor ***\n";
}

AssignmentCheckerVisitor::~AssignmentCheckerVisitor() {
  delete Type;
}

ASaPType *AssignmentCheckerVisitor::stealType() {
  ASaPType *Result = Type;
  Type = 0;
  return Result;
}

void AssignmentCheckerVisitor::VisitChildren(Stmt *S) {
  for (Stmt::child_iterator I = S->child_begin(), E = S->child_end();
    I!=E; ++I)
    if (Stmt *child = *I)
      Visit(child);
}

void AssignmentCheckerVisitor::VisitStmt(Stmt *S) {
  OS << "DEBUG:: GENERIC:: Visiting Stmt/Expr = \n";
  S->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  VisitChildren(S);
}

void AssignmentCheckerVisitor::
VisitCallExpr(CallExpr *Exp) {
  assert(!SubV);
  SubV = new SubstitutionVector();
  typecheckCallExpr(Exp, *SubV);
  delete SubV;
  SubV = 0;
}

void AssignmentCheckerVisitor::VisitMemberExpr(MemberExpr *Exp) {
  OS << "DEBUG:: VisitMemberExpr: ";
  Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  VisitChildren(Exp);
}

void AssignmentCheckerVisitor::
VisitDesignatedInitExpr(DesignatedInitExpr *Exp) {
  OS << "Designated INIT Expr!!\n";
  // TODO?
}

void AssignmentCheckerVisitor::
VisitCXXScalarValueInitExpr(CXXScalarValueInitExpr *Exp) {
  OS << "CXX Scalar Value INIT Expr!!\n";
  // TODO?
}

void AssignmentCheckerVisitor::VisitInitListExpr(InitListExpr *Exp) {
  OS << "DEBUG:: VisitInitListExpr: ";
  Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  // TODO?
}

void AssignmentCheckerVisitor::VisitDeclStmt(DeclStmt *S) {
  OS << "Decl Stmt INIT ?? (";
  S->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << ")\n";
  for(DeclGroupRef::const_iterator I = S->decl_begin(), E = S->decl_end();
      I != E; ++I) {
    if (VarDecl *VD = dyn_cast<VarDecl>(*I)) {
      if (VD->hasInit()) {
        Expr *Init = VD->getInit();

        OS << "DEBUG:: TypecheckDeclWithInit: Decl = ";
        VD->print(OS,  Ctx.getPrintingPolicy());
        OS << "\n Init Expr = ";
        Init->printPretty(OS, 0, Ctx.getPrintingPolicy());
        OS << "\n";
        Init->dump(OS, BR.getSourceManager());

        OS << "DEBUG:: IsDirectInit = "
           << (VD->isDirectInit()?"true":"false")
           << "\n";
        OS << "DEBUG:: Init Style: ";
        switch(VD->getInitStyle()) {
        case VarDecl::CInit:
          OS << "CInit\n";
          helperTypecheckDeclWithInit(VD, VD->getInit());
          break;
        case VarDecl::ListInit:
          OS << "ListInit\n";
          // Intentonally falling through (i.e., no break stmt).
        case VarDecl::CallInit:
          OS << "CallInit\n";
          CXXConstructExpr *Exp = dyn_cast<CXXConstructExpr>(VD->getInit());
          assert(Exp);
          assert(!SubV);
          SubV = new SubstitutionVector();
          typecheckCXXConstructExpr(VD, Exp, *SubV);
          delete SubV;
          SubV = 0;
          break;
        }
      }
    }
  } // end for each declaration
}

bool AssignmentCheckerVisitor::
typecheck(const ASaPType *LHSType, const ASaPType *RHSType, bool IsInit) {
  if (!RHSType)
    return true; // RHS has no region info && Clang has done typechecking
  else { // RHSType != null
    if (LHSType)
      return RHSType->isAssignableTo(*LHSType, IsInit);
    else
      return false;
  }
}

void AssignmentCheckerVisitor::
helperEmitDeclarationWarning(const Decl *D, const StringRef &Str,
                             std::string BugName, bool AddQuotes) {
  std::string Description = "";
  if (AddQuotes)
    Description.append("'");
  Description.append(Str);
  if (AddQuotes)
    Description.append("': ");
  else
    Description.append(": ");
  Description.append(BugName);
  StringRef BugCategory = "Safe Parallelism";
  StringRef BugStr = Description;
  PathDiagnosticLocation VDLoc(D->getLocation(), BR.getSourceManager());
  BR.EmitBasicReport(D, BugName, BugCategory,
                     BugStr, VDLoc, D->getSourceRange());
}

void AssignmentCheckerVisitor::
helperEmitInvalidAliasingModificationWarning(Stmt *S, Decl *D,
                                             const StringRef &Str) {
  StringRef BugName =
    "cannot modify aliasing through pointer to partly specified region";
  std::string description_std = "'";
  description_std.append(Str);
  description_std.append("' ");
  description_std.append(BugName);

  StringRef BugCategory = "Safe Parallelism";
  StringRef BugStr = description_std;

  PathDiagnosticLocation VDLoc =
    PathDiagnosticLocation::createBegin(S, BR.getSourceManager(), AC);

  BR.EmitBasicReport(D, BugName, BugCategory,
    BugStr, VDLoc, S->getSourceRange());
}

void AssignmentCheckerVisitor::
helperEmitInvalidAssignmentWarning(const Stmt *S, const ASaPType *LHS,
                                   const ASaPType *RHS, StringRef BugName) {
  std::string description_std = "The RHS type [";
  description_std.append(RHS ? RHS->toString(Ctx) : "");
  description_std.append("] is not assignable to the LHS type [");
  description_std.append(LHS ? LHS->toString(Ctx) : "");
  description_std.append("] ");
  description_std.append(BugName);
  std::string SBuf;
  llvm::raw_string_ostream StrBuf(SBuf);
  StrBuf << ": ";
  S->printPretty(StrBuf, 0, Ctx.getPrintingPolicy());
  StrBuf << "]";
  description_std.append(StrBuf.str());

  StringRef BugCategory = "Safe Parallelism";
  StringRef BugStr = description_std;

  PathDiagnosticLocation VDLoc =
    PathDiagnosticLocation::createBegin(S, BR.getSourceManager(), AC);

  BugType *BT = new BugType(BugName, BugCategory);
  BugReport *R = new BugReport(*BT, BugStr, VDLoc);
  BR.emitReport(R);
}

void AssignmentCheckerVisitor::
helperEmitInvalidArgToFunctionWarning(const Stmt *S, const ASaPType *LHS,
                                      const ASaPType *RHS) {
  StringRef BugName = "invalid argument to function call";
  helperEmitInvalidAssignmentWarning(S, LHS, RHS, BugName);
}

void AssignmentCheckerVisitor::
helperEmitInvalidExplicitAssignmentWarning(const Stmt *S, const ASaPType *LHS,
                                           const ASaPType *RHS) {
  StringRef BugName = "invalid assignment";
  helperEmitInvalidAssignmentWarning(S, LHS, RHS, BugName);
}

void AssignmentCheckerVisitor::
helperEmitInvalidReturnTypeWarning(const Stmt *S, const ASaPType *LHS,
                                   const ASaPType *RHS) {
  StringRef BugName = "invalid return type";
  helperEmitInvalidAssignmentWarning(S, LHS, RHS, BugName);
}

void AssignmentCheckerVisitor::
helperEmitInvalidInitializationWarning(const Stmt *S, const ASaPType *LHS,
                                       const ASaPType *RHS) {
  StringRef BugName = "invalid initialization";
  helperEmitInvalidAssignmentWarning(S, LHS, RHS, BugName);
}

void AssignmentCheckerVisitor::
helperEmitUnsupportedConstructorInitializer(const CXXConstructorDecl *D) {
  StringRef BugName = "unsupported constructor initializer."
    " Please file feature support request.";
  helperEmitDeclarationWarning(D, "", BugName, false);
}

void AssignmentCheckerVisitor::
helperVisitCXXConstructorDecl(const CXXConstructorDecl *D) {
  CXXConstructorDecl::init_const_iterator
    I = D->init_begin(),
    E = D->init_end();
  for(; I != E; ++I) {
    CXXCtorInitializer *Init = *I;
    if (Init->isMemberInitializer()) {
      helperTypecheckDeclWithInit(Init->getMember(), Init->getInit());
    } else if (Init->isBaseInitializer()) {
      //AssignmentCheckerVisitor ACV(BR, Ctx, Mgr, AC, OS,
      //  SymT, Def, Init->getInit());
      //FatalError |= ACV.encounteredFatalError();
      Visit(Init->getInit());
    } else {
      helperEmitUnsupportedConstructorInitializer(D);
    }
  }
}
// TODO: does this cover compound assignment?
void AssignmentCheckerVisitor::VisitBinAssign(BinaryOperator *E) {
  OS << "DEBUG:: >>>>>>>>>> TYPECHECKING BinAssign<<<<<<<<<<<<<<<<<\n";
  E->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  TypeBuilderVisitor TBVR(BR, Ctx, Mgr, AC, OS, SymT, Def, E->getRHS());
  TypeBuilderVisitor TBVL(BR, Ctx, Mgr, AC, OS, SymT, Def, E->getLHS());
  OS << "DEBUG:: Ran type builder on RHS & LHS\n";
  E->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  ASaPType *LHSType = TBVL.getType();
  ASaPType *RHSType = TBVR.getType();
  assert(LHSType);

  // allow RHSType to be NULL, e.g., we don't create ASaP Types for constants
  // because they don't have any interesting regions to typecheck.
  if (! typecheck(LHSType, RHSType, false)) {
    OS << "DEBUG:: invalid assignment: gonna emit an error\n";
    helperEmitInvalidExplicitAssignmentWarning(E, LHSType, RHSType);
    FatalError = true;
  }

  // The type of the assignment is the type of the LHS. Set it in case
  // AssignmentChecker was called recursively by a TypeBuilderVisitor
  delete Type;
  Type = TBVL.stealType();
  assert(Type);

  OS << "DEBUG:: >>>>>>>>>> DONE TYPECHECKING BinAssign<<<<<<<<<<<<<<<<<\n";
}

void AssignmentCheckerVisitor::VisitReturnStmt(ReturnStmt *Ret) {
  Expr *RetExp = Ret->getRetValue();
  OS << "DEBUG:: Visiting ReturnStmt: ";
  RetExp->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";

  TypeBuilderVisitor TBVR(BR, Ctx, Mgr, AC, OS, SymT, Def, RetExp);
  const ASaPType *FunType = SymT.getType(Def);
  assert(FunType);
  assert(FunType->isFunctionType());
  ASaPType *LHSType = new ASaPType(*FunType);
  LHSType = LHSType->getReturnType();
  assert(LHSType);
  ASaPType *RHSType = TBVR.getType();
  if (! typecheck(LHSType, RHSType, true)) {
    OS << "DEBUG:: invalid assignment: gonna emit an error\n";
    helperEmitInvalidReturnTypeWarning(Ret, LHSType, RHSType);
    FatalError = true;
  }
  delete LHSType;
}

void AssignmentCheckerVisitor::
VisitCXXConstructExpr(CXXConstructExpr *Exp) {
  OS << "DEBUG:: Visiting CXXConstructExpr: ";
  Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  assert(!SubV);
  SubV = new SubstitutionVector();
  typecheckParamAssignments(Exp->getConstructor(),
                            Exp->arg_begin(), Exp->arg_end(), *SubV);
  delete SubV;
  SubV = 0;
}

void AssignmentCheckerVisitor::
helperTypecheckDeclWithInit(const ValueDecl *VD, Expr *Init) {
  TypeBuilderVisitor TBVR(BR, Ctx, Mgr, AC, OS, SymT, Def, Init);
  const ASaPType *LHSType = SymT.getType(VD);
  ASaPType *RHSType = TBVR.getType();
  //OS << "DEBUG:: gonna call typecheck(LHS,RHS, IsInit=true\n";
  if (! typecheck(LHSType, RHSType, true)) {
    OS << "DEBUG:: invalid assignment: gonna emit an error\n";
    //  Fixme pass VS as arg instead of Init
    helperEmitInvalidInitializationWarning(Init, LHSType, RHSType);
    FatalError = true;
  }
}

bool AssignmentCheckerVisitor::
typecheckSingleParamAssignment(ParmVarDecl *Param, Expr *Arg,
                         SubstitutionVector &SubV) {
  bool Result = true;
  OS << "DEBUG:: typeckeckSingleParamAssignment of arg '";
  Arg->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "' to param '";
  Param->print(OS, Ctx.getPrintingPolicy());
  OS << "'\n";
  OS << "SubstitutionVector Size = " << SubV.size() << "\n";
  OS << "SubVec: " << SubV.toString();

  TypeBuilderVisitor TBVR(BR, Ctx, Mgr, AC, OS, SymT, Def, Arg);
  const ASaPType *LHSType = SymT.getType(Param);
  ASaPType *LHSTypeMod = 0;
  if (SubV.size() > 0 && LHSType) {
    OS << "DEBUG:: gonna perform substitution\n";
    LHSTypeMod = new ASaPType(*LHSType);
    LHSTypeMod->substitute(&SubV);
    LHSType = LHSTypeMod;
    OS << "DEBUG:: DONE performing substitution\n";
  }
  ASaPType *RHSType = TBVR.getType();
  if (! typecheck(LHSType, RHSType, true)) {
    OS << "DEBUG:: invalid argument to parameter assignment: "
      << "gonna emit an error\n";
    //  Fixme pass VS as arg instead of Init
    helperEmitInvalidArgToFunctionWarning(Arg, LHSType, RHSType);
    FatalError = true;
    Result = false;
  }
  delete LHSTypeMod;
  OS << "DEBUG:: DONE with typeckeckSingleParamAssignment. Result="
     << Result <<"\n";
  return Result;
}

void AssignmentCheckerVisitor::
typecheckParamAssignments(FunctionDecl *CalleeDecl,
                          ExprIterator ArgI,
                          ExprIterator ArgE,
                          SubstitutionVector &SubV) {
  assert(CalleeDecl);
  // Build SubV for function region params
  const ParameterVector *ParamV = SymT.getParameterVector(CalleeDecl);
  if (ParamV && ParamV->size() > 0) {
    buildParamSubstitutions(CalleeDecl, ArgI, ArgE, *ParamV, SubV);
  }

  OS << "DEBUG:: CALLING typecheckParamAssignments\n";
  FunctionDecl::param_iterator ParamI, ParamE;
  for(ParamI = CalleeDecl->param_begin(), ParamE = CalleeDecl->param_end();
      ArgI != ArgE && ParamI != ParamE; ++ArgI, ++ParamI) {
    Expr *ArgExpr = *ArgI;
    ParmVarDecl *ParamDecl = *ParamI;
    typecheckSingleParamAssignment(ParamDecl, ArgExpr, SubV);
  }
  OS << "DEBUG:: DONE with typecheckParamAssignments\n";
}

void AssignmentCheckerVisitor::
typecheckCXXConstructExpr(VarDecl *VarD,
                          CXXConstructExpr *Exp,
                          SubstitutionVector &SubV) {
  CXXConstructorDecl *ConstrDecl =  Exp->getConstructor();
  DeclContext *ClassDeclContext = ConstrDecl->getDeclContext();
  assert(ClassDeclContext);
  RecordDecl *ClassDecl = dyn_cast<RecordDecl>(ClassDeclContext);
  assert(ClassDecl);
  // Set up Substitution Vector
  const ParameterVector *PV = SymT.getParameterVector(ClassDecl);
  if (PV && PV->size() > 0) {
    assert(PV->size() == 1); // until we support multiple region params
    const ParamRplElement *ParamEl = PV->getParamAt(0);
    const ASaPType *T = SymT.getType(VarD);
    if (T) {
      const Rpl *R = T->getSubstArg();
      Substitution Sub(ParamEl, R);
      OS << "DEBUG:: ConstructExpr Substitution = " << Sub.toString() << "\n";
      SubV.push_back(&Sub);
    }
  }
  typecheckParamAssignments(ConstrDecl, Exp->arg_begin(), Exp->arg_end(), SubV);
  OS << "DEBUG:: DONE with typecheckCXXConstructExpr\n";

  // Now set Type to the return type of this call
  OS << "DEBUG:: ConstrDecl:";
  ConstrDecl->print(OS, Ctx.getPrintingPolicy());
  OS << "\n";

  const ASaPType *RetTyp = SymT.getType(VarD);
  if (RetTyp) {
    OS << "DEBUG:: ConstrDecl Return Type = " << RetTyp->toString() << "\n";

    delete Type;
    // set Type
    Type = new ASaPType(*RetTyp);
    Type->substitute(&SubV);
  }
}

void AssignmentCheckerVisitor::
typecheckCallExpr(CallExpr *Exp, SubstitutionVector &SubV) {
  OS << "DEBUG:: typecheckCallExpr: ";
  Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  OS << "DEBUG:: Expr:";
  Exp->dump(OS, BR.getSourceManager());
  OS << "\n";

  Decl *D = Exp->getCalleeDecl();
  assert(D);

  FunctionDecl *FD = dyn_cast<FunctionDecl>(D);
  // TODO we shouldn't give up like this below, but doing this for now
  // to see how to fix this here problem...
  // FD could be null in the case of a dependent type in a template
  // uninstantiated (i.e., parametric) code.
  //assert(FD);
  if (!FD)
    return;
  // Set up Substitution Vector
  const ParameterVector *FD_ParamV = SymT.getParameterVector(FD);
  if (FD_ParamV && FD_ParamV->size() > 0) {
    buildParamSubstitutions(FD, Exp->arg_begin(),
                            Exp->arg_end(), *FD_ParamV, SubV);
  }

  DeclContext *DC = FD->getDeclContext();
  assert(DC);
  RecordDecl *ClassDecl = dyn_cast<RecordDecl>(DC);
  // ClassDecl is allowed to be null

  BaseTypeBuilderVisitor  TBV(BR, Ctx, Mgr, AC, OS,
        SymT, Def, Exp->getCallee());
  // Build substitution
  const ParameterVector *ParamV = SymT.getParameterVector(ClassDecl);
  if (ParamV && ParamV->size() > 0) {
    assert(ParamV->size() == 1); // until we support multiple region params
    const ParamRplElement *ParamEl = ParamV->getParamAt(0);

    ASaPType *T = TBV.getType();
    if (T) {
      OS << "DEBUG:: Base Type = " << T->toString(Ctx) << "\n";
      const Rpl *R = T->getSubstArg();
      Substitution Sub(ParamEl, R);
      OS << "DEBUG:: typecheckCallExpr Substitution = "
        << Sub.toString() << "\n";
      SubV.push_back(&Sub);
    }
  }

  typecheckParamAssignments(FD, Exp->arg_begin(), Exp->arg_end(), SubV);
  OS << "DEBUG:: DONE typecheckCallExpr\n";

  // Now set Type to the return type of this call
  const ASaPType *FunType = SymT.getType(FD);
  if (FunType) {
    assert(FunType->isFunctionType());
    ASaPType *RetTyp = new ASaPType(*FunType);
    RetTyp = RetTyp->getReturnType();
    if (RetTyp) {
      delete Type;
      // set Type
      Type = RetTyp; // RetTyp is already a copy, no need to re-copy
      Type->substitute(&SubV);
    }
  }
} // End typecheckCallExpr

void AssignmentCheckerVisitor::
buildParamSubstitutions(const FunctionDecl *CalleeDecl,
                        ExprIterator ArgI, ExprIterator ArgE,
                        const ParameterVector &ParamV,
                        SubstitutionVector &SubV) {
  assert(CalleeDecl);
  FunctionDecl::param_const_iterator ParamI, ParamE;
  for(ParamI = CalleeDecl->param_begin(), ParamE = CalleeDecl->param_end();
      ArgI != ArgE && ParamI != ParamE; ++ArgI, ++ParamI) {
    Expr *ArgExpr = *ArgI;
    ParmVarDecl *ParamDecl = *ParamI;
    buildSingleParamSubstitution(ParamDecl, ArgExpr, ParamV, SubV);
  }
}

void AssignmentCheckerVisitor::
buildSingleParamSubstitution(ParmVarDecl *Param, Expr *Arg,
                             const ParameterVector &ParamV,
                             SubstitutionVector &SubV) {
  // if param has argument that is a parameter, create a substitution
  // based on the argument
  const ASaPType *ParamType = SymT.getType(Param);
  if (!ParamType)
    return;
  const RplVector *ParamArgV = ParamType->getArgV();
  if (!ParamArgV)
    return;
  TypeBuilderVisitor TBV(BR, Ctx, Mgr, AC, OS, SymT, Def, Arg);
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
    const Rpl *ParamR = *ParamI;
    assert(ParamR && "RplVector should not contain null Rpl pointer");
    if (ParamR->length() != 1)
      continue;
    const RplElement *Elmt = ParamR->getFirstElement();
    assert(Elmt && "Rpl should not contain null RplElement pointer");
    if (! ParamV.hasElement(Elmt))
      continue;
    // Ok find the argument
    Substitution Sub(Elmt, *ArgI);
    SubV.push_back(&Sub);
    OS << "DEBUG:: added function param sub: " << Sub.toString() << "\n";
  }
}

//////////////////////////////////////////////////////////////////////////
void TypeBuilderVisitor::memberSubstitute(const ASaPType *T) {
  assert(T && "Type can't be null");
  OS << "DEBUG:: Type used for substitution = " << T->toString(Ctx) << "\n";

  QualType QT = T->getQT(DerefNum);

  const ParameterVector *ParamVec = SymT.getParameterVectorFromQualType(QT);
  if (!ParamVec || ParamVec->size() == 0)
    return;

  // TODO support multiple Parameters
  const ParamRplElement *FromEl = ParamVec->getParamAt(0);
  assert(FromEl && "FromEl can't be null");

  const Rpl *ToRpl = T->getSubstArg(DerefNum);
  assert(ToRpl && "ToRpl can't be null");
  OS << "DEBUG:: gonna substitute... " << FromEl->getName()
    << "<-" << ToRpl->toString() << "\n";

  if (*ToRpl != *FromEl) {
    OS <<" GO!!\n";
    assert(Type && "Type can't be null");
    Substitution Sub(FromEl, ToRpl);
    Type->substitute(&Sub);
  }
}

void TypeBuilderVisitor::memberSubstitute(const ValueDecl *D) {
  assert(D && "D can't be null!");
  OS << "DEBUG:: in TypeBuilder::memberSubstitute:";
  D->print(OS, Ctx.getPrintingPolicy());
  OS << "\nDEBUG:: isBase = " << (IsBase ? "true" : "false") << "\n";
  OS << "DEBUG:: DerefNum = " << DerefNum << "\n";

  const ASaPType *T = SymT.getType(D);
  if (T)
    memberSubstitute(T);
  OS << "   DONE\n";
}

void TypeBuilderVisitor::setType(const ASaPType *T) {
  assert(T && "T can't be null");

  assert(!Type && "Type must be null");
  Type = new ASaPType(*T); // make a copy
  if (DerefNum == -1)
    Type->addrOf(RefQT);
  else {
    OS << "DEBUG :: calling ASaPType::deref(" << DerefNum << ")\n";
    Type->deref(DerefNum);
    OS << "DEBUG :: DONE calling ASaPType::deref\n";
  }
  OS << "DEBUG :: set TypeBuilder::Type = " << Type->toString(Ctx) << "\n";
}

void TypeBuilderVisitor::setType(const ValueDecl *D) {
  OS << "DEBUG:: in TypeBuilder::setType: ";
  D->print(OS, Ctx.getPrintingPolicy());
  //OS << "\n Decl pointer address:" << D;
  OS << "\n";
  const ASaPType *T = SymT.getType(D);
  if (T)
    setType(T);
}

TypeBuilderVisitor::TypeBuilderVisitor (
  ento::BugReporter &BR,
  ASTContext &Ctx,
  AnalysisManager &Mgr,
  AnalysisDeclContext *AC,
  raw_ostream &OS,
  SymbolTable &SymT,
  const FunctionDecl *Def,
  Expr *E
  ) : BR(BR),
  Ctx(Ctx),
  Mgr(Mgr),
  AC(AC),
  OS(OS),
  SymT(SymT),
  Def(Def),
  FatalError(false),
  IsBase(false),
  DerefNum(0),
  Type(0) {

    OS << "DEBUG:: ******** INVOKING TypeBuilderVisitor...\n";
    E->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";

    Visit(E);
    OS << "DEBUG:: ******** DONE WITH TypeBuilderVisitor...\n";
}

TypeBuilderVisitor::~TypeBuilderVisitor() {
  //delete SubV;
  delete Type;
}

ASaPType *TypeBuilderVisitor::stealType() {
  ASaPType *Result = Type;
  Type = 0;
  return Result;
}

void TypeBuilderVisitor::VisitChildren(Stmt *S) {
  for (Stmt::child_iterator I = S->child_begin(), E = S->child_end();
    I!=E; ++I)
    if (Stmt *child = *I)
      Visit(child);
}

void TypeBuilderVisitor::VisitStmt(Stmt *S) {
  OS << "DEBUG:: GENERIC:: Visiting Stmt/Expr = \n";
  S->dump(OS, BR.getSourceManager());
  //S->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";

  VisitChildren(S);
}

void TypeBuilderVisitor::VisitUnaryAddrOf(UnaryOperator *Exp)  {
  assert(DerefNum>=0 && "Must be positive dereference number");
  DerefNum--;
  OS << "DEBUG:: Visit Unary: AddrOf (DerefNum=" << DerefNum << ") Type = ";
  Exp->getType().print(OS, Ctx.getPrintingPolicy());
  OS << "\n";

  RefQT = Exp->getType();
  assert(RefQT->isPointerType() && "Must be a pointer type here");

  Visit(Exp->getSubExpr());
  DerefNum++;
}

void TypeBuilderVisitor::VisitUnaryDeref(UnaryOperator *E) {
  DerefNum++;
  OS << "DEBUG:: Visit Unary: Deref (DerefNum=" << DerefNum << ")\n";
  Visit(E->getSubExpr());
  DerefNum--;
}

void TypeBuilderVisitor::VisitDeclRefExpr(DeclRefExpr *E) {
  OS << "DEBUG:: VisitDeclRefExpr --- whatever that is!: ";
  E->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  ValueDecl* VD = E->getDecl();
  assert(VD && "VD can't be null");
  if (IsBase)
    memberSubstitute(VD);
  else
    setType(VD);
}

void TypeBuilderVisitor::VisitCXXThisExpr(CXXThisExpr *E) {
  OS << "DEBUG:: visiting 'this' expression\n";
  assert(E && "E can't be null");
  //DerefNum = 0;
  if (!IsBase) {
    assert(!Type && "Type must be null at this place.");
    // Add parameter as implicit argument
    CXXRecordDecl *RecDecl =
      const_cast<CXXRecordDecl*>(E->getBestDynamicClassType());
    assert(RecDecl && "RecDecl can't be null");

    /* Keeping this code below as an example of how to add nodes to the AST
    /// If the declaration does not yet have an implicit region argument
    /// add it to the Declaration
    if (!RecDecl->getAttr<RegionArgAttr>()) {
    const RegionParamAttr *Param = RecDecl->getAttr<RegionParamAttr>();
    assert(Param);
    RegionArgAttr *Arg =
    ::new (RecDecl->getASTContext()) RegionArgAttr(Param->getRange(),
    RecDecl->getASTContext(),
    Param->getName());
    RecDecl->addAttr(Arg);

    /// also add it to RplMap
    RplMap[Arg] = new Rpl(new ParamRplElement(Param->getName()));
    }
    RegionArgAttr* Arg = RecDecl->getAttr<RegionArgAttr>();
    assert(Arg);
    Rpl *Tmp = RplMap[Arg];
    assert(Tmp);
    TmpRegions->push_back(new Rpl(Tmp));*/

    //const RegionParamAttr *Param = RecDecl->getAttr<RegionParamAttr>();
    //assert(Param);

    //RplElement *El = RplElementMap[Param];
    //assert(El);
    //ParamRplElement *ParamEl = dyn_cast<ParamRplElement>(El);
    //assert(ParamEl);

    const ParameterVector *ParamVec = SymT.getParameterVector(RecDecl);
    QualType ThisQT = E->getType();

    RplVector RV(*ParamVec);

    OS << "DEBUG:: adding 'this' type : ";
    ThisQT.print(OS, Ctx.getPrintingPolicy());
    OS << "\n";
    // simple==true because 'this' is an rvalue (can't have its address taken)
    // so we want to keep InRpl=0
    Type = new ASaPType(ThisQT, &RV, 0, true);
    if (DerefNum == -1)
      Type->addrOf(RefQT);
    else {
      OS << "DEBUG :: calling ASaPType::deref(" << DerefNum << ")\n";
      Type->deref(DerefNum);
      OS << "DEBUG :: DONE calling ASaPType::deref\n";
    }

    OS << "DEBUG:: type actually added: " << Type->toString(Ctx) << "\n";

    //TmpRegions->push_back(new Rpl(new ParamRplElement(Param->getName())));
  }
  OS << "DEBUG:: DONE visiting 'this' expression\n";
}

void TypeBuilderVisitor::VisitMemberExpr(MemberExpr *Exp) {
  OS << "DEBUG:: VisitMemberExpr: ";
  Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  /*OS << "Rvalue=" << E->isRValue()
  << ", Lvalue=" << E->isLValue()
  << ", Xvalue=" << E->isGLValue()
  << ", GLvalue=" << E->isGLValue() << "\n";
  Expr::LValueClassification lvc = E->ClassifyLValue(Ctx);
  if (lvc==Expr::LV_Valid)
  OS << "LV_Valid\n";
  else
  OS << "not LV_Valid\n";*/
  ValueDecl* VD = Exp->getMemberDecl();
  //VD->print(OS, Ctx.getPrintingPolicy());
  //OS << "\n";
  if (IsBase)
    memberSubstitute(VD);
  else
    setType(VD);

  // Visit Base with read semantics, then restore write semantics
  bool SavedIsBase = IsBase;
  int SavedDerefNum = DerefNum;
  DerefNum = Exp->isArrow() ? 1 : 0;
  IsBase = true;
  Visit(Exp->getBase());
  IsBase = SavedIsBase;
  DerefNum = SavedDerefNum;

} // end VisitMemberExpr

void TypeBuilderVisitor::helperBinAddSub(Expr *LHS, Expr* RHS) {
  OS << "DEBUG:: helperBinAddSub\n";
  Visit(LHS);
  ASaPType *T = stealType();
  Visit(RHS);
  if (Type)
    Type->join(T);
  else
    Type = T;
  // memory leak? do we need to dealloc T?
}


// isPtrMemOp : BO_PtrMemD || BO_PtrMemI
// isMultiplicativeOp: BO_Mul || BO_Div || BO_Rem
// isAdditiveOp: BO_Add || BO_Sub
// isShiftOp: BO_Shl || BO_Shr
// isBitwiseOp: BO_And || BO_Xor || BO_Or
// isRelationalOp: BO_LT || BO_GT || BO_LE || BO_GE
// isEqualityOp:                                       BO_EQ || BO_NE
// isComparisonOp  BO_LT || BO_GT || BO_LE || BO_GE || BO_EQ || BO_NE
// isLogicalOp: BO_LAnd || BO_LOr
// isAssignmetOp:    BO_Assign    || BO_MulAssign || BO_DivAssign || BO_RemAssign
//                || BO_AddAssign || BO_SubAssign || BO_ShlAssign || BO_ShrAssign
//                || BO_AndAssign || BO_XorAssign || BO_OrAssign
// BO_Comma
void TypeBuilderVisitor::VisitBinaryOperator(BinaryOperator* Exp) {
  OS << "Visiting Operator " << Exp->getOpcodeStr() << "\n";
  if (Exp->isPtrMemOp()) {
    // TODO
    OS << "DEBUG: iz a PtrMemOp!! ";
    Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    VisitChildren(Exp);
  } else if (Exp->isMultiplicativeOp()) {
    // TODO
    helperBinAddSub(Exp->getLHS(), Exp->getRHS());
  } else if (Exp->isAdditiveOp()) {
    helperBinAddSub(Exp->getLHS(), Exp->getRHS());
  } else if (Exp->isBitwiseOp()) {
    // TODO
    helperBinAddSub(Exp->getLHS(), Exp->getRHS());
  } else if (Exp->isComparisonOp() || Exp->isLogicalOp()) {
    // BO_LT || BO_NE
    assert(!Type && "Type must be null");
    Rpl LOCALRpl(*SymbolTable::LOCAL_RplElmt);
    QualType QT = Exp->getType();
    OS << "DEBUG:: QT = ";
    QT.print(OS, Ctx.getPrintingPolicy());
    OS << "\n";
    Type = new ASaPType(QT, 0, &LOCALRpl);
    OS << "DEBUG:: (VisitComparisonOrLogicalOp) Type = " << Type->toString() << "\n";
    AssignmentCheckerVisitor
      ACVR(BR, Ctx, Mgr, AC, OS, SymT, Def, Exp->getRHS());
    AssignmentCheckerVisitor
      ACVL(BR, Ctx, Mgr, AC, OS, SymT, Def, Exp->getLHS());
    OS << "DEBUG:: (VisitComparisonOrLogicalOp) Type = " << Type->toString() << "\n";
  } else if (Exp->isAssignmentOp()) {
    OS << "DEBUG:: >>>>>>>>>>VisitBinOpAssign<<<<<<<<<<<<<<<<<\n";
    Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";

    AssignmentCheckerVisitor ACV(BR, Ctx, Mgr, AC, OS, SymT, Def, Exp);
    assert(!Type && "Type must be null here");
    Type = ACV.stealType();
    assert(Type && "Type must not be null here");
  } else {
    // CommaOp
    Visit(Exp->getRHS()); // visit to typeckeck possible assignments
    delete Type; Type = 0; // discard results
    Visit(Exp->getLHS());
  }
}

void TypeBuilderVisitor::VisitConditionalOperator(ConditionalOperator *Exp) {
  OS << "DEBUG:: @@@@@@@@@@@@VisitConditionalOp@@@@@@@@@@@@@@\n";
  Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  AssignmentCheckerVisitor ACV(BR, Ctx, Mgr, AC, OS,
    SymT, Def, Exp->getCond());
  FatalError |= ACV.encounteredFatalError();

  assert(!Type && "Type must be null here");
  OS << "DEBUG:: Visiting Cond LHS\n";
  Visit(Exp->getLHS());
  OS << "DEBUG:: DONE Visiting Cond LHS\n";
  ASaPType *LHSType = stealType();

  OS << "DEBUG:: Visiting Cond RHS\n";
  Visit(Exp->getRHS());
  OS << "DEBUG:: DONE Visiting Cond RHS\n";
  if (Type)
    Type->join(LHSType);
  else
    Type = LHSType;
  OS << "DEBUG:: Joining Cond LHS & RHS\n";

}

void TypeBuilderVisitor::
VisitBinaryConditionalOperator(BinaryConditionalOperator *Exp) {
  OS << "DEBUG:: @@@@@@@@@@@@VisitConditionalOp@@@@@@@@@@@@@@\n";
  Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  // TODO?
}

//void TypeBuilderVisitor::VisitCXXNewExpr(CXXNewExpr *Exp) {
//}

void TypeBuilderVisitor::VisitCXXConstructExpr(CXXConstructExpr *Exp) {
  OS << "DEBUG:: VisitCXXConstructExpr:";
  Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  // Call AssignmentChecker recursively
  AssignmentCheckerVisitor
    ACV(BR, Ctx, Mgr, AC, OS, SymT, Def, Exp);
  // CXXConstruct Expr return Types without region constraints.
  // The region is fresh. Think of it as an object with
  // parametric region that gets unified based on the region args
  // of the variable that gets initialized. It's like saying that
  // a constructor returns T<P>.

}

void TypeBuilderVisitor::VisitCallExpr(CallExpr *Exp) {
  OS << "DEBUG:: VisitCallExpr:";
  Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  // Call AssignmentChecker recursively
  AssignmentCheckerVisitor
    ACV(BR, Ctx, Mgr, AC, OS, SymT, Def, Exp);

  OS << "DEBUG:: isBase = " << IsBase << "\n";
  ASaPType *T = ACV.getType();
  if (IsBase) {
    memberSubstitute(T);
  } else {
    setType(T);
  }
}

void TypeBuilderVisitor::VisitArraySubscriptExpr(ArraySubscriptExpr *Exp) {
  // Visit index expression in case we need to typecheck assignments
  AssignmentCheckerVisitor
    ACV(BR, Ctx, Mgr, AC, OS, SymT, Def, Exp->getIdx());
  // For now ignore the index type
  Visit(Exp->getBase());
}

void TypeBuilderVisitor::VisitReturnStmt(ReturnStmt *Ret) {
  assert(false && "TypeBuilder should not be called on ReturnStmt");
  return;
}

//////////////////////////////////////////////////////////////////////////

BaseTypeBuilderVisitor::BaseTypeBuilderVisitor(
  ento::BugReporter &BR,
  ASTContext &Ctx,
  AnalysisManager &Mgr,
  AnalysisDeclContext *AC,
  raw_ostream &OS,
  SymbolTable &SymT,
  const FunctionDecl *Def,
  Expr *E
  ) : BR(BR),
  Ctx(Ctx),
  Mgr(Mgr),
  AC(AC),
  OS(OS),
  SymT(SymT),
  Def(Def),
  FatalError(false),
  Type(0) {

    OS << "DEBUG:: ******** INVOKING BaseTypeBuilderVisitor...\n";
    E->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";

    Visit(E);
    OS << "DEBUG:: ******** DONE WITH BaseTypeBuilderVisitor...\n";
}

BaseTypeBuilderVisitor::~BaseTypeBuilderVisitor() {
  delete Type;
}

ASaPType *BaseTypeBuilderVisitor::stealType() {
  ASaPType *Result = Type;
  Type = 0;
  return Result;
}

void BaseTypeBuilderVisitor::VisitChildren(Stmt *S) {
  for (Stmt::child_iterator I = S->child_begin(), E = S->child_end();
    I!=E; ++I)
    if (Stmt *child = *I)
      Visit(child);
}

void BaseTypeBuilderVisitor::VisitStmt(Stmt *S) {
  OS << "DEBUG:: GENERIC:: Visiting Stmt/Expr = \n";
  S->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  VisitChildren(S);
}

void BaseTypeBuilderVisitor::VisitMemberExpr(MemberExpr *Exp) {
  OS << "DEBUG:: VisitMemberExpr: ";
  Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  TypeBuilderVisitor TBV(BR, Ctx, Mgr, AC, OS, SymT, Def, Exp->getBase());
  Type = TBV.stealType();
  if (Exp->isArrow())
    Type->deref(1);
}




