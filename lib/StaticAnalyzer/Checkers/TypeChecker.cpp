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
  Stmt *S
  ) : BR(BR),
  Ctx(Ctx),
  Mgr(Mgr),
  AC(AC),
  OS(OS),
  SymT(SymT),
  Def(Def),
  FatalError(false), Type(0) {

    OS << "DEBUG:: ******** INVOKING AssignmentCheckerVisitor...\n";
    Def->print(OS, Ctx.getPrintingPolicy());
    //S->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    if (const CXXConstructorDecl *D = dyn_cast<CXXConstructorDecl>(Def)) {
      // Also visit initialization lists
      helperVisitCXXConstructorDecl(D);
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

void AssignmentCheckerVisitor::VisitCXXMemberCallExpr(CXXMemberCallExpr *Exp) {
  typecheckCallExpr(Exp);
  VisitChildren(Exp);
}

void AssignmentCheckerVisitor::
VisitCXXOperatorCallExpr(CXXOperatorCallExpr *Exp) {
  typecheckCallExpr(Exp);
  VisitChildren(Exp);
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
            typecheckCXXConstructExpr(VD, Exp);
            break;
          }
        }
      }
  } // end for each declaration
}

bool AssignmentCheckerVisitor::typecheck(const ASaPType *LHSType,
                                         const ASaPType *RHSType,
                                         bool IsInit) {
  assert(LHSType);
  if (RHSType && !RHSType->isAssignableTo(*LHSType, IsInit) )
    return false;
  else
    return true;
}

void AssignmentCheckerVisitor::typecheckCall(FunctionDecl *CalleeDecl,
                                             ExprIterator ArgI,
                                             ExprIterator ArgE,
                                             SubstitutionVector *SubV) {
  assert(CalleeDecl);
  FunctionDecl::param_iterator ParamI, ParamE;
  for(ParamI = CalleeDecl->param_begin(), ParamE = CalleeDecl->param_end();
      ArgI != ArgE && ParamI != ParamE; ++ArgI, ++ParamI) {
    OS << "DEBUG:: " << "\n";
    Expr *ArgExpr = *ArgI;
    ParmVarDecl *ParamDecl = *ParamI;
    typecheckParamAssignment(ParamDecl, ArgExpr, SubV);
  }
}

void AssignmentCheckerVisitor::typecheckCXXConstructExpr(VarDecl *D,
                                                         CXXConstructExpr *Exp) {
  CXXConstructorDecl *ConstrDecl =  Exp->getConstructor();
  DeclContext *ClassDeclContext = ConstrDecl->getDeclContext();
  assert(ClassDeclContext);
  RecordDecl *ClassDecl = dyn_cast<RecordDecl>(ClassDeclContext);
  assert(ClassDecl);
  // Set up Substitution Vector
  SubstitutionVector SubV, *SubPtr = 0;
  Substitution S;
  const ParameterVector *PV = SymT.getParameterVector(ClassDecl);
  if (PV) {
    assert(PV->size() == 1); // until we support multiple region params
    const ParamRplElement *ParamEl = PV->getParamAt(0);
    const ASaPType *T = SymT.getType(D);
    if (T) {
      const Rpl *R = T->getSubstArg();
      S.set(ParamEl, R);
      OS << "DEBUG:: ConstructExpr Substitution = " << S.toString() << "\n";
      SubV.push_back(&S);
      SubPtr = &SubV;
    }
  }
  typecheckCall(ConstrDecl, Exp->arg_begin(), Exp->arg_end(), SubPtr);
}

