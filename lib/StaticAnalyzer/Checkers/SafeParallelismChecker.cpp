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

#include "SemanticChecker.h"
#include "TypeChecker.h"
#include "EffectChecker.h"

//#include <typeinfo>

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
  void checkASTDecl(const TranslationUnitDecl *D,
                    AnalysisManager &Mgr,
                    BugReporter &BR) const {
    SymbolTable::Initialize();

    // initialize traverser
    SymbolTable SymT;
    AnnotationSchemeT AnnotScheme(SymT);
    SymT.setAnnotationScheme(&AnnotScheme);

    ASTContext &Ctx = D->getASTContext();
    AnalysisDeclContext *AC = Mgr.getAnalysisDeclContext(D);
    VisitorBundle VB = {BR, Ctx, Mgr, AC, os, SymT};

    os << "DEBUG:: starting ASaP Semantic Checker\n";
    ASaPSemanticCheckerTraverser SemanticChecker(VB);
    // run checker
    SemanticChecker.TraverseDecl(const_cast<TranslationUnitDecl*>(D));
    os << "##############################################\n";
    os << "DEBUG:: done running ASaP Semantic Checker\n\n";
    if (SemanticChecker.encounteredFatalError()) {
      os << "DEBUG:: SEMANTIC CHECKER ENCOUNTERED FATAL ERROR!! STOPPING\n";
    } else {
      // else continue with Typechecking
      StmtVisitorInvoker<AssignmentCheckerVisitor> TypeChecker(VB);
      TypeChecker.TraverseDecl(const_cast<TranslationUnitDecl*>(D));
      os << "##############################################\n";
      os << "DEBUG:: done running ASaP Type Checker\n\n";
      if (TypeChecker.encounteredFatalError()) {
        os << "DEBUG:: Type Checker ENCOUNTERED FATAL ERROR!! STOPPING\n";
      } else {
        // Check that Effect Summaries cover effects
        StmtVisitorInvoker<EffectCollectorVisitor> EffectChecker(VB);
        EffectChecker.TraverseDecl(const_cast<TranslationUnitDecl*>(D));
        os << "##############################################\n";
        os << "DEBUG:: done running ASaP Effect Checker\n\n";
        if (EffectChecker.encounteredFatalError()) {
          os << "DEBUG:: Effect Checker ENCOUNTERED FATAL ERROR!! STOPPING\n";
        }
      }
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
