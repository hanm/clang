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
#include <SWI-Prolog.h>
#include "ClangSACheckers.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/AnalysisManager.h"

#include "ASaP/ASaPSymbolTable.h"
#include "ASaP/CollectRegionNamesAndParameters.h"
#include "ASaP/DetectTBBParallelism.h"
#include "ASaP/EffectConstraintGeneration.h"
#include "ASaP/EffectSummaryNormalizer.h"
#include "ASaP/NonInterferenceChecker.h"
#include "ASaP/SemanticChecker.h"
#include "ASaP/TypeChecker.h"


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
  bool FatalError;

public:
  /// Constructor
  explicit StmtVisitorInvoker() : FatalError(false) {}

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

      StmtVisitorT StmtVisitor(Definition, S, true);

      FatalError |= StmtVisitor.encounteredFatalError();
    }
    return true;
  }
}; /// class StmtVisitorInvoker


class  SafeParallelismChecker
  : public Checker<check::ASTDecl<TranslationUnitDecl> > {

public:
  void checkASTDecl(const TranslationUnitDecl *TUDeclConst,
                    AnalysisManager &Mgr,
                    BugReporter &BR) const {
    bool Error = false;
    TranslationUnitDecl *TUDecl = const_cast<TranslationUnitDecl*>(TUDeclConst);
    ASTContext &Ctx = TUDecl->getASTContext();
    AnalysisDeclContext *AC = Mgr.getAnalysisDeclContext(TUDecl);
    VisitorBundle VB = {&BR, &Ctx, &Mgr, AC, &os};
    SymbolTable::Initialize(VB);

    // initialize traverser
    SymbolTable &SymT = *SymbolTable::Table;

    // Choose default Annotation Scheme from command-line option
    const StringRef OptionName("-asap-default-scheme");
    StringRef SchemeStr(Mgr.getAnalyzerOptions().Config.GetOrCreateValue(OptionName, "simple").getValue());
    os << "DEBUG:: asap-default-scheme = " << SchemeStr << "\n";

    AnnotationScheme *AnnotScheme = 0;
    if (SchemeStr.compare("simple") == 0) {
      AnnotScheme = new SimpleAnnotationScheme(SymT);
    } else if (SchemeStr.compare("param") == 0) {
      AnnotScheme = new ParametricAnnotationScheme(SymT);
    } else if (SchemeStr.compare("global") == 0) {
      AnnotScheme = new CheckGlobalsAnnotationScheme(SymT);
    } else if (SchemeStr.compare("inference") == 0) {
      AnnotScheme = new InferenceAnnotationScheme(SymT);
    } else {
      // ERROR TODO
      llvm::errs() << "ERROR: Invalid argument to command-line option -asap-default-scheme\n";
      Error = true;
    }

    if (!Error) {
      SymT.setAnnotationScheme(AnnotScheme);
      runCheckers(TUDecl);
    }


    SymbolTable::Destroy();
    delete AnnotScheme;
  }

  void runCheckers(TranslationUnitDecl *TUDecl) const {
    os << "DEBUG:: starting ASaP TBB Parallelism Detection!\n";
    DetectTBBParallelism DetectTBBPar;
    DetectTBBPar.TraverseDecl(TUDecl);
    os << "##############################################\n";
    os << "DEBUG:: done running ASaP TBB Parallelism Detection\n\n";
    if (DetectTBBPar.encounteredFatalError()) {
      os << "DEBUG:: TBB PARALLELISM DETECTION ENCOUNTERED FATAL ERROR!! STOPPING\n";
      return;
    }

    os << "DEBUG:: starting ASaP Region Name & Parameter Collector\n";
    CollectRegionNamesAndParametersTraverser NameCollector;
    NameCollector.TraverseDecl(TUDecl);
    os << "##############################################\n";
    os << "DEBUG:: done running ASaP Region Name & Parameter Collector\n\n";
    if (NameCollector.encounteredFatalError()) {
      os << "DEBUG:: NAME COLLECTOR ENCOUNTERED FATAL ERROR!! STOPPING\n";
      return;
    }

    os << "DEBUG:: starting ASaP Semantic Checker\n";
    ASaPSemanticCheckerTraverser SemanticChecker;
    SemanticChecker.TraverseDecl(TUDecl);
    os << "##############################################\n";
    os << "DEBUG:: done running ASaP Semantic Checker\n\n";
    if (SemanticChecker.encounteredFatalError()) {
      os << "DEBUG:: SEMANTIC CHECKER ENCOUNTERED FATAL ERROR!! STOPPING\n";
      return;
    }

    os << "DEBUG:: starting ASaP Effect Coverage Checker\n";
    EffectSummaryNormalizerTraverser EffectNormalizerChecker;
    EffectNormalizerChecker.TraverseDecl(TUDecl);
    os << "##############################################\n";
    os << "DEBUG:: done running ASaP Effect Normalizer Checker\n\n";
    if (EffectNormalizerChecker.encounteredFatalError()) {
      os << "DEBUG:: EFFECT NORMALIZER CHECKER ENCOUNTERED FATAL ERROR!! STOPPING\n";
      return;
    }

    os << "DEBUG:: starting ASaP Type Checker\n";
    StmtVisitorInvoker<AssignmentCheckerVisitor> TypeChecker;
    TypeChecker.TraverseDecl(TUDecl);
    os << "##############################################\n";
    os << "DEBUG:: done running ASaP Type Checker\n\n";
    if (TypeChecker.encounteredFatalError()) {
      os << "DEBUG:: Type Checker ENCOUNTERED FATAL ERROR!! STOPPING\n";
      return;
    }
    // Check that Effect Summaries cover effects
    //StmtVisitorInvoker<EffectCollectorVisitor> EffectChecker;
    StmtVisitorInvoker<EffectConstraintVisitor> EffectChecker;
    EffectChecker.TraverseDecl(TUDecl);
    os << "##############################################\n";
    os << "DEBUG:: done running ASaP Constraint Generator\n\n";
    if (EffectChecker.encounteredFatalError()) {
      os << "DEBUG:: Effect Checker ENCOUNTERED FATAL ERROR!! STOPPING\n";
      return;
    }
    SymbolTable &SymT = *SymbolTable::Table;

    int Argc; 
    
    char Libpl[] = "libpl.dll";
    char G32[] = "-G32m";
    char L32[] = "-L32m";
    char T32[] = "-T32m";

    char* Argv[4] = {Libpl, G32, L32, T32};
    Argc = 4; 

    PL_initialise(Argc, Argv); 
    //PL_action(PL_ACTION_DEBUG);
    //PL_action(PL_ACTION_TRACE);
    predicate_t Consult = PL_predicate("consult",1,"user");
    term_t Plfile=PL_new_term_ref();
    PL_put_atom_chars(Plfile, "/opt/lib/asap.pl");
    PL_call_predicate(NULL, PL_Q_NORMAL, Consult, Plfile); 

    SymT.solveInclusionConstraints();

    PL_cleanup(0);

    os << "DEBUG:: starting ASaP Non-Interference Checking\n";
    StmtVisitorInvoker<NonInterferenceChecker> NonIChecker;
    NonIChecker.TraverseDecl(TUDecl);
    os << "##############################################\n";
    os << "DEBUG:: done running ASaP Non-Interference Checking\n\n";
    if (NonIChecker.encounteredFatalError()) {
      os << "DEBUG:: NON-INTERFERENCE CHECKING ENCOUNTERED FATAL ERROR!! STOPPING\n";
      return;
    }

  }
}; // end class SafeParallelismChecker
} // end unnamed namespace

void ento::registerSafeParallelismChecker(CheckerManager &mgr) {
  mgr.registerChecker<SafeParallelismChecker>();
}