void AssignmentCheckerVisitor::
helperEmitDeclarationWarning(const Decl *D, const StringRef &Str,
                             std::string BugName, bool AddQuotes) {
  std::string Description = "";
  if (AddQuotes)
    Description.append("'");
  Description.append(Str);
  if (AddQuotes)
    Description.append("' ");
  else
    Description.append(" ");
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
    } else {
      helperEmitUnsupportedConstructorInitializer(D);
    }
  }
}

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
  const ASaPType *LHSType = SymT.getType(Def);
  assert(LHSType);
  ASaPType *RHSType = TBVR.getType();
  if (! typecheck(LHSType, RHSType, true)) {
    OS << "DEBUG:: invalid assignment: gonna emit an error\n";
    helperEmitInvalidReturnTypeWarning(Ret, LHSType, RHSType);
    FatalError = true;
  }
  delete Type;
  Type = 0;
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
typecheckParamAssignment(ParmVarDecl *Param, Expr *Arg,
                         SubstitutionVector *SubV) {
  bool Result = true;
  OS << "DEBUG:: typeckeckParamAssignment\n";
  if (SubV) {
    OS << "SubstitutionVector Size = " << SubV->size() << "\n";
    OS << "SubVec: " << SubV->toString();
  }
  TypeBuilderVisitor TBVR(BR, Ctx, Mgr, AC, OS, SymT, Def, Arg);
  const ASaPType *LHSType = SymT.getType(Param);
  ASaPType *LHSTypeMod = 0;
  if (SubV && LHSType) {
    OS << "DEBUG:: gonna perform substitution\n";
    LHSTypeMod = new ASaPType(*LHSType);
    LHSTypeMod->substitute(*SubV);
    LHSType = LHSTypeMod;
    OS << "DEBUG:: DONE perform substitution\n";
  }
  ASaPType *RHSType = TBVR.getType();
  if (! typecheck(LHSType, RHSType, true)) {
    OS << "DEBUG:: invalid argument to parameter assignment: "
      << LHSType->toString(Ctx) << ", " << RHSType->toString(Ctx)
      << "gonna emit an error\n";
    //  Fixme pass VS as arg instead of Init
    helperEmitInvalidArgToFunctionWarning(Arg, LHSType, RHSType);
    FatalError = true;
    Result = false;
  }
  delete LHSTypeMod;
  OS << "DEBUG:: DONE with typeckeckParamAssignment\n";
  return Result;
}

void AssignmentCheckerVisitor::typecheckCallExpr(CallExpr *Exp) {
  Decl *D = Exp->getCalleeDecl();
  assert(D);
  FunctionDecl *FD = dyn_cast<FunctionDecl>(D);
  assert(FD);
  // Set up Substitution Vector
  DeclContext *DC = FD->getDeclContext();
  assert(DC);
  RecordDecl *ClassDecl = dyn_cast<RecordDecl>(DC);
  assert(ClassDecl);
  const ParameterVector *PV = SymT.getParameterVector(ClassDecl);

  SubstitutionVector SubV, *SubPtr = 0;
  Substitution S;
  BaseTypeBuilderVisitor  TBV(BR, Ctx, Mgr, AC, OS,
    SymT, Def, Exp->getCallee());
  if (PV) {
    assert(PV->size() == 1); // until we support multiple region params
    const ParamRplElement *ParamEl = PV->getParamAt(0);

    ASaPType *T = TBV.getType();
    if (T) {
      OS << "DEBUG:: Base Type = " << T->toString(Ctx) << "\n";
      const Rpl *R = T->getSubstArg();
      S.set(ParamEl, R);
      OS << "DEBUG:: typecheckCallExpr Substitution = "
        << S.toString() << "\n";
      SubV.push_back(&S);
      SubPtr = &SubV;
    }
  }
  typecheckCall(FD, Exp->arg_begin(), Exp->arg_end(), SubPtr);
}

void TypeBuilderVisitor::memberSubstitute(const ValueDecl *D) {
  assert(D);
  OS << "DEBUG:: in TypeBuilder::memberSubstitute:";
  D->print(OS, Ctx.getPrintingPolicy());
  OS << "\nDEBUG:: isBase = " << (IsBase ? "true" : "false") << "\n";
  OS << "DEBUG:: DerefNum = " << DerefNum << "\n";

  const ASaPType *T = SymT.getType(D);
  assert(T);
  OS << "DEBUG:: Type used for substitution = " << T->toString(Ctx) << "\n";

  QualType QT = T->getQT(DerefNum);

  const ParameterVector *ParamVec = SymT.getParameterVectorFromQualType(QT);

  // TODO support multiple Parameters
  const ParamRplElement *FromEl = ParamVec->getParamAt(0);
  assert(FromEl);

  const Rpl *ToRpl = T->getSubstArg(DerefNum);
  assert(ToRpl);
  OS << "DEBUG:: gonna substitute... " << FromEl->getName()
    << "<-" << ToRpl->toString() << "\n";

  if (FromEl->getName().compare(ToRpl->toString())) {
    OS <<" GO!!\n";
    assert(Type);
    Substitution S(FromEl, ToRpl);
    Type->substitute(S);
  }
  OS << "   DONE\n";
}

