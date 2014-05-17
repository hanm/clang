//=== TypeChecker.cpp - Safe Parallelism checker -----*- C++ -*------===//
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

#include "TypeChecker.h"

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Type.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugReporter.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/AnalysisManager.h"

#include "llvm/Support/SaveAndRestore.h"

#include "ASaPUtil.h"
#include "ASaPSymbolTable.h"
#include "ASaPType.h"
#include "Effect.h"
#include "Rpl.h"
#include "Substitution.h"

namespace clang {
namespace asap {

static void emitUnsafeExplicitCastWarning(Expr *Exp, StringRef FromTo) {
  StringRef BugName = "unsafe explicit cast";
  helperEmitStatementWarning(SymbolTable::VB.Checker,
                             *SymbolTable::VB.BR,
                             SymbolTable::VB.AC, Exp,
                             0, FromTo, BugName, false);
}

static void emitUnsafeImplicitCastWarning(Expr *Exp, StringRef FromTo) {
  StringRef BugName = "unsafe implicit cast";
  helperEmitStatementWarning(SymbolTable::VB.Checker,
                             *SymbolTable::VB.BR,
                             SymbolTable::VB.AC,
                             Exp, 0, FromTo, BugName, false);
}

AssignmentCheckerVisitor::AssignmentCheckerVisitor(
  const FunctionDecl *Def,
  Stmt *S,
  bool VisitCXXInitializer
  ) : BaseClass(Def),
      Type(0), SubV(0) {

    OS << "DEBUG:: ******** INVOKING AssignmentCheckerVisitor...(VisitInit="
       << (VisitCXXInitializer?"ture":"false") << ")\n";
    if (!BR.getSourceManager().isInMainFile(Def->getLocation())) {
      OS << "DEBUG::TypeChecker::Skipping Declaration that is not in main compilation file\n";
      return;
    }
    OS << "DEBUG:: Stmt:";
    S->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    // S->dump();
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
    OS << "DEBUG:: ******** DONE INVOKING AssignmentCheckerVisitor (Type="
       << (Type ? Type->toString() : "<null>") << ")***\n";
}

AssignmentCheckerVisitor::~AssignmentCheckerVisitor() {
  delete Type;
}

ASaPType *AssignmentCheckerVisitor::stealType() {
  ASaPType *Result = Type;
  Type = 0;
  return Result;
}

void AssignmentCheckerVisitor::
VisitCallExpr(CallExpr *Exp) {
  if (!Exp->getBuiltinCallee()) {
    assert(!SubV);
    SubV = new SubstitutionVector();
    typecheckCallExpr(Exp, *SubV);
    delete SubV;
    SubV = 0;
  }
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
        OS << "\n VarDecl isDependentType ? "
          << (VD->getType()->isDependentType() ? "true" : "false") << "\n";
        OS << "\n Init Expr = ";
        Init->printPretty(OS, 0, Ctx.getPrintingPolicy());
        OS << "\n";
        Init->dump(OS, BR.getSourceManager());
        OS << "\n";

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
          if (VD->getType()->isDependentType() && !Exp)
            break; // VD->getInit() could be a ParenListExpr or perhaps some
                   // other Expr
          assert(Exp);
          assert(!SubV);
          SubV = new SubstitutionVector();
          typecheckCXXConstructExpr(VD, Exp, *SubV);
          delete SubV;
          SubV = 0;
          break;
        }
      }
    } // end if VarDecl
  } // end for each declaration
}

bool AssignmentCheckerVisitor::
typecheck(const ASaPType *LHSType, const ASaPType *RHSType, bool IsInit) {
  if (!LHSType)
    return true; // LHS has no region info (e.g., type cast). Don't type check
  if (!RHSType)
    return true; // RHS has no region info && Clang has done typechecking
  else { // RHSType != null
    OS << "DEBUG:: RHS isDependentType? "
       << (RHSType->getQT()->isDependentType() ? "true" : "false") << "\n";
    OS << "DEBUG:: LHS isDependentType? "
       << (LHSType->getQT()->isDependentType() ? "true" : "false") << "\n";

    return RHSType->isAssignableTo(*LHSType, SymT, Ctx, IsInit);
  }
}

void AssignmentCheckerVisitor::
helperEmitInvalidArgToFunctionWarning(const Stmt *S,
                                      const ASaPType *LHS,
                                      const ASaPType *RHS) {
  StringRef BugName = "invalid argument to function call";
  helperEmitInvalidAssignmentWarning(Checker, BR, AC, Ctx,
                                     S, LHS, RHS, BugName);
}

