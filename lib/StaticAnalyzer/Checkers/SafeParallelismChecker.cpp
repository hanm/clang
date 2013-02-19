//=== SafeParallelismChecker.cpp - Safe Parallelism checker -----*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This files defines the Safe Parallelism checker, which tries to prove the
// safety of parallelism given region and effect annotations
//
//===----------------------------------------------------------------------===//

#include "ClangSACheckers.h"
#include "clang/AST/Attr.h"
#include "clang/AST/StmtVisitor.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/CheckerManager.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

#include <typeinfo>

using namespace clang;
using namespace ento;

#define ASAP_DEBUG
#define ASAP_DEBUG_VERBOSE2

#ifdef ASAP_DEBUG
static raw_ostream& os = llvm::errs();
#else
static raw_ostream& os = llvm::nulls();
#endif

#ifdef ASAP_DEBUG_VERBOSE2
static raw_ostream& OSv2 = llvm::errs();
#else
static raw_ostream& OSv2 = llvm::nulls();
#endif

template<typename T>
static void destroyVector(T &V) {
  for (typename T::const_iterator I = V.begin(), E = V.end(); I != E; ++I)
    delete(*I);
}

template<typename T>
static void destroyVectorVector(T &V) {
  for (typename T::const_iterator I = V.begin(), E = V.end(); I != E; ++I) {
    destroyVector(*(*I));
    delete(*I);
  }
}

inline bool isNonPointerScalarType(QualType QT) {
  return (QT->isScalarType() && !QT->isPointerType());
}

#include "asap/RplElement.cpp"
#include "asap/Rpl.cpp"
#include "asap/Effect.cpp"

#include "asap/ASaPType.cpp"
#include "asap/ASaPSymbolTable.cpp"

namespace {
///-///////////////////////////////////////////////////////////////////
/// GENERIC VISITORS
using ASaP::SymbolTable;

/// 1. Wrapper pass that calls a Stmt visitor on each function definition.
template<typename StmtVisitorTy>
class StmtVisitorInvoker :
  public RecursiveASTVisitor<StmtVisitorInvoker<StmtVisitorTy> > {

private:
  /// Private Fields
  ento::BugReporter &BR;
  ASTContext &Ctx;
  AnalysisManager &Mgr;
  AnalysisDeclContext *AC;
  raw_ostream &OS;

  SymbolTable &SymT;

  bool FatalError;

public:
  /// Constructor
  explicit StmtVisitorInvoker(
    ento::BugReporter &BR, ASTContext &Ctx,
    AnalysisManager &Mgr, AnalysisDeclContext *AC, raw_ostream &OS,
    SymbolTable &SymT)
      : BR(BR),
        Ctx(Ctx),
        Mgr(Mgr),
        AC(AC),
        OS(OS),
        SymT(SymT),
        FatalError(false)
  {}

  /// Getters & Setters
  inline bool encounteredFatalError() { return FatalError; }

  /// Visitors
  bool VisitFunctionDecl(FunctionDecl* D) {
    const FunctionDecl* Definition;
    if (D->hasBody(Definition)) {
      Stmt* S = Definition->getBody();
      assert(S);

      StmtVisitorTy StmtVisitor(BR, Ctx, Mgr, AC, OS, SymT, Definition, S);

      FatalError |= StmtVisitor.encounteredFatalError();
    }
    return true;
  }
}; /// class StmtVisitorInvoker

/// TODO Create a base class for my statement visitors that visit
/// the code of entire function definitions. The tricky part is,
/// do we want this base class to follow CRTP (Curiously Recurring
/// template pattern
//template<typename CustomVisitorTy>
class ASaPStmtVisitorBase
    : public StmtVisitor<ASaPStmtVisitorBase/*<CustomVisitorTy>*/ > {

protected:
  /// Fields
  ento::BugReporter &BR;
  ASTContext &Ctx;
  AnalysisManager &Mgr;
  AnalysisDeclContext *AC;
  raw_ostream &OS;

  SymbolTable &SymT;

  const FunctionDecl *Def;
  bool FatalError;

public:
  /// Constructor
  ASaPStmtVisitorBase (
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
        FatalError(false) {
      //Visit(S);
    }

  /// Getters
  inline bool encounteredFatalError() { return FatalError; }

  /// Visitors
  void VisitChildren(Stmt *S) {
    for (Stmt::child_iterator I = S->child_begin(), E = S->child_end();
         I!=E; ++I)
      if (Stmt *child = *I)
        Visit(child);
  }

  void VisitStmt(Stmt *S) {
    VisitChildren(S);
  }

}; // end class StmtVisitor

/// FIXME temporarily just using pre-processor to concatenate code here... UGLY
#include "asap/SemanticChecker.cpp"
#include "asap/TypeChecker.cpp"
#include "asap/EffectChecker.cpp"

class  SafeParallelismChecker
  : public Checker<check::ASTDecl<TranslationUnitDecl> > {

public:
  void checkASTDecl(const TranslationUnitDecl *D,
                    AnalysisManager &Mgr,
                    BugReporter &BR) const {
    os << "DEBUG:: starting ASaP Semantic Checker\n";

    //BuiltinDefaulrRegionParam = ::new(D->getASTContext())
      //RegionParamAttr(D->getSourceRange(), D->getASTContext(), "P");

    /** initialize traverser */
    SymbolTable SymT;
    ASTContext &Ctx = D->getASTContext();
    AnalysisDeclContext *AC = Mgr.getAnalysisDeclContext(D);


    ASaPSemanticCheckerTraverser
      SemanticChecker(BR, Ctx, AC, os, SymT);
    /** run checker */
    SemanticChecker.TraverseDecl(const_cast<TranslationUnitDecl*>(D));
    os << "##############################################\n";
    os << "DEBUG:: done running ASaP Semantic Checker\n\n";
    if (SemanticChecker.encounteredFatalError()) {
      os << "DEBUG:: SEMANTIC CHECKER ENCOUNTERED FATAL ERROR!! STOPPING\n";
    } else {
      // else continue with Typechecking
      StmtVisitorInvoker<AssignmentCheckerVisitor>
        TypeChecker(BR, Ctx, Mgr, AC, os, SymT);
      TypeChecker.TraverseDecl(const_cast<TranslationUnitDecl*>(D));
      os << "##############################################\n";
      os << "DEBUG:: done running ASaP Type Checker\n\n";
      if (TypeChecker.encounteredFatalError()) {
        os << "DEBUG:: Type Checker ENCOUNTERED FATAL ERROR!! STOPPING\n";
      } else {
        // TODO check for fatal errors during typechecking
        // else continue with Effects Checking
        // Check that Effect Summaries cover effects
        StmtVisitorInvoker<EffectCollectorVisitor>
          EffectChecker(BR, Ctx, Mgr, AC, os, SymT);

        EffectChecker.TraverseDecl(const_cast<TranslationUnitDecl*>(D));
        os << "##############################################\n";
        os << "DEBUG:: done running ASaP Effect Checker\n\n";
        if (EffectChecker.encounteredFatalError()) {
          os << "DEBUG:: Effect Checker ENCOUNTERED FATAL ERROR!! STOPPING\n";
        }
      }
    }

    // Clean-Up
    delete ROOTRplElmt;
    delete LOCALRplElmt;
    delete STARRplElmt;
    // Note: implicit deallocation of SymT
  }
}; // end class SafeParallelismChecker
} // end unnamed namespace

void ento::registerSafeParallelismChecker(CheckerManager &mgr) {
  mgr.registerChecker<SafeParallelismChecker>();
}