void TypeBuilderVisitor::setType(const ValueDecl *D) {
  OS << "DEBUG:: in TypeBuilder::setType: ";
  D->print(OS, Ctx.getPrintingPolicy());
  //OS << "\n Decl pointer address:" << D;
  OS << "\n";
  const ASaPType *T = SymT.getType(D);
  assert(T);

  assert(!Type);
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
  S->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  VisitChildren(S);
}

void TypeBuilderVisitor::VisitUnaryAddrOf(UnaryOperator *Exp)  {
  assert(DerefNum>=0);
  DerefNum--;
  OS << "DEBUG:: Visit Unary: AddrOf (DerefNum=" << DerefNum << ") Type = ";
  Exp->getType().print(OS, Ctx.getPrintingPolicy());
  OS << "\n";

  RefQT = Exp->getType();
  assert(RefQT->isPointerType());

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
  assert(VD);
  if (IsBase)
    memberSubstitute(VD);
  else
    setType(VD);
}

void TypeBuilderVisitor::VisitCXXThisExpr(CXXThisExpr *E) {
  OS << "DEBUG:: visiting 'this' expression\n";
  assert(E);
  //DerefNum = 0;
  if (!IsBase) {
    assert(!Type);
    // Add parameter as implicit argument
    CXXRecordDecl *RecDecl =
      const_cast<CXXRecordDecl*>(E->getBestDynamicClassType());
    assert(RecDecl);

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
  Visit(LHS);
  ASaPType *T = stealType();
  Visit(RHS);
  if (Type)
    Type->join(T);
  else
    Type = T;
}

void TypeBuilderVisitor::VisitBinSub(BinaryOperator *Exp) {
  helperBinAddSub(Exp->getLHS(), Exp->getRHS());
}

void TypeBuilderVisitor::VisitBinAdd(BinaryOperator *Exp) {
  helperBinAddSub(Exp->getLHS(), Exp->getRHS());
}

void TypeBuilderVisitor::VisitBinAssign(BinaryOperator *Exp) {
  OS << "DEBUG:: >>>>>>>>>>VisitBinAssign<<<<<<<<<<<<<<<<<\n";
  Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";

  AssignmentCheckerVisitor ACV(BR, Ctx, Mgr, AC, OS, SymT, Def, Exp);
  assert(!Type);
  Type = ACV.stealType();
  assert(Type);
}

void TypeBuilderVisitor::VisitConditionalOperator(ConditionalOperator *Exp) {
  OS << "DEBUG:: @@@@@@@@@@@@VisitConditionalOp@@@@@@@@@@@@@@\n";
  Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  AssignmentCheckerVisitor ACV(BR, Ctx, Mgr, AC, OS,
    SymT, Def, Exp->getCond());
  FatalError |= ACV.encounteredFatalError();

  assert(!Type);
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

void TypeBuilderVisitor::VisitCXXNewExpr(CXXNewExpr *Exp) {
}

void TypeBuilderVisitor::VisitCallExpr(CallExpr *Exp) {
  // Don't visit call arguments. Typechecking those is initiated
  // by the AssignmentCheckerVisitor.
  OS << "DEBUG:: VisitCallExpr:";
  Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  if (Exp->getCallee())
    Visit(Exp->getCallee());
  else
    OS << "NOTHING TODO HERE!\n";
  return;
}

void TypeBuilderVisitor::VisitReturnStmt(ReturnStmt *Ret) {
  assert(false);
  return;
}

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




