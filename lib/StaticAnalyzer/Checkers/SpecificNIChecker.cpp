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

void emitNICheckNotImplemented(const Stmt *S, const FunctionDecl *FunD) {
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

void emitInterferingEffects(const Stmt *S,
                            const EffectSummary &ES1,
                            const EffectSummary &ES2) {
  StringRef BugName = "Interfering effects";
  std::string SBuf;
  llvm::raw_string_ostream OS(SBuf);
  OS << "{" << ES1.toString() << "} # {" << ES2.toString() << "}";
  StringRef Str(OS.str());
  helperEmitStatementWarning(*SymbolTable::VB.BR,
                             SymbolTable::VB.AC,
                             S, 0, Str, BugName, false);
}

bool TBBSpecificNIChecker::check(CallExpr *E) const {
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
  QualType RetQT = FT->getResultType();
  if (!RetQT->isVoidType())
    return false; // Technically we could allow any return type
  // Check that it takes no arguments
  if (FT->getNumArgs() != 0)
    return false;

  return true;
}

bool TBBParallelInvokeNIChecker::check(CallExpr *Exp) const {
  // for each of the arguments to this call (except the last which may be a
  // context argument) get its effects and make sure they don't interfere
  // FIXME just dealing with 2 params for now
  raw_ostream &OS = *SymbolTable::VB.OS;
  unsigned int NumArgs = Exp->getNumArgs();
  assert(NumArgs>=2 &&
         "tbb::parallel_invoke with fewer than two args makes no sense");
  Expr *Arg0 = Exp->getArg(0)->IgnoreImplicit();
  Expr *Arg1 = Exp->getArg(1)->IgnoreImplicit();

  QualType QTArg0 = Arg0->getType();
  QualType QTArg1 = Arg1->getType();

  // For now just try to support our very specific example
  if (QTArg0->isRecordType()) {
    assert(QTArg1->isRecordType());
    // Find operator() and its effects
    CXXRecordDecl *RecDecl0 = QTArg0->getAsCXXRecordDecl()->getCanonicalDecl();
    CXXRecordDecl *RecDecl1 = QTArg1->getAsCXXRecordDecl()->getCanonicalDecl();
    EffectSummary *ES0, *ES1;

    for(CXXRecordDecl::method_iterator I = RecDecl0->method_begin(),
                                        E = RecDecl0->method_end();
         I != E; ++I) {
      CXXMethodDecl *Method = *I;
      QualType MethQT = Method->getType();
      std::string Name = Method->getNameInfo().getAsString();
      OS << "DEBUG:: Method Name: " << Name << "\n";
      if (isParInvokOperator(MethQT) &&
          !Name.compare("operator()")) {
        ES0 = new EffectSummary(*SymbolTable::Table->getEffectSummary(Method));
        if (ES0) {
          OS << "DEBUG:: ES0 EffectSummary:" << ES0->toString() << "\n";
        } else {
          OS << "DEBUG:: ES0 : No EffectSummary...\n";
        }
        break;
      }
    } // end for


    for(CXXRecordDecl::method_iterator I = RecDecl1->method_begin(),
                                        E = RecDecl1->method_end();
         I != E; ++I) {
      CXXMethodDecl *Method = *I;
      QualType MethQT = Method->getType();
      std::string Name = Method->getNameInfo().getAsString();
      OS << "DEBUG:: Method Name: " << Name << "\n";
      if (isParInvokOperator(MethQT) &&
          !Name.compare("operator()")) {
        ES1 = new EffectSummary(*SymbolTable::Table->getEffectSummary(Method));
        if (ES1) {
          OS << "DEBUG:: ES1 EffectSummary:" << ES1->toString() << "\n";
        } else {
          OS << "DEBUG:: ES1 : No EffectSummary...\n";
        }
        break;
      }
    } // end for
    // TODO: perform substitution on EffectSummaries (from the implicit call
    // to operator () and the argument to parallel_invoke )
    const SubstitutionVector *SubVec0 = SymbolTable::Table->getInheritanceSubVec(RecDecl0);
    /*if (SubVec0)
      SubVec0->applyTo(ES0);*/
    ES0->substitute(SubVec0);
    assert(isa<DeclRefExpr>(Arg0));
    DeclRefExpr *DeclRef0 = dyn_cast<DeclRefExpr>(Arg0);
    ValueDecl *ValD0 = DeclRef0->getDecl();
    const ASaPType *T0 = SymbolTable::Table->getType(ValD0);
    OS << "DEBUG:: 1\n";
    assert(T0);
    std::auto_ptr<SubstitutionVector> SubV0 = T0->getSubstitutionVector();
    OS << "DEBUG:: 2\n";
    ES0->substitute(SubV0.get());
    OS << "DEBUG:: 3\n";

    const SubstitutionVector *SubVec1 = SymbolTable::Table->getInheritanceSubVec(RecDecl1);
    /*if (SubVec1)
      SubVec0->applyTo(ES1);*/
    ES1->substitute(SubVec1);

    assert(isa<DeclRefExpr>(Arg1));
    DeclRefExpr *DeclRef1 = dyn_cast<DeclRefExpr>(Arg1);
    ValueDecl *ValD1 = DeclRef1->getDecl();
    const ASaPType *T1 = SymbolTable::Table->getType(ValD1);
    std::auto_ptr<SubstitutionVector> SubV1 = T1->getSubstitutionVector();
    ES1->substitute(SubV1.get());
    // TODO: check non-interference
    OS << "DEBUG:: just need to check Non-Interference of ES0 and ES1:\n"
       << "ES0: " << ES0->toString() << "\n"
       << "ES1: " << ES1->toString() << "\n";
    //if (ES0->nonInterferring(ES1))
    if(!ES0->isNonInterfering(ES1)) {
      emitInterferingEffects(Exp, *ES0, *ES1);
    }
  } else {
    emitNICheckNotImplemented(Exp, 0);
  }


  return true;
}

} // end namespace asap
} // end namespace clang