void AssignmentCheckerVisitor::
helperEmitInvalidExplicitAssignmentWarning(const Stmt *S,
                                           const ASaPType *LHS,
                                           const ASaPType *RHS) {
  StringRef BugName = "invalid assignment";
  helperEmitInvalidAssignmentWarning(Checker, BR, AC, Ctx,
                                     S, LHS, RHS, BugName);
}

void AssignmentCheckerVisitor::
helperEmitInvalidReturnTypeWarning(const Stmt *S,
                                   const ASaPType *LHS,
                                   const ASaPType *RHS) {
  StringRef BugName = "invalid return type";
  helperEmitInvalidAssignmentWarning(Checker, BR, AC, Ctx,
                                     S, LHS, RHS, BugName);
}

void AssignmentCheckerVisitor::
helperEmitInvalidInitializationWarning(const Stmt *S,
                                       const ASaPType *LHS,
                                       const ASaPType *RHS) {
  StringRef BugName = "invalid initialization";
  helperEmitInvalidAssignmentWarning(Checker, BR, AC, Ctx,
                                     S, LHS, RHS, BugName);
}

void AssignmentCheckerVisitor::
helperEmitUnsupportedConstructorInitializer(const CXXConstructorDecl *D) {
  StringRef BugName = "unsupported constructor initializer."
    " Please file feature support request.";
  helperEmitDeclarationWarning(Checker, BR, D, "", BugName, false);
}

