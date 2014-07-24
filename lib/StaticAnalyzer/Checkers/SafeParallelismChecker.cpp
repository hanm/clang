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
#include <stdio.h>

#include "ClangSACheckers.h"
#include "clang/AST/DeclBase.h"
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
  bool VisitFunctionDecl(FunctionDecl *D) {
    const FunctionDecl *Definition;
    if (D->hasBody(Definition)) {
      Stmt *S = Definition->getBody();
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
    // Initialize Prolog otherwise assert won't work
    // Known swipl bug: http://www.swi-prolog.org/bugzilla/show_bug.cgi?id=41
    // Bug 41 - swi-prolog overrides all assertions by defining __assert_fail
    char Libpl[] = "libpl.dll";
    char G32[] = "-G32m";
    char L32[] = "-L32m";
    char T32[] = "-T32m";

    char* Argv[4] = {Libpl, G32, L32, T32};
    int Argc = 4;
    PL_initialise(Argc, Argv);
    //PL_action(PL_ACTION_DEBUG);
    //PL_action(PL_ACTION_TRACE);

    bool Error = false;
    TranslationUnitDecl *TUDecl = const_cast<TranslationUnitDecl*>(TUDeclConst);
    ASTContext &Ctx = TUDecl->getASTContext();
    AnalysisDeclContext *AC = Mgr.getAnalysisDeclContext(TUDecl);
    VisitorBundle VB = {this, &BR, &Ctx, &Mgr, AC, &os};
    SymbolTable::Initialize(VB);

    // initialize traverser
    SymbolTable &SymT = *SymbolTable::Table;

    // Choose default Annotation Scheme from command-line option
    const StringRef OptionName("-asap-default-scheme");
    StringRef SchemeStr(Mgr.getAnalyzerOptions().Config.GetOrCreateValue(OptionName, "simple").getValue());
    os << "DEBUG:: asap-default-scheme = " << SchemeStr << "\n";

    AnnotationScheme *AnnotScheme = 0;
    bool DoEffectInference = false;
    bool DoFullInference = false;
    if (SchemeStr.compare("simple") == 0) {
      AnnotScheme = new SimpleAnnotationScheme(SymT);
    } else if (SchemeStr.compare("param") == 0
              || SchemeStr.compare("parametric") == 0) {
      AnnotScheme = new ParametricAnnotationScheme(SymT);
    } else if (SchemeStr.compare("global") == 0) {
      AnnotScheme = new CheckGlobalsAnnotationScheme(SymT);
    } else if (SchemeStr.compare("effect-inference") == 0
              || SchemeStr.compare("simple-effect-inference") == 0) {
      AnnotScheme = new SimpleEffectInferenceAnnotationScheme(SymT);
      DoEffectInference = true;
    } else if (SchemeStr.compare("parametric-effect-inference") == 0) {
      AnnotScheme = new ParametricEffectInferenceAnnotationScheme(SymT);
      DoEffectInference = true;
    } else if (SchemeStr.compare("inference") == 0) {
      AnnotScheme = new InferenceAnnotationScheme(SymT);
      DoFullInference = true;
    } else {
      StringRef BugName = "Invalid argument to command-line flag -asap-default-scheme  flag";
      os << "DEBUG:: Here1 \n";
      assert(TUDecl->noload_decls_begin() != TUDecl->noload_decls_end());
      DeclContext::decl_iterator I = TUDecl->decls_begin(),
                    E = TUDecl->decls_end();
      while (I != E && (*I)->getLocation().isInvalid()) {
        ++I;
      }
      assert(I!=E);
      Decl *D = *I;
      helperEmitDeclarationWarning(this, BR, D, SchemeStr, BugName);
      Error = true;
    }

    if (!Error) {
      SymT.setAnnotationScheme(AnnotScheme);
      runCheckers(TUDecl, DoEffectInference, DoFullInference);
    }


    SymbolTable::Destroy();
    delete AnnotScheme;
    PL_cleanup(0);
  }

  void runCheckers(TranslationUnitDecl *TUDecl,
                   bool DoEffectInference, bool DoFullInference) const {
    assert(!(DoEffectInference && DoFullInference) &&
           "Either effect or full inference can be performed");
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
    os << "DEBUG:: starting ASaP Effect Constraint Generator\n";
    //StmtVisitorInvoker<EffectCollectorVisitor> EffectChecker;
    StmtVisitorInvoker<EffectConstraintVisitor> EffectChecker;
    EffectChecker.TraverseDecl(TUDecl);
    os << "##############################################\n";
    os << "DEBUG:: done running ASaP Effect Constraint Generator\n\n";
    if (EffectChecker.encounteredFatalError()) {
      os << "DEBUG:: Effect Checker ENCOUNTERED FATAL ERROR!! STOPPING\n";
      return;
    }

    if (DoEffectInference) {
      setupProlog();
      SymbolTable::Table->solveConstraints();
    } else {
      os << "DEBUG:: starting ASaP Non-Interference Checking\n";
      StmtVisitorInvoker<NonInterferenceChecker> NonIChecker;
      NonIChecker.TraverseDecl(TUDecl);
      os << "##############################################\n";
      os << "DEBUG:: done running ASaP Non-Interference Checking\n\n";
      if (NonIChecker.encounteredFatalError()) {
        os << "DEBUG:: NON-INTERFERENCE CHECKING ENCOUNTERED FATAL ERROR!! STOPPING\n";
        return;
      }
      if (DoFullInference) {
        setupProlog();
        SymbolTable::Table->solveConstraints();
        // TODO
      }
    } // end else (!DoEffectInference)
  }

  void setupProlog() const {
    // Make sure asap.pl exists at the expected location.
    FILE *File = fopen("/opt/lib/asap.pl", "r");
    if (!File) {
      fclose(File);
      assert(File && "Prolog rules file does not exist");
    }
    // Consult the asap.pl file
    predicate_t Consult = PL_predicate("consult", 1, "user");
    term_t Plfile = PL_new_term_ref();
    PL_put_atom_chars(Plfile, "/opt/lib/asap.pl");
    PL_call_predicate(NULL, PL_Q_NORMAL, Consult, Plfile);
  }

}; // end class SafeParallelismChecker
} // end unnamed namespace

void ento::registerSafeParallelismChecker(CheckerManager &Mgr) {
  Mgr.registerChecker<SafeParallelismChecker>();
}
