//=== ASaPGenericStmtVisitor.cpp - Safe Parallelism checker -*- C++ -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This file defines the ASaPStmtVisitor template classe used by the Safe
// Parallelism checker, which tries to prove the safety of parallelism
// given region and effect annotations.
//
// ASaPStmtVisitor extends clang::StmtVisitor and it includes common
// functionality shared by the ASaP passes that visit statements,
// including TypeBuilder, AssignmentChecker, EffectCollector, and more.
//
//===----------------------------------------------------------------===//
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Expr.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugReporter.h"

#include "ASaPSymbolTable.h"
#include "ASaPType.h"
#include "Effect.h"
#include "SpecificNIChecker.h"

namespace clang {
namespace asap {

const static std::string CXX_CALL_OPERATOR = "operator()";

/// \brief The position of the Body for tbb::parallel_for with a Range argument.
/// Note Positions are numbered starting from 0
const static int TBB_PARFOR_RANGE_BODY_POSITION = 1;
/// \brief The position of the functor for tbb::parallel_for with indices
/// but without a step argument
const static int TBB_PARFOR_INDEX2_FUNCTOR_POSITION = 2;
/// \brief The position of the functor for tbb::parallel_for with indices and with a step argument
const static int TBB_PARFOR_INDEX3_FUNCTOR_POSITION = 3;


static void emitNICheckNotImplemented(const Stmt *S,
                                      const FunctionDecl *FunD) {
  StringRef BugName = "Non-interference check not implemented";
  std::string Name;
  if (FunD)
    Name = FunD->getNameInfo().getAsString();
  else
    Name = "";
  StringRef Str(Name);
  helperEmitStatementWarning(*SymbolTable::VB.BR,
                             SymbolTable::VB.AC,
                             S, FunD, Str, BugName, false);
}

static void emitUnexpectedTypeOfArgumentPassed(const Stmt *S,
                                               const FunctionDecl *FunD) {
  StringRef BugName = "unexpected type of argument passed to TBB method";
  std::string Name;
  if (FunD)
    Name = FunD->getNameInfo().getAsString();
  else
    Name = "";
  StringRef Str(Name);
  helperEmitStatementWarning(*SymbolTable::VB.BR,
                             SymbolTable::VB.AC,
                             S, FunD, Str, BugName, false);
}

static void emitInterferingEffects(const Stmt *S,
                                   const EffectSummary &ES1,
                                   const EffectSummary &ES2) {
  StringRef BugName = "interfering effects";
  std::string SBuf;
  llvm::raw_string_ostream OS(SBuf);
  OS << "{" << ES1.toString() << "} interferes with {"
     << ES2.toString() << "}";
  StringRef Str(OS.str());
  helperEmitStatementWarning(*SymbolTable::VB.BR,
                             SymbolTable::VB.AC,
                             S, 0, Str, BugName, false);
}

static void emitEffectsNotCoveredWarning(const Stmt *S,
                                        const Decl *D,
                                        const StringRef &Str) {
  std::string SBuf;
  llvm::raw_string_ostream OS(SBuf);
  OS << "effects not covered by effect summary";
  const EffectSummary *DefES = SymbolTable::Table->getEffectSummary(D);
  if (DefES) {
    OS << ": " << DefES->toString();
  }
  StringRef BugName(OS.str());
  helperEmitStatementWarning(*SymbolTable::VB.BR,
                             SymbolTable::VB.AC,
                             S, D, Str, BugName);
}

bool TBBSpecificNIChecker::check(CallExpr *E, const FunctionDecl *Def) const {
  emitNICheckNotImplemented(E, 0);
  return false;
}


static bool checkMethodType(QualType MethQT, bool TakesParam) {
  if (!MethQT->isFunctionType())
    return false;
  const FunctionProtoType *FT = MethQT->getAs<FunctionProtoType>();
  assert(FT);
  // Check that return type is void
  QualType RetQT = FT->getReturnType();
  if (!RetQT->isVoidType())
    return false; // Technically we could allow any return type

  const unsigned int NumParams = TakesParam ? 1 :0;
  if (FT->getNumParams() != NumParams) {
    return false;
  }
  return true;
}

static const CXXMethodDecl
*tryGetOperatorMethod(const Expr *Arg,
                      bool TakesParam = false, bool Force = false) {
  const CXXMethodDecl *Result = 0;

  //raw_ostream &OS = *SymbolTable::VB.OS;
  //OS << "DEBUG:: getOperatorMethod : Arg :";
  //Arg->dump(OS, SymbolTable::Table->VB.BR->getSourceManager());

  QualType QTArg = Arg->getType();
  if (QTArg->isRecordType()) {
    CXXRecordDecl *RecDecl = QTArg->getAsCXXRecordDecl()->getCanonicalDecl();
    assert(RecDecl);

    // Iterate over the methods of the class, searching for the overloaded
    // call operator [operator ()].
    for(CXXRecordDecl::method_iterator I = RecDecl->method_begin(),
                                        E = RecDecl->method_end();
        I != E; ++I) {
      const CXXMethodDecl *Method = *I;
      QualType MethQT = Method->getType();
      std::string Name = Method->getNameInfo().getAsString();
      //OS << "DEBUG:: Method Name: " << Name << "\n";
      //Method->dump(OS);
      //OS << "\n";
      if (checkMethodType(MethQT, TakesParam) &&
          !Name.compare(CXX_CALL_OPERATOR)) {
        Result = Method;
        break;
      }
    } // end for
    if (Force) {
      assert(Result && "could not find overridden operator() "
                       "method to check parallel safety");
    }
    return Result;

  // fn-pointer style and lambda style parallel_invoke not supported yet
  /* FIXME: the Lambda case falls under the RecordDecl case but the call operator method is
     accessible through the function getLambdaCallOperator() which returns a CXXMethod*
     but only if isLambda() is true. All these are methods of the subtype CXXRecordDecl.
  } else if (QTArg->isLambdaType()) {
    //TODO
    emitNICheckNotImplemented(Arg, 0);
    return Result;*/
  } else {
    //TODO
    if (Force) { // when force = true we are not trying, we must succeed!
      emitNICheckNotImplemented(Arg, 0);
    }
    return Result;
  }

}

inline static const CXXMethodDecl
*getOperatorMethod(const Expr *Arg, bool TakesParam = false) {
  const CXXMethodDecl *Result = tryGetOperatorMethod(Arg, TakesParam, true);
  return Result;
}

static std::auto_ptr<EffectSummary>
getInvokeEffectSummary(const Expr *Arg,
                       const CXXMethodDecl *Method, const FunctionDecl *Def) {
  EffectSummary *ES = 0;
  //raw_ostream &OS = *SymbolTable::VB.OS;

  if (SymbolTable::Table->getEffectSummary(Method)) {
    ES = new EffectSummary(*SymbolTable::Table->getEffectSummary(Method));
    //OS << "DEBUG:: ES0 EffectSummary:" << ES->toString() << "\n";
    const SubstitutionVector *SubVec = SymbolTable::Table->getInheritanceSubVec(Method->getParent());
    ES->substitute(SubVec);
    // perform 'this' substitution
    const NamedDecl *NamD = 0;
    if (isa<DeclRefExpr>(Arg)) {
      const DeclRefExpr *DeclRef = dyn_cast<DeclRefExpr>(Arg);
      NamD = DeclRef->getDecl();
    } else if (isa<MaterializeTemporaryExpr>(Arg)) {
      const MaterializeTemporaryExpr *MEx = dyn_cast<MaterializeTemporaryExpr>(Arg);
      Expr *Exp = MEx->GetTemporaryExpr();
      assert(Exp);
      Exp = Exp->IgnoreImplicit();
      if (isa<CXXFunctionalCastExpr>(Exp)) {
        Exp = dyn_cast<CXXFunctionalCastExpr>(Exp)->getSubExpr();
      }
      assert(Exp && isa<CXXConstructExpr>(Exp));
      CXXConstructExpr *CXXC = dyn_cast<CXXConstructExpr>(Exp);
      NamD = CXXC->getConstructor()->getParent();
      assert(NamD && "Internal Error: ValD should have been initialized");
    } else {
      emitUnexpectedTypeOfArgumentPassed(Arg, Def);
      return std::auto_ptr<EffectSummary>(0);
    }
    assert(NamD && "Internal Error: ValD should have been initialized");
    //OS << "DEBUG:: ValD = ";
    //NamD->print(OS);
    //OS << "\n";
    const ASaPType *T = SymbolTable::Table->getType(NamD);
    if (T) {
      //OS << "DEBUG: T= " << T->toString() << "\n";
      std::auto_ptr<SubstitutionVector> SubV = T->getSubstitutionVector();
      ES->substitute(SubV.get());
    }
  } else {
    // No effect summary recorded for this method, means we don't want/need
    // to check it. Nothing to do.
  }

  return std::auto_ptr<EffectSummary>(ES);
}

/////////////////////////////////////////////////////////////////////////////
// tbb::parallel_invoke

bool TBBParallelInvokeNIChecker::check(CallExpr *Exp, const FunctionDecl *Def) const {
  // for each of the arguments to this call (except the last which may be a
  // context argument) get its effects and make sure they don't interfere
  // FIXME add support for the trailing context argument
  bool Result = true;
  unsigned int NumArgs = Exp->getNumArgs();
  assert(NumArgs>=2 &&
         "tbb::parallel_invoke with fewer than two args is unexpected");
#ifndef EFFECT_SUMMARY_VECTOR_SIZE
#define EFFECT_SUMMARY_VECTOR_SIZE 8
#endif
  typedef llvm::SmallVector<EffectSummary*, EFFECT_SUMMARY_VECTOR_SIZE>
          EffectSummaryVector;
  EffectSummaryVector ESVec;

  for(unsigned int I = 0; I < NumArgs; ++I) {
    Expr *Arg = Exp->getArg(I)->IgnoreImplicit();
    std::auto_ptr<EffectSummary> ES =
        getInvokeEffectSummary(Arg,getOperatorMethod(Arg), Def);
    ESVec.push_back(ES.release());
  }

  // check non-interference of all pairs
  for(EffectSummaryVector::iterator I = ESVec.begin(), E = ESVec.end();
      I != E; ++I) {
    EffectSummaryVector::iterator J = I; ++J;
    while (J != E) {
      if ((*I) && (*I)->isNonInterfering(*J)==BaseEffectSummary::RK_FALSE) {
        assert(*J);
        emitInterferingEffects(Exp, *(*I), *(*J));
        Result = false;
      }
      else if ((*I) && (*I)->isNonInterfering(*J)==BaseEffectSummary::RK_DUNNO){
	assert(false && "Found variable effect summary");
      }
      ++J;
    }
  }
  // check effect coverage
  const EffectSummary *DefES = SymbolTable::Table->getEffectSummary(Def);
  assert(DefES);
  llvm::raw_ostream &OS = *SymbolTable::VB.OS;
  OS << "DEBUG:: Checking if the effects of the calls through parallel_invoke "
     << "are covered by the effect summary of the enclosing function, which is:\n"
     << DefES->toString() << "\n";
  {
    unsigned int Idx = 0;
    EffectSummaryVector::iterator I = ESVec.begin(), E = ESVec.end();
    for(; I != E; ++I, ++Idx) {
      assert(Idx<NumArgs && "Internal Error: Unexpected number of Effect Summaries");
      if (DefES->covers(*I)==BaseEffectSummary::RK_FALSE) {
        std::string Str = (*I)->toString();
        emitEffectsNotCoveredWarning(Exp->getArg(Idx), Def, Str);
        Result = false;
      }
      else if (DefES->covers(*I)==BaseEffectSummary::RK_DUNNO) {
	assert(false && "Found variable effect summary");
      }
    }
  }
  // delete effect summaries
  for(EffectSummaryVector::iterator I = ESVec.begin(), E = ESVec.end();
      I != E; ++I) {
    delete (*I);
  }
  return Result;
}

/////////////////////////////////////////////////////////////////////////////
// tbb::parallel_for

bool TBBParallelForRangeNIChecker::check(CallExpr *Exp, const FunctionDecl *Def) const {
  bool Result = true;
  // 1. Get the effect summary of the operator method of the 2nd argument
  Expr *Arg = Exp->getArg(TBB_PARFOR_RANGE_BODY_POSITION)->IgnoreImplicit();
  raw_ostream &OS = *SymbolTable::VB.OS;
  //QualType QTArg = Arg->getType();
  const CXXMethodDecl *Method = getOperatorMethod(Arg, true);
  std::auto_ptr<EffectSummary> ES = getInvokeEffectSummary(Arg, Method, Def);

  // 2. Detect induction variables
  // TODO InductionVarVector IVV = detectInductionVariablesVector

  // 3. Check non-interference
  if (ES->isNonInterfering(ES.get())==BaseEffectSummary::RK_FALSE) {
    emitInterferingEffects(Exp, *ES, *ES);
    Result = false;
  }
  else if (ES->isNonInterfering(ES.get())==BaseEffectSummary::RK_DUNNO) {
    assert(false && "Found variable effect summary");
  }
  // 4. Check effect coverage
  const EffectSummary *DefES = SymbolTable::Table->getEffectSummary(Def);
  assert(DefES);
  OS << "DEBUG:: Checking if the effects of the calls through parallel_for "
     << "are covered by the effect summary of the enclosing function, which is:\n"
     << DefES->toString() << "\n";
  // 4.1. TODO For each induction variable substitute it with [?] in ES
  // 4.2 check
  if (DefES->covers(ES.get())==BaseEffectSummary::RK_FALSE) {
    std::string Str = ES->toString();
    emitEffectsNotCoveredWarning(Arg, Def, Str);
    Result = false;
  }
  else if (DefES->covers(ES.get())==BaseEffectSummary::RK_DUNNO){
    assert(false && "Found variable effect summary");
  }
  // 5. Cleanup
  //delete(ES);
  return Result;
}

bool TBBParallelForIndexNIChecker::check(CallExpr *Exp, const FunctionDecl *Def) const {
  bool Result = true;
  raw_ostream &OS = *SymbolTable::VB.OS;
  // 1. Get the effect summary of the operator method of the 3rd or 4th argument
  Expr *Arg = Exp->getArg(TBB_PARFOR_INDEX2_FUNCTOR_POSITION)->IgnoreImplicit();
  const CXXMethodDecl *Method = tryGetOperatorMethod(Arg, true);
  if (!Method) {
    Arg = Exp->getArg(TBB_PARFOR_INDEX3_FUNCTOR_POSITION)->IgnoreImplicit();
    Method = getOperatorMethod(Arg, true);
  }
  std::auto_ptr<EffectSummary> ES = getInvokeEffectSummary(Arg, Method, Def);

  // 2. Detect induction variables
  // TODO InductionVarVector IVV = detectInductionVariablesVector

  // 3. Check non-interference
  if (ES->isNonInterfering(ES.get())==BaseEffectSummary::RK_FALSE) {
    emitInterferingEffects(Exp, *ES, *ES);
    Result = false;
  }
  else if (ES->isNonInterfering(ES.get())==BaseEffectSummary::RK_DUNNO) {
    assert(false && "Found variable effect summary");
  }
  // 4. Check effect coverage
  const EffectSummary *DefES = SymbolTable::Table->getEffectSummary(Def);
  assert(DefES);
  OS << "DEBUG:: Checking if the effects of the calls through parallel_for "
     << "are covered by the effect summary of the enclosing function, which is:\n"
     << DefES->toString() << "\n";
  // 4.1. TODO For each induction variable substitute it with [?] in ES
  // 4.2 check
  if (DefES->covers(ES.get())==BaseEffectSummary::RK_FALSE) {
    std::string Str = ES->toString();
    emitEffectsNotCoveredWarning(Arg, Def, Str);
    Result = false;
  }
  else if (DefES->covers(ES.get())==BaseEffectSummary::RK_FALSE) {
    assert(false && "Found variable effect summary");
  }
  // 5. Cleanup
  //delete(ES);
  return Result;
}

} // end namespace asap
} // end namespace clang
