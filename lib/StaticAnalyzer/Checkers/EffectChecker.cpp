//=== EffectChecker.cpp - Safe Parallelism checker -----*- C++ -*----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This files defines the Effect Checker pass of the Safe Parallelism
// checker, which tries to prove the safety of parallelism given region
// and effect annotations.
//
//===----------------------------------------------------------------===//

#include "EffectChecker.h"
#include "ASaPType.h"
#include "Rpl.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Stmt.h"
// FIXME: try clear these headers up.
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/AnalysisManager.h"
#include "clang/StaticAnalyzer/Core/BugReporter/PathDiagnostic.h"

using namespace clang;
using namespace clang::ento;
using namespace clang::asap;
using namespace llvm;

void EffectCollectorVisitor::memberSubstitute(const ValueDecl *D) {
  assert(D);
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
    << "->" << ToRpl->toString() << "\n";

  if (FromEl->getName().compare(ToRpl->toString())) {
    // if (from != to) then substitute
    Substitution S(FromEl, ToRpl);
    /// 2.1.1 Substitution of effects
    EffectsTmp.substitute(S);
  }
  OS << "   DONE\n";
}

int EffectCollectorVisitor::collectEffects(const ValueDecl *D) {
  if (DerefNum < 0)
    return 0;
  OS << "DEBUG:: in EffectChecker::collectEffects: ";
  D->print(OS, Ctx.getPrintingPolicy());
  OS << "\nDEBUG:: isBase = " << (IsBase ? "true" : "false") << "\n";
  OS << "DEBUG:: DerefNum = " << DerefNum << "\n";

  assert(D);
  const ASaPType *T = SymT.getType(D);
  if (!T) // e.g., method returning null
    return 0;

  int EffectNr = 0;
  ASaPType *Type = 0;
  Type = new ASaPType(*T); // Make a copy of T.

  OS << "DEBUG:: Type used for collecting effects = "
    << Type->toString(Ctx) << "\n";


  // Dereferences have read effects
  // TODO is this atomic or not? just ignore atomic for now
  for (int I = DerefNum; I > 0; --I) {
    const Rpl *InRpl = Type->getInRpl();
    assert(InRpl);
    if (InRpl) {
      // References do not have an InRpl
      Effect E(Effect::EK_ReadsEffect, InRpl);
      EffectsTmp.push_back(&E);
      EffectNr++;
    }
    Type->deref();
  }
  if (!IsBase) {
    // TODO is this atomic or not? just ignore atomic for now
    Effect::EffectKind EK = (HasWriteSemantics) ?
      Effect::EK_WritesEffect : Effect::EK_ReadsEffect;
    const Rpl *InRpl = Type->getInRpl();
    if (InRpl) {
      Effect E(EK, InRpl);
      EffectsTmp.push_back(&E);
      EffectNr++;
    }
  }
  delete Type;
  return EffectNr;
}

