//=== SafeParallelismChecker.cpp - Safe Parallelism checker --*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===--------------------------------------------------------------------===//
//
// This file defines the Safe Parallelism checker, which tries to prove the
// safety of parallelism given region and effect annotations.
//
//===--------------------------------------------------------------------===//

#include "ClangSACheckers.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/AnalysisManager.h"

#include "ASaPSymbolTable.h"
#include "CollectRegionNamesAndParameters.h"
#include "EffectChecker.h"
#include "EffectSummaryNormalizer.h"
#include "SemanticChecker.h"
#include "TypeChecker.h"


using namespace clang;
using namespace ento;
using namespace clang::asap;

namespace {

using clang::asap::SymbolTable;

/// 1. Wrapper pass that calls a Stmt visitor on each function definition.
template<typename StmtVisitorT>
class StmtVisitorInvoker :
  public RecursiveASTVisitor<StmtVisitorInvoker<StmtVisitorT> > {

private:
  /// Private Fields
  VisitorBundle &VB;

  bool FatalError;

public:
  /// Constructor
  explicit StmtVisitorInvoker(VisitorBundle &VB) : VB(VB), FatalError(false) {}

  bool shouldVisitTemplateInstantiations() const { return true; }
  bool shouldVisitImplicitCode() const { return true; }
  bool shouldWalkTypesOfTypeLocs() const { return true; }

  /// Getters & Setters
  inline bool encounteredFatalError() { return FatalError; }

  /// Visitors
  bool VisitFunctionDecl(FunctionDecl* D) {
    const FunctionDecl* Definition;
    if (D->hasBody(Definition)) {
      Stmt* S = Definition->getBody();
      assert(S);

      StmtVisitorT StmtVisitor(VB, Definition, S, true);

      FatalError |= StmtVisitor.encounteredFatalError();
    }
    return true;
  }
}; /// class StmtVisitorInvoker


// The template parameter is the type of the default annotation scheme
// used. This is a temporary solution since it is not easy to pass
// command line arguments to checkers.
template<typename AnnotationSchemeT>
class  SafeParallelismChecker
  : public Checker<check::ASTDecl<TranslationUnitDecl> > {

public:
  void checkASTDecl(const TranslationUnitDecl *TUDeclConst,
                    AnalysisManager &Mgr,
                    BugReporter &BR) const {

    TranslationUnitDecl *TUDecl = const_cast<TranslationUnitDecl*>(TUDeclConst);
    SymbolTable::Initialize();

    // initialize traverser
    SymbolTable &SymT = *SymbolTable::Table;
    AnnotationSchemeT AnnotScheme(SymT);
    SymT.setAnnotationScheme(&AnnotScheme);

    ASTContext &Ctx = TUDecl->getASTContext();
    AnalysisDeclContext *AC = Mgr.getAnalysisDeclContext(TUDecl);
    VisitorBundle VB = {BR, Ctx, Mgr, AC, os, SymT};

    os << "DEBUG:: starting ASaP Region Name & Parameter Collector\n";
    CollectRegionNamesAndParametersTraverser NameCollector(VB);
    NameCollector.TraverseDecl(TUDecl);
    os << "##############################################\n";
    os << "DEBUG:: done running ASaP Region Name & Parameter Collector\n\n";
    if (NameCollector.encounteredFatalError()) {
      os << "DEBUG:: NAME COLLECTOR ENCOUNTERED FATAL ERROR!! STOPPING\n";
      SymbolTable::Destroy();
      return;
    }

    os << "DEBUG:: starting ASaP Semantic Checker\n";
    ASaPSemanticCheckerTraverser SemanticChecker(VB);
    SemanticChecker.TraverseDecl(TUDecl);
    os << "##############################################\n";
    os << "DEBUG:: done running ASaP Semantic Checker\n\n";
    if (SemanticChecker.encounteredFatalError()) {
      os << "DEBUG:: SEMANTIC CHECKER ENCOUNTERED FATAL ERROR!! STOPPING\n";
      SymbolTable::Destroy();
      return;
    }

    os << "DEBUG:: starting ASaP Effect Coverage Checker\n";
    EffectSummaryNormalizerTraverser EffectNormalizerChecker(VB);
    EffectNormalizerChecker.TraverseDecl(TUDecl);
    os << "##############################################\n";
    os << "DEBUG:: done running ASaP Effect Coverage Checker\n\n";
    if (EffectNormalizerChecker.encounteredFatalError()) {
      os << "DEBUG:: EFFECT NORMALIZER CHECKER ENCOUNTERED FATAL ERROR!! STOPPING\n";
      SymbolTable::Destroy();
      return;
    }

    os << "DEBUG:: starting ASaP Type Checker\n";
    StmtVisitorInvoker<AssignmentCheckerVisitor> TypeChecker(VB);
    TypeChecker.TraverseDecl(TUDecl);
    os << "##############################################\n";
    os << "DEBUG:: done running ASaP Type Checker\n\n";
    if (TypeChecker.encounteredFatalError()) {
      os << "DEBUG:: Type Checker ENCOUNTERED FATAL ERROR!! STOPPING\n";
      SymbolTable::Destroy();
      return;
    }
    // Check that Effect Summaries cover effects
    StmtVisitorInvoker<EffectCollectorVisitor> EffectChecker(VB);
    EffectChecker.TraverseDecl(TUDecl);
    os << "##############################################\n";
    os << "DEBUG:: done running ASaP Effect Checker\n\n";
    if (EffectChecker.encounteredFatalError()) {
      os << "DEBUG:: Effect Checker ENCOUNTERED FATAL ERROR!! STOPPING\n";
      SymbolTable::Destroy();
      return;
    }
    SymbolTable::Destroy();
  }
}; // end class SafeParallelismChecker
} // end unnamed namespace

void ento::registerSafeParallelismChecker(CheckerManager &mgr) {
  mgr.registerChecker<SafeParallelismChecker<SimpleAnnotationScheme> >();
}

void ento::registerGlobalAccessChecker(CheckerManager &mgr) {
  mgr.registerChecker<SafeParallelismChecker<CheckGlobalsAnnotationScheme> >();
}
