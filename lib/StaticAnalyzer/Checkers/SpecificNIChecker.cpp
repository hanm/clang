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
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Expr.h"

#include "ASaPSymbolTable.h"
#include "ASaPType.h"
#include "Effect.h"
#include "SpecificNIChecker.h"

namespace clang {
namespace asap {

static void emitNICheckNotImplemented(const Stmt *S, const FunctionDecl *FunD) {
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

static void emitInterferingEffects(const Stmt *S,
                                   const EffectSummary &ES1,
                                   const EffectSummary &ES2) {
  StringRef BugName = "Interfering effects";
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
  StringRef BugName = "effects not covered by effect summary";
  helperEmitStatementWarning(*SymbolTable::VB.BR,
                             SymbolTable::VB.AC,
                             S, D, Str, BugName);
}

bool TBBSpecificNIChecker::check(CallExpr *E, const FunctionDecl *Def) const {
  emitNICheckNotImplemented(E, 0);
  return false;
}

// detect 'void (void) const' type
bool isParInvokOperator(QualType MethQT) {
  if (!MethQT->isFunctionType())
    return false;
  const FunctionProtoType *FT = MethQT->getAs<FunctionProtoType>();
  assert(FT);
  // Check that return type is void
  QualType RetQT = FT->getReturnType();
  if (!RetQT->isVoidType())
    return false; // Technically we could allow any return type
  // Check that it takes no arguments
  if (FT->getNumParams() != 0)
    return false;

  return true;
}

std::auto_ptr<EffectSummary> getInvokeEffectSummary(Expr *Arg) {
  raw_ostream &OS = *SymbolTable::VB.OS;
  QualType QTArg = Arg->getType();
  if (QTArg->isRecordType()) {
    CXXRecordDecl *RecDecl = QTArg->getAsCXXRecordDecl()->getCanonicalDecl();
    EffectSummary *ES;

    for(CXXRecordDecl::method_iterator I = RecDecl->method_begin(),
                                        E = RecDecl->method_end();
        I != E; ++I) {
      CXXMethodDecl *Method = *I;
      QualType MethQT = Method->getType();
      std::string Name = Method->getNameInfo().getAsString();
      OS << "DEBUG:: Method Name: " << Name << "\n";
      if (isParInvokOperator(MethQT) &&
          !Name.compare("operator()")) {
        ES = new EffectSummary(*SymbolTable::Table->getEffectSummary(Method));
        if (ES) {
          OS << "DEBUG:: ES0 EffectSummary:" << ES->toString() << "\n";
        } else {
          OS << "DEBUG:: ES0 : No EffectSummary...\n";
        }
        break;
      }
    } // end for

    // perform substitution on EffectSummaries (from the implicit call
    // to operator () and the argument to parallel_invoke )
    const SubstitutionVector *SubVec = SymbolTable::Table->getInheritanceSubVec(RecDecl);
    ES->substitute(SubVec);
    // perform 'this' substitution
    assert(isa<DeclRefExpr>(Arg));
    DeclRefExpr *DeclRef = dyn_cast<DeclRefExpr>(Arg);
    ValueDecl *ValD = DeclRef->getDecl();
    const ASaPType *T = SymbolTable::Table->getType(ValD);
    assert(T);
    std::auto_ptr<SubstitutionVector> SubV = T->getSubstitutionVector();
    ES->substitute(SubV.get());
    return std::auto_ptr<EffectSummary>(ES);
  //} else if (QTArg->isLambdaType()) {
    //TODO
  } else {
    // fn-pointer style and lambda style parallel_invoke not supported yet
    emitNICheckNotImplemented(Arg, 0);
    return std::auto_ptr<EffectSummary>(0);
  }
}

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
    std::auto_ptr<EffectSummary> ES = getInvokeEffectSummary(Arg);
    ESVec.push_back(ES.release());
  }

  // check non-interference of all pairs
  for(EffectSummaryVector::iterator I = ESVec.begin(), E = ESVec.end();
      I != E; ++I) {
    EffectSummaryVector::iterator J = I; ++J;
    while (J != E) {
      if ((*I) && !(*I)->isNonInterfering(*J)) {
        assert(*J);
        emitInterferingEffects(Exp, *(*I), *(*J));
        Result = false;
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
      if (!DefES->covers(*I)) {
        std::string Str = (*I)->toString();
        emitEffectsNotCoveredWarning(Exp->getArg(Idx), Def, Str);
        Result = false;
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

} // end namespace asap
} // end namespace clang