void AssignmentCheckerVisitor::
helperVisitCXXConstructorDecl(const CXXConstructorDecl *D) {
  CXXConstructorDecl::init_const_iterator
    I = D->init_begin(),
    E = D->init_end();
  for(; I != E; ++I) {
    CXXCtorInitializer *Init = *I;
    if (Init->isMemberInitializer()) {
      OS << "DEBUG::helperVisitCXXConstructorDecl::isMemberInitializer\n";
      helperTypecheckDeclWithInit(Init->getMember(), Init->getInit());
    } else if (Init->isBaseInitializer()) {
      OS << "DEBUG::helperVisitCXXConstructorDecl::isBaseInitializer\n";
      Expr *E = Init->getInit();
      E->printPretty(OS, 0, Ctx.getPrintingPolicy());
      OS << "\n";
      E->dump(OS, BR.getSourceManager());
      OS << "\n";
      //const class Type *BaseClass = Init->getBaseClass();
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
  TypeBuilderVisitor TBVR(Def, E->getRHS());
  TypeBuilderVisitor TBVL(Def, E->getLHS());
  OS << "DEBUG:: Ran type builder on RHS & LHS\n";
  E->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  ASaPType *LHSType = TBVL.getType();
  ASaPType *RHSType = TBVR.getType();

  // allow RHSType to be NULL, e.g., we don't create ASaP Types for constants
  // because they don't have any interesting regions to typecheck.
  if (!typecheck(LHSType, RHSType, false)) {
    OS << "DEBUG:: invalid assignment: gonna emit an error\n";
    helperEmitInvalidExplicitAssignmentWarning(E, LHSType, RHSType);
    FatalError = true;
  }

  // The type of the assignment is the type of the LHS. Set it in case
  // AssignmentChecker was called recursively by a TypeBuilderVisitor
  delete Type;
  Type = TBVL.stealType();
  //assert(Type);

  OS << "DEBUG:: >>>>>>>>>> DONE TYPECHECKING BinAssign<<<<<<<<<<<<<<<<<\n";
}

void AssignmentCheckerVisitor::VisitReturnStmt(ReturnStmt *Ret) {
  if (!Ret->getRetValue())
    return; // this is a 'return' statement with no return expression

  if (Def->getType()->isDependentType())
    return; // do nothing if the function is a template.

  Expr *RetExp = Ret->getRetValue();
  OS << "DEBUG:: Visiting ReturnStmt (" << Ret << "). RetExp ("
     << RetExp << "): ";
  RetExp->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";

  TypeBuilderVisitor TBVR(Def, RetExp);
  if (!TBVR.getType())
    return;

  const ASaPType *FunType = SymT.getType(Def);
  Def->dump(OS);
  OS << "\n";
  assert(FunType);
  assert(FunType->isFunctionType());
  ASaPType *LHSType = new ASaPType(*FunType);
  LHSType = LHSType->getReturnType();
  ASaPType *RHSType = TBVR.getType();
  if (!typecheck(LHSType, RHSType, true)) {
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
  TypeBuilderVisitor TBVR(Def, Init);
  const ASaPType *LHSType = SymT.getType(VD);
  ASaPType *RHSType = TBVR.getType();
  //OS << "DEBUG:: gonna call typecheck(LHS,RHS, IsInit=true\n";
  if (!typecheck(LHSType, RHSType, true)) {
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
  OS << "SubVec: " << SubV.toString() << "\n";

  TypeBuilderVisitor TBVR(Def, Arg);
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
  if (!typecheck(LHSType, RHSType, true)) {
    OS << "DEBUG:: invalid argument to parameter assignment: "
      << "gonna emit an error\n";
    OS << "DEBUG:: Param:";
    Param->print(OS);
    OS << " with type " << (LHSType ? LHSType->toString() : "[]") << "\n";
    OS << "DEBUG:: Arg:";
    Arg->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << " with Type " << (RHSType ? RHSType->toString() : "[]") << "\n";
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
  ParameterSet *ParamSet = new ParameterSet();
  assert(ParamSet);
  // Build SubV for function region params
  const ParameterVector *ParamV = SymT.getParameterVector(CalleeDecl);
  // FIXME: if isa<CXXMethodDecl>CalleeDecl -> add Class parameters to vector
  if (ParamV && ParamV->size() > 0) {
    ParamV->addToParamSet(ParamSet);
  }
  if (CXXMethodDecl *CXXCalleeDecl = dyn_cast<CXXMethodDecl>(CalleeDecl)) {
    CXXRecordDecl *Rec = CXXCalleeDecl->getParent();
    ParamV = SymT.getParameterVector(Rec);
    if (ParamV && ParamV->size() > 0) {
      ParamV->addToParamSet(ParamSet);
    }
  }
  if (ParamSet->size() > 0) {
    buildParamSubstitutions(CalleeDecl, ArgI, ArgE, *ParamSet, SubV);
  }
  delete ParamSet;

  OS << "DEBUG:: CALLING typecheckParamAssignments\n";
  FunctionDecl::param_iterator
    ParamI = CalleeDecl->param_begin(),
    ParamE = CalleeDecl->param_end();

  if (CalleeDecl->isOverloadedOperator()) {
    if (isa<CXXMethodDecl>(CalleeDecl)) {
      // if the overloaded operator is a member function, it's 1st
      // (implicit) argument is 'this' which doesn't have a corresponding
      // parameter, so skip it!
      assert(ArgI!=ArgE);
      OS << "DEBUG:: Callee is Overloaded Operator -> skipping 1st arg:";
      ArgI->printPretty(OS, 0, Ctx.getPrintingPolicy());
      OS << ", with Type: " << ArgI->getType().getAsString();
      OS << "\n";
      ++ArgI;
    }
  }
  for(; ArgI != ArgE && ParamI != ParamE; ++ArgI, ++ParamI) {
    Expr *ArgExpr = *ArgI;
    ParmVarDecl *ParamDecl = *ParamI;
    typecheckSingleParamAssignment(ParamDecl, ArgExpr, SubV);
  }
  //assert(ParamI=ParamE); // there may be parameters w. default values
  // FIXME assert that remaining params take default args.
  if (!CalleeDecl->isVariadic())
    assert(ArgI==ArgE);
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
  //Exp->dump(OS, BR.getSourceManager());
  Exp->dump(OS, BR.getSourceManager());
  OS << "\n";

  Decl *D = Exp->getCalleeDecl();
  if (D) {
    OS << "DEBUG:: CalleeExpr(" << D << "):";
    D->dump(OS);
    OS << "\n";
  } else { // D == null
    OS << "DEBUG:: CalleeExpr(" << D << ")\n";
  }

  if (Exp->getType()->isDependentType())
    return; // Don't check

  // First Visit/typecheck potential sub-assignments in base expression
  BaseTypeBuilderVisitor TBV(Def, Exp->getCallee());

  if (isa<CXXPseudoDestructorExpr>(Exp->getCallee()))
    return; // Don't check if this is a pseudo destructor.

  assert(D);
  OS << "DEBUG:: CalleeDecl: ";
  D->print(OS, Ctx.getPrintingPolicy());
  OS << "\n";

  FunctionDecl *FunD = dyn_cast<FunctionDecl>(D);
  VarDecl *VarD = dyn_cast<VarDecl>(D); // Non-null if calling through fn-ptr
  assert(FunD || VarD);
  if (FunD) {
    // Use the cannonical decl for annotations
    FunctionDecl *CanD = FunD->getCanonicalDecl();
    if (CanD)
      FunD = CanD;

    DeclContext *DC = FunD->getDeclContext();
    assert(DC);
    RecordDecl *ClassDecl = dyn_cast<RecordDecl>(DC);
    // ClassDecl is allowed to be null

    // Build substitution for class region parameter(s)
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
        OS << "DEBUG:: SubVec: " << SubV.toString() << "\n";
      }
    }
    unsigned NumArgs = Exp->getNumArgs();
    unsigned NumParams = FunD->getNumParams();
    OS << "DEBUG:: NumArgs=" << NumArgs << ", NumParams=" << NumParams << "\n";
    OS << "DEBUG:: isOverloadedOperator: " << (FunD->isOverloadedOperator() ? "true":"false")
       << ", isVariadic: " << (FunD->isVariadic() ? "true" : "false") << "\n";
    OS << "DEBUG:: FunD:";
    FunD->print(OS, Ctx.getPrintingPolicy());
    OS << "\n";
    assert((FunD->isVariadic() || NumParams == NumArgs ||
          NumParams+((FunD->isOverloadedOperator()) ? 1 : 0) == NumArgs) &&
          "Unexpected number of arguments to a call expresion");
    typecheckParamAssignments(FunD, Exp->arg_begin(), Exp->arg_end(), SubV);
    OS << "DEBUG:: DONE typecheckCallExpr\n";

    // Now set Type to the return type of this call
    const ASaPType *FunType = SymT.getType(FunD);
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
  // end if (FunD)
  } else { // VarD != null (call through function pointer)
    // TODO
  }
} // End typecheckCallExpr

void AssignmentCheckerVisitor::
buildParamSubstitutions(const FunctionDecl *CalleeDecl,
                        ExprIterator ArgI, ExprIterator ArgE,
                        const ParameterSet &ParamSet,
                        SubstitutionVector &SubV) {
  assert(CalleeDecl);
  FunctionDecl::param_const_iterator ParamI, ParamE;
  for(ParamI = CalleeDecl->param_begin(), ParamE = CalleeDecl->param_end();
      ArgI != ArgE && ParamI != ParamE; ++ArgI, ++ParamI) {
    Expr *ArgExpr = *ArgI;
    ParmVarDecl *ParamDecl = *ParamI;
    buildSingleParamSubstitution(ParamDecl, ArgExpr, ParamSet, SubV);
  }
}

void AssignmentCheckerVisitor::
buildSingleParamSubstitution(
    ParmVarDecl *Param, Expr *Arg,
    const ParameterSet &ParamSet, // Set of fn & class region params
    SubstitutionVector &SubV) {
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
    const Rpl *ParamR = *ParamI;
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
    if (! ParamSet.hasElement(Elmt))
      continue;
    // Ok find the argument
    Substitution Sub(Elmt, *ArgI);
    SubV.push_back(&Sub);
    OS << "DEBUG:: added function param sub: " << Sub.toString() << "\n";
  }
}


//////////////////////////////////////////////////////////////////////////
// TypeBuilderVisitor

void TypeBuilderVisitor::memberSubstitute(const ASaPType *T) {
  assert(T && "Type can't be null");
  OS << "DEBUG:: Type used for substitution = " << T->toString(Ctx) << "\n";

  QualType QT = T->getQT(DerefNum);

  const ParameterVector *ParamVec = SymT.getParameterVectorFromQualType(QT);
  if (!ParamVec || ParamVec->size() == 0)
    return;

  // First, compute inheritance induced substitutions
  const SubstitutionVector *InheritanceSubV =
      SymT.getInheritanceSubVec(QT);
  Type->substitute(InheritanceSubV);

  // Next, build&apply SubstitutionVector
  RplVector RplVec;
  for (size_t I = 0; I < ParamVec->size(); ++I) {
    const Rpl *ToRpl = T->getSubstArg(DerefNum+I);
    assert(ToRpl);
    RplVec.push_back(ToRpl);
  }
  SubstitutionVector SubV;
  SubV.buildSubstitutionVector(ParamVec, &RplVec);
  Type->substitute(&SubV);
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
  OS << "DEBUG:: in TypeBuilder::setType(T): " << T->toString() << "\n";

  if (Type)
    OS << "DEBUG:: <TypeBuilderVisitor::setType(T)>: type already set:" << Type->toString() << "\n";
  assert(!Type && "Type must be null");
  Type = new ASaPType(*T); // make a copy

  if (Type->getQT()->isReferenceType()) {
      OS << "DEBUG::<TypeBuilderVisitor::setType> Type->isReferenceType()==true\n";
      Type->deref();
  }

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
  OS << "DEBUG:: in TypeBuilder::setType(D): ";
  D->print(OS, Ctx.getPrintingPolicy());
  //OS << "\n Decl pointer address:" << D;
  OS << "\n";
  const ASaPType *T = SymT.getType(D);
  if (T) {
    setType(T);
  }
}

void TypeBuilderVisitor::helperVisitLogicalExpression(Expr *Exp) {
  if (! Exp->getType()->isDependentType() ) {
    assert(!Type && "Type must be null");
    Rpl LOCALRpl(*SymbolTable::LOCAL_RplElmt);
    QualType QT = Exp->getType();
    OS << "DEBUG:: QT = ";
    QT.print(OS, Ctx.getPrintingPolicy());
    OS << "\n";
    Type = new ASaPType(QT, 0, 0, &LOCALRpl);
    OS << "DEBUG:: (VisitLogicalNotOp) Type = "
       << Type->toString() << "\n";
  }
}

void TypeBuilderVisitor::helperBinAddSub(BinaryOperator* Exp) {
  TypeBuilderVisitor ASVL(Def, Exp->getLHS());
  TypeBuilderVisitor ASVR(Def, Exp->getRHS());
  QualType QT = Exp->getType();
  OS << "DEBUG::<TypeBuilder::helperBinAddSub> Type:" << QT.getAsString()
     << "\n";

  if (QT->isDependentType()){
    clearType();
    return; // do nothing
  } else if (QT->isPointerType()) {
    // find which of the two sides is a pointer type and use that
    const ASaPType *LHSType = ASVL.getType();
    if (LHSType && LHSType->getQT()->isPointerType()) {
      assert(!Type);
      Type = ASVL.stealType();
      return;
    } //else

    const ASaPType *RHSType = ASVL.getType();
    if (RHSType && RHSType->getQT()->isPointerType()) {
      assert(!Type);
      Type = ASVR.stealType();
      return;
    } //else
    // Nothing, the pointer type subexpression may have an explicit cast
    // resulting in the TypeBuilder Visitor's 'Type' to be erased.
    //assert(false && "expression evaluates to pointer but neither "
    //       "side (LHS or RHS) returned a valid pointer type");
  } else if (QT->isScalarType()) { // but isn't pointer type
    return; // do nothing (we could also build a default Type in Local)
  }
}



TypeBuilderVisitor::
TypeBuilderVisitor (const FunctionDecl *Def, Expr *E)
  : BaseClass(Def), IsBase(false), DerefNum(0), Type(0) {

  // Detecting options
  WarnUnsafeCasts = Mgr.getAnalyzerOptions().getBooleanOption("-asap-warn-unsafe-casts", false);

  OS << "DEBUG:: ******** INVOKING TypeBuilderVisitor...(" << E << ")\n";
  E->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";

  Visit(E);

  OS << "DEBUG:: ******** DONE WITH TypeBuilderVisitor (Type="
     << (Type ? Type->toString() : "<null>") << ")***\n";
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

void TypeBuilderVisitor::VisitUnaryAddrOf(UnaryOperator *Exp)  {
  assert(DerefNum>=0 && "Must be positive dereference number");
  SaveAndRestore<int> DecrementDerefNum(DerefNum, DerefNum-1);
  OS << "DEBUG:: Visit Unary: AddrOf (DerefNum=" << DerefNum << ") Type = ";
  Exp->getType().print(OS, Ctx.getPrintingPolicy());
  OS << "\n";

  RefQT = Exp->getType();
  assert((RefQT->isDependentType() || RefQT->isPointerType())
         && "Must be a pointer type or a dependent type here");

  Visit(Exp->getSubExpr());
}

void TypeBuilderVisitor::VisitUnaryDeref(UnaryOperator *Exp) {
  SaveAndRestore<int> IncreaseDerefNum(DerefNum, DerefNum+1);
  OS << "DEBUG:: Visit Unary: Deref (DerefNum=" << DerefNum << ")\n";
  Visit(Exp->getSubExpr());
}

void TypeBuilderVisitor::VisitUnaryLNot(UnaryOperator *Exp) {
  OS << "DEBUG:: Visit Unary: Logical Not\n";
  helperVisitLogicalExpression(Exp);
  AssignmentCheckerVisitor ACV(Def, Exp->getSubExpr());
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

void TypeBuilderVisitor::VisitCXXThisExpr(CXXThisExpr *Exp) {
  OS << "DEBUG:: visiting 'this' expression\n";
  assert(Exp && "Exp can't be null");
  //DerefNum = 0;
  if (!IsBase) {
    if (!Exp->getType()->isDependentType()) {
      assert(!Type && "Type must be null at this place.");
      // Add parameter as implicit argument
      CXXRecordDecl *RecDecl =
        const_cast<CXXRecordDecl*>(Exp->getBestDynamicClassType());
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
      QualType ThisQT = Exp->getType();

      RplVector RV(*ParamVec);

      OS << "DEBUG:: adding 'this' type : ";
      ThisQT.print(OS, Ctx.getPrintingPolicy());
      OS << "\n";
      // simple==true because 'this' is an rvalue (can't have its address taken)
      // so we want to keep InRpl=0
      Type = new ASaPType(ThisQT, SymT.getInheritanceMap(RecDecl), &RV, 0, true);
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
  } else { // IsBase == true
    const SubstitutionVector *InheritanceSubV =
        SymT.getInheritanceSubVec(Exp->getType()->getPointeeType());
    Type->substitute(InheritanceSubV);
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
  SaveAndRestore<bool> VisitBase(IsBase, true);
  SaveAndRestore<int> ResetDerefNum(DerefNum, (Exp->isArrow() ? 1 : 0));
  Visit(Exp->getBase());

} // end VisitMemberExpr


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
  OS << "Expression Type:" << Exp->getType().getAsString() << "\n";
  if (Exp->isPtrMemOp()) {
    // TODO
    OS << "DEBUG: iz a PtrMemOp!! ";
    Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    VisitChildren(Exp);
  } else if (Exp->isMultiplicativeOp()) {
    // TODO
    helperBinAddSub(Exp);
  } else if (Exp->isAdditiveOp()) {
    helperBinAddSub(Exp);
  } else if (Exp->isBitwiseOp()) {
    // TODO
    helperBinAddSub(Exp);
  } else if (Exp->isComparisonOp() || Exp->isLogicalOp()) {
    // BO_LT || ... || BO_NE
    helperVisitLogicalExpression(Exp);

    AssignmentCheckerVisitor ACVR(Def, Exp->getRHS());
    AssignmentCheckerVisitor ACVL(Def, Exp->getLHS());
  } else if (Exp->isAssignmentOp()) {
    OS << "DEBUG:: >>>>>>>>>>VisitBinOpAssign<<<<<<<<<<<<<<<<<\n";
    Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";

    AssignmentCheckerVisitor ACV(Def, Exp);
    assert(!Type && "Type must be null here");
    Type = ACV.stealType();
    //assert(Type && "Type must not be null here");
  } else {
    // CommaOp
    Visit(Exp->getRHS()); // visit to typeckeck possible assignments
    clearType();
    Visit(Exp->getLHS());
  }
}

void TypeBuilderVisitor::VisitConditionalOperator(ConditionalOperator *Exp) {
  OS << "DEBUG:: @@@@@@@@@@@@VisitConditionalOp@@@@@@@@@@@@@@\n";
  Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  AssignmentCheckerVisitor ACV(Def, Exp->getCond());
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

void TypeBuilderVisitor::VisitCXXConstructExpr(CXXConstructExpr *Exp) {
  OS << "DEBUG:: VisitCXXConstructExpr:";
  Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  // Call AssignmentChecker recursively
  AssignmentCheckerVisitor ACV(Def, Exp);
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
  AssignmentCheckerVisitor ACV(Def, Exp);

  OS << "DEBUG::<TypeBuilder::VisitCallExpr> isBase = " << IsBase << "\n";
  ASaPType *T = ACV.getType();
  if (T) {
    if (IsBase) {
      memberSubstitute(T);
    } else {
      setType(T);
    }
  }
}

void TypeBuilderVisitor::VisitArraySubscriptExpr(ArraySubscriptExpr *Exp) {
  // Visit index expression in case we need to typecheck assignments
  OS << "DEBUG::<TypeBuilderVisitor::VisitArraySubscriptExpr>::\n";
  OS << "     IdxExpr:";
  Exp->getIdx()->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  OS << "     BaseExpr:";
  Exp->getBase()->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";

  AssignmentCheckerVisitor
    ACV(Def, Exp->getIdx());
  // For now ignore the index type

  OS << "DEBUG:: BaseExpType=" << Exp->getBase()->getType().getAsString() << "\n";
  if (Exp->getBase()->getType()->isDependentType() &&
      !(Exp->getBase()->getType()->isPointerType() ||
        Exp->getBase()->getType()->isArrayType())) {
    // Sometimes it is not possible to know which is the base and which
    // is the index expression in e1[e2] (e.g., if the types of both are
    // template parameters.
    Visit(Exp->getBase());
  } else {
    SaveAndRestore<int> IncreaseDerefNum(DerefNum, DerefNum+1);
    Visit(Exp->getBase());
  }
}

void TypeBuilderVisitor::VisitReturnStmt(ReturnStmt *Ret) {
  assert(false && "TypeBuilder should not be called on ReturnStmt");
  return;
}

void TypeBuilderVisitor::VisitCastExpr(CastExpr *Exp) {
  OS << "DEBUG<TypeBuilder>:: Visiting Cast Expression!! ";
  Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  OS << "DEBUG<TypeBuilder>:: Cast Kind Name : " << Exp->getCastKindName()
     << "\n";

  Visit(Exp->getSubExpr());
}

void TypeBuilderVisitor::VisitExplicitCastExpr(ExplicitCastExpr *Exp) {
  OS << "DEBUG<TypeBuilder>:: Visiting ExplicitCast Expression!! ";
  Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  OS << "DEBUG<TypeBuilder>:: Cast Kind Name : " << Exp->getCastKindName()
     << "\n";
  OS << "DEBUG<TypeBuilder>:: Cast To Type : " << Exp->getType().getAsString()
     << "\n";
  OS << "DEBUG<TypeBuilder>:: Cast From Type : "
     << Exp->getSubExpr()->getType().getAsString()
     << "\n";

  Visit(Exp->getSubExpr());
  if (Type) {
    //QualType CastQT = Exp->getType();

    switch(Exp->getCastKind()) {
    case CK_LValueToRValue:
    case CK_NoOp:
      // NoOp
      break;
    default:
      clearType();
      if (WarnUnsafeCasts) {
        std::string FromTo;
        llvm::raw_string_ostream FromToOS(FromTo);
        FromToOS << "From Type: " << Exp->getSubExpr()->getType().getAsString()
                << ", To Type: " << Exp->getType().getAsString()
                << " [Kind: " << Exp->getCastKindName() << "]";
        emitUnsafeExplicitCastWarning(Exp, FromToOS.str());
      }
    }
  }
  // do not visit sub-expression
}

void TypeBuilderVisitor::VisitImplicitCastExpr(ImplicitCastExpr *Exp) {
  OS << "DEBUG<TypeBuilder>:: Visiting Implicit Cast Expression!! ";
  Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  OS << "DEBUG<TypeBuilder>:: Cast Kind Name : " << Exp->getCastKindName()
     << "\n";
  OS << "DEBUG<TypeBuilder>:: Cast To Type   : " << Exp->getType().getAsString()
     << "\n";
  OS << "DEBUG<TypeBuilder>:: Cast From Type : "
     << Exp->getSubExpr()->getType().getAsString()
     << "\n";

  Visit(Exp->getSubExpr());
  if (Type) {
    QualType CastQT = Exp->getType();

    switch(Exp->getCastKind()) {
    case CK_IntegralCast:
    case CK_IntegralToBoolean:
    case CK_IntegralToFloating:
    case CK_FloatingCast:
    case CK_FloatingToIntegral:
    case CK_FloatingToBoolean:
    case CK_FloatingRealToComplex:
    case CK_FloatingComplexToReal:
    case CK_FloatingComplexToBoolean:
    case CK_FloatingComplexCast:
    case CK_FloatingComplexToIntegralComplex:
    case CK_IntegralRealToComplex:
    case CK_IntegralComplexCast:
    case CK_IntegralComplexToBoolean:
    case CK_IntegralComplexToReal:
    case CK_IntegralComplexToFloatingComplex:
      Type->setQT(CastQT);
      OS << "DEBUG:: ImplicitCast: Setting QT to " << CastQT.getAsString() << "\n";
      OS << "DEBUG:: Type = " << Type->toString() << "\n";
      break;
    case CK_ArrayToPointerDecay:
      {
        QualType AdjustedCastQT = ASaPType::deref(CastQT, DerefNum, Ctx);
        OS << "DEBUG:: ImplicitCast: Setting QT to " << AdjustedCastQT.getAsString() << "\n";
        OS << "DEBBG:: DerefNum=" << DerefNum << ", CastQT=" << CastQT.getAsString() << "\n";
        OS << "DEBUG:: Type = " << Type->toString() << "\n";

        Type->setQT(AdjustedCastQT);
        // In RPL of array is empty because it is immutable
      }
      break;
    case CK_PointerToBoolean:
      Type->setQT(CastQT);
      Type->dropArgV();
      OS << "DEBUG:: ImplicitCast: Setting QT to " << CastQT.getAsString() << "\n";
      OS << "DEBUG:: Type = " << Type->toString() << "\n";
      break;
    case CK_BitCast:
      // when casting to void*, we should drop the region args of the target type.
      if (CastQT->isVoidPointerType()) {
        // FIXME: here we should also take care of void **, void ***, ...
        Type->setQT(CastQT);
        Type->dropArgV();
        OS << "DEBUG:: ImplicitCast: Setting QT to " << CastQT.getAsString() << "\n";
        OS << "DEBUG:: Type = " << Type->toString() << "\n";
      }
      break;
    case CK_LValueToRValue:
    case CK_NoOp:
      // NoOp
      break;
    default:
      // do nothing;
      if (WarnUnsafeCasts) {
        std::string FromTo;
        llvm::raw_string_ostream FromToOS(FromTo);
        FromToOS << "From Type: " << Exp->getSubExpr()->getType().getAsString()
                 << ", To Type: " << Exp->getType().getAsString()
                 << " [Kind: " << Exp->getCastKindName() << "]";
        emitUnsafeImplicitCastWarning(Exp, FromToOS.str());
  }
      break;
    } // end switch
  } // end if (Type)
}

void TypeBuilderVisitor::VisitVAArgExpr(VAArgExpr *Exp) {
  OS << "DEBUG<TypeBuilder>:: Visiting VA_Arg Expression!! ";
  Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";

  // Treat like malloc or new as a fresh memory whose region(s)
  // depend on the LHS of the assignment.
  clearType();
  // do not visit sub-expression
}

void TypeBuilderVisitor::VisitCXXNewExpr(CXXNewExpr *Exp) {
  OS << "DEBUG<TypeBuilder>:: Visiting C++ 'new' Expression!! ";
  Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  if (Exp->isArray()) {
    AssignmentCheckerVisitor ACV(Def, Exp->getArraySize());
  }
  // invoke the assignment checker on the (implicit) constructor call
  AssignmentCheckerVisitor(Def,
                           const_cast<CXXConstructExpr*>
                                       (Exp->getConstructExpr()));
}

void TypeBuilderVisitor::VisitAtomicExpr(AtomicExpr *Exp) {
  OS << "DEBUG<TypeBuilder>:: Visiting AtomicExpr:";
  Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  for (unsigned int i = 0; i < Exp->getNumSubExprs(); i++) {
    OS << "DEBUG:: Atomic Expr[" << i << "]=";
    Expr *SubExp = Exp->getSubExprs()[i];
    SubExp->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    TypeBuilderVisitor TBV(Def, SubExp);
  }
  assert(!Type);
  // TODO: infer region arguments of "return type"
  QualType AtomicQT = Exp->getType();
  OS << "DEBUG:: AtomicQT =" << AtomicQT.getAsString() << "\n";
  Type = new ASaPType(AtomicQT, 0, 0, 0, true);
}

// Takes care of SizeOf, AlignOf, and VecStep expressions
void TypeBuilderVisitor::
VisitUnaryExprOrTypeTraitExpr(UnaryExprOrTypeTraitExpr *Exp) {
  if (!Exp->isArgumentType()) {
    TypeBuilderVisitor TBV(Def, Exp->getArgumentExpr());
  }

  assert(!Type);
  // TODO build proper local temp type
  Type = new ASaPType(Exp->getType(), 0, 0, 0, true);
}

//////////////////////////////////////////////////////////////////////////
// BaseTypeBuilderVisitor

BaseTypeBuilderVisitor::
BaseTypeBuilderVisitor(const FunctionDecl *Def, Expr *Exp)
  : BaseClass(Def), Type(0) {

  OS << "DEBUG:: ******** INVOKING BaseTypeBuilderVisitor...\n";
  Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";

  Visit(Exp);

  OS << "DEBUG:: ******** DONE WITH BaseTypeBuilderVisitor (Type="
     << (Type ? Type->toString() : "<null>") << ")***\n";
}

BaseTypeBuilderVisitor::~BaseTypeBuilderVisitor() {
  delete Type;
}

ASaPType *BaseTypeBuilderVisitor::stealType() {
  ASaPType *Result = Type;
  Type = 0;
  return Result;
}

void BaseTypeBuilderVisitor::VisitMemberExpr(MemberExpr *Exp) {
  OS << "DEBUG:: VisitMemberExpr: ";
  Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  TypeBuilderVisitor TBV(Def, Exp->getBase());
  Type = TBV.stealType();
  if (Type && Exp->isArrow())
    Type->deref(1);
}

} // end namespace asap
} // end namespace clang
