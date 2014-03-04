//=== EffectChecker.cpp - Safe Parallelism checker -----*- C++ -*----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This file defines the Effect Checker pass of the Safe Parallelism
// checker, which tries to prove the safety of parallelism given region
// and effect annotations.
//
// The effect checker makes sure that function effect summaries are
// conservative by covering all the effects of the body of the function.
//
//===----------------------------------------------------------------===//

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/DeclCXX.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugReporter.h"
#include "llvm/Support/SaveAndRestore.h"

#include "ASaPSymbolTable.h"
#include "ASaPType.h"
#include "ASaPUtil.h"
#include "Effect.h"
#include "EffectChecker.h"
#include "Rpl.h"
#include "Substitution.h"
#include "TypeChecker.h"

namespace clang {
namespace asap {

EffectCollectorVisitor::EffectCollectorVisitor (
  const FunctionDecl* Def,
  Stmt *S,
  bool VisitCXXInitializer,
  bool HasWriteSemantics
  ) : BaseClass(Def),
      HasWriteSemantics(HasWriteSemantics),
      IsBase(false),
      DerefNum(0),
      IsCoveredBySummary(true) {
  EffectsTmp = new EffectVector();
  OS << "DEBUG:: ******** INVOKING EffectCheckerVisitor...\n";

  if (!BR.getSourceManager().isInMainFile(Def->getLocation())) {
    OS << "DEBUG::EffectChecker::Skipping Declaration that is not in main compilation file\n";
    return;
  }

  Def->print(OS, Ctx.getPrintingPolicy());
  //S->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  // Check that the effect summary on the canonical decl covers this one.

  EffSummary = SymT.getEffectSummary(this->Def);
  assert(EffSummary);

  if (VisitCXXInitializer) {
    if (const CXXConstructorDecl *D = dyn_cast<CXXConstructorDecl>(Def)) {
      helperVisitCXXConstructorDecl(D);
    }
  }
  Visit(S);
  OS << "DEBUG:: done running Visit\n";
  if (const CXXMethodDecl *CXXD = dyn_cast<CXXMethodDecl>(Def)) {
    // check overidden methods have an effect summary that covers this one
    const EffectSummary *DerivedSum = SymT.getEffectSummary(CXXD);
    assert(DerivedSum);
    const CXXRecordDecl *DerivedClass = CXXD->getParent();

    for(CXXMethodDecl::method_iterator
        I = CXXD->begin_overridden_methods(),
        E = CXXD->end_overridden_methods();
        I != E; ++I) {
      const CXXMethodDecl* OverriddenMethod = *I; // i.e., base Method

      const EffectSummary *OverriddenSum = SymT.getEffectSummary(OverriddenMethod);
      assert(OverriddenSum);

      const SubstitutionVector *SubVec =SymT.getInheritanceSubVec(DerivedClass);
      EffectSummary SubstOVRDSum(*OverriddenSum);
      if (SubVec)
        //SubVec->reverseApplyTo(&SubstOVRDSum);
        SubVec->applyTo(&SubstOVRDSum);
      //SubstBaseSum.substitute(SubVec);
      OS << "DEBUG:: overidden summary error:\n";
      OS << "   DerivedSum: " << DerivedSum->toString() << "\n";
      OS << "   OverriddenSum: " << OverriddenSum->toString() << "\n";
      OS << "   Overridden Method:";
      OverriddenMethod->print(OS, Ctx.getPrintingPolicy());
      OS << "\n";
      OS << "   Derived Method:";
      CXXD->print(OS, Ctx.getPrintingPolicy());
      OS << "\n";
      OS << "   DerivedClass:" << DerivedClass->getNameAsString() << "\n";
      OS << "   InheritanceSubst: ";
      if (SubVec)
        SubVec->print(OS);
      OS << " \n";

      if ( ! SubstOVRDSum.covers(DerivedSum) ) {
        emitOverridenVirtualFunctionMustCoverEffectsOfChildren(OverriddenMethod, CXXD);
      }
    } // end forall method declarations
  }
  OS << "DEBUG:: ******** DONE INVOKING EffectCheckerVisitor ***\n";
  delete EffectsTmp;
}

void EffectCollectorVisitor::memberSubstitute(const ValueDecl *D) {
  assert(D && "D can't be null");
  const ASaPType *T0 = SymT.getType(D);
  if (!T0)
    return; // Nothing to do here
  ASaPType *T1 = new ASaPType(*T0);
  if (T1->isFunctionType())
    T1 = T1->getReturnType();
  if (!T1)
    return;
  OS << "DEBUG:: Type used for substitution = " << T1->toString(Ctx)
     << ", (DerefNum=" << DerefNum << ")\n";

  T1->deref(DerefNum);

  const ParameterVector *ParamVec =
      SymT.getParameterVectorFromQualType(T1->getQT());
  if (!ParamVec || ParamVec->size() == 0)
    return; // Nothing to do here

  // First, compute inheritance induced substitutions
  const SubstitutionVector *InheritanceSubV = // memory leak?
      SymT.getInheritanceSubVec(T1->getQT());
  EffectsTmp->substitute(InheritanceSubV);

  std::auto_ptr<SubstitutionVector> SubV = T1->getSubstitutionVector();

  EffectsTmp->substitute(SubV.get());

  OS << "   DONE\n";
  delete T1;
}

int EffectCollectorVisitor::collectEffects(const ValueDecl *D) {
  // if it's an address, the region is Immutable and there is no effect
  if (DerefNum < 0)
    return 0;

  OS << "DEBUG:: in EffectChecker::collectEffects: ";
  D->print(OS, Ctx.getPrintingPolicy());
  OS << "\nDEBUG:: isBase = " << (IsBase ? "true" : "false") << "\n";
  OS << "DEBUG:: DerefNum = " << DerefNum << "\n";

  assert(D);
  const ASaPType *T0 = SymT.getType(D);
  if (!T0) // e.g., method returning void
    return 0; // Nothing to do here.
  // If it's a function type, we're interested in the return type
  ASaPType *T1 = new ASaPType(*T0);
  if (T1->isFunctionType())
    T1 = T1->getReturnType();
  // If the return type is null, nothing to do
  if (!T1)
    return 0;

  if (T1->isReferenceType())
    T1->deref();
  int EffectNr = 0;

  OS << "DEBUG:: Type used for collecting effects = "
    << T1->toString(Ctx) << "\n";


  // Dereferences have read effects
  // TODO is this atomic or not? just ignore atomic for now
  for (int I = DerefNum; I > 0; --I) {
    const Rpl *InRpl = T1->getInRpl();
    //assert(InRpl);
    if (InRpl) {
      // Arrays may not have an InRpl
      Effect E(Effect::EK_ReadsEffect, InRpl);
      EffectsTmp->push_back(&E);
      EffectNr++;
    }
    T1->deref();
  }
  if (!IsBase) {
    // TODO is this atomic or not? just ignore atomic for now
    Effect::EffectKind EK = (HasWriteSemantics) ?
      Effect::EK_WritesEffect : Effect::EK_ReadsEffect;
    const Rpl *InRpl = T1->getInRpl();
    if (InRpl) {
      Effect E(EK, InRpl);
      EffectsTmp->push_back(&E);
      EffectNr++;
    }
  }
  delete T1;
  return EffectNr;
}

void EffectCollectorVisitor::
emitOverridenVirtualFunctionMustCoverEffectsOfChildren(
    const CXXMethodDecl *Parent, const CXXMethodDecl *Child) {
  StringRef BugName = "overridden virtual function does not cover the effects "\
      "of the overridding methods";
  std::string Str;
  llvm::raw_string_ostream StrOS(Str);
  StrOS << "[in derived class '" << Child->getParent()->getName() << "']";
  helperEmitDeclarationWarning(BR, Parent, StrOS.str(), BugName, false);
}

void EffectCollectorVisitor::
emitUnsupportedConstructorInitializer(const CXXConstructorDecl *D) {
  FatalError = true;
    StringRef BugName = "unsupported constructor initializer."
      " Please file feature support request.";
    helperEmitDeclarationWarning(BR, D, "", BugName, false);
}

void EffectCollectorVisitor::
emitEffectNotCoveredWarning(const Stmt *S, const Decl *D,
                                  const StringRef &Str) {
  FatalError = true;
  StringRef BugName = "effect not covered by effect summary";
  helperEmitStatementWarning(BR, AC, S, D, Str, BugName);
}

int EffectCollectorVisitor::
copyAndPushFunctionEffects(const FunctionDecl *FunD,
                           const SubstitutionVector &SubV) {
  if (!FunD)
    return 0;
  const EffectSummary *FunEffects =
    SymT.getEffectSummary(FunD->getCanonicalDecl());
  assert(FunEffects);
  // Must make copies because we will substitute, cannot use append:
  //EffectsTmp->append(EV->size(), (*EV->begin()));
  for(EffectSummary::const_iterator
    I = FunEffects->begin(),
    E = FunEffects->end();
  I != E; ++I) {
    Effect Eff(*(*I));
    SubV.applyTo(&Eff);
    EffectsTmp->push_back(&Eff);
  }
  return FunEffects->size();
}

bool EffectCollectorVisitor::
checkEffectCoverage(const Expr *Exp, const Decl *D, int N) {
  if (N<=0)
    return true;
  bool Result = true;
  for (int I=0; I<N; ++I){
    std::auto_ptr<Effect> E = EffectsTmp->pop_back_val();
    OS << "### "; E->print(OS); OS << "\n";
    if (EffSummary->covers(E.get())) {
      OS << "DEBUG:: effect not covered: Expr = ";
      Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
      OS << "\n";
      if (D) {
        OS << "\tDecl = ";
        D->print(OS, Ctx.getPrintingPolicy());
        OS << "\n";
      } else {
        OS << "\tDecl = NULL\n";
      }
      std::string Str = E->toString();
      emitEffectNotCoveredWarning(Exp, D, Str);
      Result = false;
    }
  }
  OS << "DEBUG:: effect covered (OK)\n";
  IsCoveredBySummary &= Result;
  return Result;
}

void EffectCollectorVisitor::helperVisitAssignment(BinaryOperator *E) {
  OS << "DEBUG:: helperVisitAssignment. ";
  E->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS <<")\n";

  // 1. Visit RHS with Read Semantics
  {
    SaveAndRestore<bool> VisitWithReadSemantics(HasWriteSemantics, false);
    Visit(E->getRHS());
  }
  // 2. Visit LHS with Write Semantics
  {
    SaveAndRestore<bool> VisitWithWriteSemantics(HasWriteSemantics, true);
    Visit(E->getLHS());
  }
}

void EffectCollectorVisitor::
helperVisitCXXConstructorDecl(const CXXConstructorDecl *D) {
  CXXConstructorDecl::init_const_iterator
    I = D->init_begin(),
    E = D->init_end();
  for(; I != E; ++I) {
    CXXCtorInitializer *Init = *I;
    if (Init->isMemberInitializer() || Init->isBaseInitializer()) {
      Visit(Init->getInit());
    } else {
      OS << "DEBUG:: unsupported initializer:\n";
      Init->getInit()->printPretty(OS, 0, Ctx.getPrintingPolicy());
      emitUnsupportedConstructorInitializer(D);
    }
  }
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
  SaveAndRestore<bool> VisitBase(IsBase, true);
  SaveAndRestore<bool> VisitBaseWithReadSemantics(HasWriteSemantics, false);
  SaveAndRestore<int> ResetDerefNum(DerefNum, (Exp->isArrow() ? 1 : 0));

  Visit(Exp->getBase());

  /// Post-Visit Actions: check that effects (after substitutions)
  /// are covered by effect summary
  checkEffectCoverage(Exp, VD, EffectNr);
} // end VisitMemberExpr

void EffectCollectorVisitor::VisitUnaryAddrOf(UnaryOperator *E)  {
  assert(DerefNum>=0);
  SaveAndRestore<int> DecrementDerefNum(DerefNum, DerefNum-1);
  OS << "DEBUG:: Visit Unary: AddrOf (DerefNum=" << DerefNum << ")\n";
  Visit(E->getSubExpr());
}

void EffectCollectorVisitor::VisitUnaryDeref(UnaryOperator *E) {
  SaveAndRestore<int> IncrementDerefNum(DerefNum, DerefNum+1);
  OS << "DEBUG:: Visit Unary: Deref (DerefNum=" << DerefNum << ")\n";
  Visit(E->getSubExpr());
}

void EffectCollectorVisitor::VisitPrePostIncDec(UnaryOperator *E) {
  SaveAndRestore<bool> VisitWithWriteSemantics(HasWriteSemantics, true);
  Visit(E->getSubExpr());
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
  if (!Ret->getRetValue())
    return; // this is a 'return' statement with no return expression
  // This next lookup actually returns the function type.
  const ASaPType *FunType = SymT.getType(Def);
  if (!FunType) {
    // This is probably (hopefully) a template function.
    // We are not yet checking effects and types of parametric code
    // (only of instantiated templates)
    return;
  }

  assert(FunType);
  // Make a copy because getReturnType will modify RetTyp.
  ASaPType *RetTyp = new ASaPType(*FunType);
  RetTyp = RetTyp->getReturnType();
  assert(RetTyp);

  if (RetTyp->getQT()->isReferenceType()) {
    SaveAndRestore<int> DecrementDerefNum(DerefNum, DerefNum-1);
    Visit(Ret->getRetValue());
  } else {
    Visit(Ret->getRetValue());
  }
  delete RetTyp;
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
  //DerefNum = 0;
}

void EffectCollectorVisitor::VisitCXXThisExpr(CXXThisExpr *E) {
  //OS << "DEBUG:: visiting 'this' expression\n";
  //DerefNum = 0;
  OS << "DEBUG:: VisitCXXThisExpr!! :)\n";
  OS << "DEBUG:: Type of 'this' = " << E->getType().getAsString() << "\n";
  const SubstitutionVector *InheritanceSubV =
      SymT.getInheritanceSubVec(E->getType()->getPointeeType());
  if(InheritanceSubV) {
    OS << "DEBUG:: InheritanceSubV.size = " << InheritanceSubV->size() << "\n";

    EffectsTmp->substitute(InheritanceSubV);
  }

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

void EffectCollectorVisitor::VisitCallExpr(CallExpr *Exp) {
  if (Exp->getType()->isDependentType())
    return; // Do not visit if this is dependent type

  OS << "DEBUG:: VisitCallExpr\n";

  if (isa<CXXPseudoDestructorExpr>(Exp->getCallee())) {
    Visit(Exp->getCallee());
  } else {
    // Not a PseudoDestructorExpr -> Exp->getCalleeDecl should return non-null
    Decl *D = Exp->getCalleeDecl();
    assert(D);

    /// 1. Visit Arguments w. Read semantics
    {
      SaveAndRestore<bool> VisitWithReadSemantics(HasWriteSemantics, false);
      for(ExprIterator I = Exp->arg_begin(), E = Exp->arg_end();
          I != E; ++I) {
        Visit(*I);
      }
    }

    FunctionDecl *FunD = dyn_cast<FunctionDecl>(D);
    VarDecl *VarD = dyn_cast<VarDecl>(D); // Non-null if calling through fn-ptr
    assert(FunD || VarD);
    if (FunD) {

      SubstitutionVector SubV;
      // Set up Substitution Vector
      const ParameterVector *FD_ParamV = SymT.getParameterVector(FunD);
      if (FD_ParamV && FD_ParamV->size() > 0) {
        buildParamSubstitutions(FunD, Exp->arg_begin(),
                                Exp->arg_end(), *FD_ParamV, SubV);
      }

      /// 2. Add effects to tmp effects
      int EffectCount = copyAndPushFunctionEffects(FunD, SubV);
      /// 3. Visit base if it exists
      Visit(Exp->getCallee());
      /// 4. Check coverage
      checkEffectCoverage(Exp, D, EffectCount);
    // end if (FunD)
    } else { // VarD != null (call through function pointer)
    // TODO
    }
  }
}

void EffectCollectorVisitor::
VisitArraySubscriptExpr(ArraySubscriptExpr *Exp) {
  // 1. Visit index with read semantics
  {
    SaveAndRestore<bool> VisitWithReadSemantics(HasWriteSemantics, false);
    SaveAndRestore<int> IncreaseDerefNum(DerefNum, 0);
    Visit(Exp->getIdx());
  }
  // 2. visit base
  SaveAndRestore<int> IncreaseDerefNum(DerefNum, DerefNum+1);
  Visit(Exp->getBase());
}

void EffectCollectorVisitor::
VisitCXXDeleteExpr(CXXDeleteExpr *Exp) {
  OS << "DEBUG:: VisitCXXDeleteExpr: ";
  Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";

  // 1. Visit expression
  Visit(Exp->getArgument());

  // Since we assume memory safety, we can ignore the effects of
  // freeing memory: the code should never access freed memory!
  // Not ignoring the effects of freeing memory (e.g., assuming write
  // effects to the freed region(s)) may result in too conservative
  // effect summaries.

  // The code below is kept temporarily as this decision should be discussed!
  // 2. Include the effects of the implicitly called destructor
  /*
  TypeBuilderVisitor TBV(VB, Def, Exp->getArgument());
  ASaPType *T = TBV.getType();
  OS << "DEBUG:: The Type of deleted expression is: " << T->toString() << "\n";
  assert(T->getQT()->isPointerType());
  T->deref();

  if (T->getQT()->isScalarType()) {
    // 2.A Look at the type of the expression and if it is a pointer to a
    // builtin type the effect writes to the region pointed to (deallocating
    // part of a region is like writing to that region)
    Effect Eff(Effect::EK_WritesEffect, T->getInRpl());
    EffectsTmp->push_back(&Eff);
    Decl *D = 0; //Exp->getArgument()->getDecl()
    checkEffectCoverage(Exp->getArgument(), D, 1);
  } else {
    // 2.B otherwise, use the declared effects of the destructor for the type
    // TODO
  } // FIXME what about delete [] Expr (arrays) ?
  */
}

void EffectCollectorVisitor::VisitCXXNewExpr(CXXNewExpr *Exp) {
  OS << "DEBUG<EffectCollectorVisitor>:: Visiting C++ 'new' Expression!! ";
  Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";

  SaveAndRestore<int> VisitWithZeroDeref(DerefNum, 0);
  VisitChildren(Exp);
}


// End Visitors
//////////////////////////////////////////////////////////////////////////
void EffectCollectorVisitor::
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

void EffectCollectorVisitor::
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
    if (ParamR->length() != 1)
      continue;
    const RplElement *Elmt = ParamR->getFirstElement();
    assert(Elmt && "Rpl should not contain null RplElement pointer");
    if (ParamV.hasElement(Elmt)) {
      // Ok find the argument
      Substitution Sub(Elmt, *ArgI);
      SubV.push_back(&Sub);
      OS << "DEBUG:: added function param sub: " << Sub.toString() << "\n";
    }
  }
}

} // end namespace asap
} // end namespace clang