void EffectCollectorVisitor::helperEmitDeclarationWarning(const Decl *D,
                                                          const StringRef &Str,
                                                          std::string BugName,
                                                          bool AddQuotes) {
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

void EffectCollectorVisitor::
helperEmitEffectNotCoveredWarning(const Stmt *S, const Decl *D,
                                  const StringRef &Str) {
  StringRef BugName = "effect not covered by effect summary";
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

int EffectCollectorVisitor::copyAndPushFunctionEffects(FunctionDecl *FunD) {
  const EffectSummary *FunEffects = SymT.getEffectSummary(FunD);
  assert(FunEffects);
  // Must make copies because we will substitute, cannot use append:
  //EffectsTmp.append(EV->size(), (*EV->begin()));
  for(EffectSummary::const_iterator
    I = FunEffects->begin(),
    E = FunEffects->end();
  I != E; ++I) {
    Effect Eff(*(*I));
    EffectsTmp.push_back(&Eff);
  }
  return FunEffects->size();
}

bool EffectCollectorVisitor::checkEffectCoverage(const Expr *Exp, const Decl *D,
                                                 int N) {
  bool Result = true;
  for (int I=0; I<N; ++I){
    Effect* E = EffectsTmp.pop_back_val();
    OS << "### "; E->print(OS); OS << "\n";
    if (!E->isCoveredBy(*EffSummary, LOCALRplElmt)) {
      std::string Str = E->toString();
      helperEmitEffectNotCoveredWarning(Exp, D, Str);
      Result = false;
    }
  }
  IsCoveredBySummary &= Result;
  return Result;
}

void EffectCollectorVisitor::helperVisitAssignment(BinaryOperator *E) {
    OS << "DEBUG:: helperVisitAssignment. ";
    E->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS <<")\n";

    bool SavedHWS = HasWriteSemantics;
    HasWriteSemantics = false;
    Visit(E->getRHS());

    HasWriteSemantics = true;
    Visit(E->getLHS());

    /// Restore flags
    HasWriteSemantics = SavedHWS;
  }

void EffectCollectorVisitor::
helperEmitUnsupportedConstructorInitializer(const CXXConstructorDecl *D) {
    StringRef BugName = "unsupported constructor initializer."
      " Please file feature support request.";
    helperEmitDeclarationWarning(D, "", BugName, false);
  }

void EffectCollectorVisitor::
helperVisitCXXConstructorDecl(const CXXConstructorDecl *D) {
  CXXConstructorDecl::init_const_iterator
    I = D->init_begin(),
    E = D->init_end();
  for(; I != E; ++I) {
    CXXCtorInitializer *Init = *I;
    if (Init->isMemberInitializer()) {
      //helperTypecheckDeclWithInit(Init->getMember(), Init->getInit());
      Visit(Init->getInit());
    } else {
      helperEmitUnsupportedConstructorInitializer(D);
    }
  }
}

EffectCollectorVisitor::EffectCollectorVisitor (
  ento::BugReporter &BR,
  ASTContext &Ctx,
  AnalysisManager &Mgr,
  AnalysisDeclContext *AC,
  raw_ostream &OS,
  SymbolTable &SymT,
  const FunctionDecl* Def,
  Stmt *S
  ) : BR(BR),
  Ctx(Ctx),
  AC(AC),
  OS(OS),
  SymT(SymT),
  Def(Def),
  FatalError(false),
  HasWriteSemantics(false),
  IsBase(false),
  DerefNum(0),
  IsCoveredBySummary(true) {

    OS << "DEBUG:: ******** INVOKING EffectCheckerVisitor...\n";
    S->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    EffSummary = SymT.getEffectSummary(this->Def);
    assert(EffSummary);

    if (const CXXConstructorDecl *D = dyn_cast<CXXConstructorDecl>(Def)) {
      helperVisitCXXConstructorDecl(D);
    }

    Visit(S);
    OS << "DEBUG:: ******** DONE INVOKING EffectCheckerVisitor ***\n";
}

void EffectCollectorVisitor::VisitChildren(Stmt *S) {
  for (Stmt::child_iterator I = S->child_begin(), E = S->child_end();
    I!=E; ++I)
    if (Stmt *child = *I)
      Visit(child);
}

void EffectCollectorVisitor::VisitStmt(Stmt *S) {
  VisitChildren(S);
}

void EffectCollectorVisitor::VisitMemberExpr(MemberExpr *Exp) {
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
  VD->print(OS, Ctx.getPrintingPolicy());
  OS << "\n";

  if (IsBase)
    memberSubstitute(VD);

  int EffectNr = collectEffects(VD);

  /// 2.3. Visit Base with read semantics, then restore write semantics
  bool SavedHWS = HasWriteSemantics;
  bool SavedIsBase = IsBase; // probably not needed to save

  DerefNum = Exp->isArrow() ? 1 : 0;
  HasWriteSemantics = false;
  IsBase = true;
  Visit(Exp->getBase());

  /// Post visitation checking
  HasWriteSemantics = SavedHWS;
  IsBase = SavedIsBase;
  /// Post-Visit Actions: check that effects (after substitutions)
  /// are covered by effect summary
  checkEffectCoverage(Exp, VD, EffectNr);
} // end VisitMemberExpr

void EffectCollectorVisitor::VisitUnaryAddrOf(UnaryOperator *E)  {
  assert(DerefNum>=0);
  DerefNum--;
  OS << "DEBUG:: Visit Unary: AddrOf (DerefNum=" << DerefNum << ")\n";
  Visit(E->getSubExpr());
}

void EffectCollectorVisitor::VisitUnaryDeref(UnaryOperator *E) {
  DerefNum++;
  OS << "DEBUG:: Visit Unary: Deref (DerefNum=" << DerefNum << ")\n";
  Visit(E->getSubExpr());
}

void EffectCollectorVisitor::VisitPrePostIncDec(UnaryOperator *E) {
  bool savedHws = HasWriteSemantics;
  HasWriteSemantics=true;
  Visit(E->getSubExpr());
  HasWriteSemantics = savedHws;
}

void EffectCollectorVisitor::VisitUnaryPostInc(UnaryOperator *E) {
  VisitPrePostIncDec(E);
}

void EffectCollectorVisitor::VisitUnaryPostDec(UnaryOperator *E) {
  VisitPrePostIncDec(E);
}

void EffectCollectorVisitor::VisitUnaryPreInc(UnaryOperator *E) {
  VisitPrePostIncDec(E);
}

void EffectCollectorVisitor::VisitUnaryPreDec(UnaryOperator *E) {
  VisitPrePostIncDec(E);
}

void EffectCollectorVisitor::VisitReturnStmt(ReturnStmt *Ret) {
  // This next lookup actually returns the function type.
  const ASaPType *ReturnType = SymT.getType(Def);
  assert(ReturnType);

  if (ReturnType->getQT()->isReferenceType()) {
    DerefNum--; // FIXME: we prob need a stack of DerefNum for complex exprs.
  }
  Visit(Ret->getRetValue());
}

void EffectCollectorVisitor::VisitDeclRefExpr(DeclRefExpr *Exp) {
  OS << "DEBUG:: VisitDeclRefExpr --- whatever that is!: ";
  Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  ValueDecl* VD = Exp->getDecl();
  assert(VD);

  if (IsBase)
    memberSubstitute(VD);

  int EffectNr = collectEffects(VD);
  checkEffectCoverage(Exp, VD, EffectNr);

  //TODO Collect effects
  /*OS << "Rvalue=" << E->isRValue()
  << ", Lvalue=" << E->isLValue()
  << ", Xvalue=" << E->isGLValue()
  << ", GLvalue=" << E->isGLValue() << "\n";
  Expr::LValueClassification lvc = E->ClassifyLValue(Ctx);
  if (lvc==Expr::LV_Valid)
  OS << "LV_Valid\n";
  else
  OS << "not LV_Valid\n";
  ValueDecl* vd = E->getDecl();
  vd->print(OS, Ctx.getPrintingPolicy());
  OS << "\n";*/
  DerefNum = 0;
}

void EffectCollectorVisitor::VisitCXXThisExpr(CXXThisExpr *E) {
  //OS << "DEBUG:: visiting 'this' expression\n";
  DerefNum = 0;
}

void EffectCollectorVisitor::
VisitCompoundAssignOperator(CompoundAssignOperator *E) {
  OS << "DEBUG:: !!!!!!!!!!! Mother of compound Assign!!!!!!!!!!!!!\n";
  E->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  helperVisitAssignment(E);
}

void EffectCollectorVisitor::VisitBinAssign(BinaryOperator *E) {
  OS << "DEBUG:: >>>>>>>>>>VisitBinAssign<<<<<<<<<<<<<<<<<\n";
  E->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  helperVisitAssignment(E);
}

void EffectCollectorVisitor::VisitCallExpr(CallExpr *E) {
  OS << "DEBUG:: VisitCallExpr\n";
}

void EffectCollectorVisitor::VisitCXXMemberCallExpr(CXXMemberCallExpr *Exp) {
  OS << "DEBUG:: VisitCXXMemberCallExpr\n";
  CXXMethodDecl *D = Exp->getMethodDecl();
  assert(D);

  /// 2. Add effects to tmp effects
  int EffectCount = copyAndPushFunctionEffects(D);
  /// 3. Visit base if it exists
  //TODO we have to visit call arguments with read semantics
  VisitChildren(Exp);
  /// 4. Check coverage
  checkEffectCoverage(Exp, D, EffectCount);
}

void EffectCollectorVisitor::
VisitCXXOperatorCallExpr(CXXOperatorCallExpr *Exp) {
  OS << "DEBUG:: VisitCXXOperatorCall: ";
  //Exp->dump(OS, BR.getSourceManager());
  Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";

  Decl *D = Exp->getCalleeDecl();
  assert(D);
  FunctionDecl *FD = dyn_cast<FunctionDecl>(D);
  assert(FD);
  OS << "DEBUG:: FunctionDecl = ";
  FD->print(OS, Ctx.getPrintingPolicy());
  OS << "DEBUG:: isOverloadedOperator = "
    << FD->isOverloadedOperator() << "\n";

  /// 2. Add effects to tmp effects
  int EffectCount = copyAndPushFunctionEffects(FD);

  /// 3. Visit base if it exists
  //TODO we have to visit call arguments with read semantics
  VisitChildren(Exp);

  /// 4. Check coverage
  checkEffectCoverage(Exp, D, EffectCount);
}

