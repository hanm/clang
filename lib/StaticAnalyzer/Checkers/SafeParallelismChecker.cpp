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
    // Choose debug level
    const StringRef DbgLvlOptionName("-asap-debug-level");
    StringRef DbgLvlStr(Mgr.getAnalyzerOptions().Config.
                              GetOrCreateValue(DbgLvlOptionName, "0").
                              getValue());
    int DbgLvl = std::stoi(DbgLvlStr);
    if (DbgLvl > 0)
      OS = &llvm::errs();
    if (DbgLvl > 1)
      OSv2 =  &llvm::errs();

    *OS << "DEBUG:: " << DbgLvlOptionName << " = " << DbgLvl << "\n";

    // Initialize Symbol Table
    TranslationUnitDecl *TUDecl = const_cast<TranslationUnitDecl*>(TUDeclConst);
    ASTContext &Ctx = TUDecl->getASTContext();
    AnalysisDeclContext *AC = Mgr.getAnalysisDeclContext(TUDecl);
    VisitorBundle VB = {this, &BR, &Ctx, &Mgr, AC, OS};
    SymbolTable::Initialize(VB);
    SymbolTable &SymT = *SymbolTable::Table;

    // Choose prolog debug level.
    // 0: no debug (default)
    // 1: debug inference
    // 2: debug emitting constraints & inference
    // 3: debug emitting facts, constraints, & inference
    const StringRef PrologDbgLvlOptionName("-asap-debug-prolog");
    StringRef PrologDbgLvlStr(Mgr.getAnalyzerOptions().Config.
                              GetOrCreateValue(PrologDbgLvlOptionName, "0").
                              getValue());
    int PrologDbgLvl = std::stoi(PrologDbgLvlStr);
    *OS << "DEBUG:: " << PrologDbgLvlOptionName << " = " << PrologDbgLvl << "\n";

    // Choose default Annotation Scheme from command-line option
    const StringRef DefaultAnnotOptionName("-asap-default-scheme");
    StringRef SchemeStr(Mgr.getAnalyzerOptions().Config.
                        GetOrCreateValue(DefaultAnnotOptionName, "simple").
                        getValue());
    *OS << "DEBUG:: asap-default-scheme = " << SchemeStr << "\n";
    SymT.setPrologDbgLvl(PrologDbgLvl);

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
    *OS << "DEBUG:: starting ASaP TBB Parallelism Detection!\n";
    DetectTBBParallelism DetectTBBPar;
    DetectTBBPar.TraverseDecl(TUDecl);
    *OS << "##############################################\n";
    *OS << "DEBUG:: done running ASaP TBB Parallelism Detection\n\n";
    if (DetectTBBPar.encounteredFatalError()) {
      *OS << "DEBUG:: TBB PARALLELISM DETECTION ENCOUNTERED FATAL ERROR!! STOPPING\n";
      return;
    }

    *OS << "DEBUG:: starting ASaP Region Name & Parameter Collector\n";
    CollectRegionNamesAndParametersTraverser NameCollector;
    NameCollector.TraverseDecl(TUDecl);
    *OS << "##############################################\n";
    *OS << "DEBUG:: done running ASaP Region Name & Parameter Collector\n\n";
    if (NameCollector.encounteredFatalError()) {
      *OS << "DEBUG:: NAME COLLECTOR ENCOUNTERED FATAL ERROR!! STOPPING\n";
      return;
    }

    *OS << "DEBUG:: starting ASaP Semantic Checker\n";
    ASaPSemanticCheckerTraverser SemanticChecker;
    SemanticChecker.TraverseDecl(TUDecl);
    *OS << "##############################################\n";
    *OS << "DEBUG:: done running ASaP Semantic Checker\n\n";
    if (SemanticChecker.encounteredFatalError()) {
      *OS << "DEBUG:: SEMANTIC CHECKER ENCOUNTERED FATAL ERROR!! STOPPING\n";
      return;
    }

    *OS << "DEBUG:: starting ASaP Effect Coverage Checker\n";
    EffectSummaryNormalizerTraverser EffectNormalizerChecker;
    EffectNormalizerChecker.TraverseDecl(TUDecl);
    *OS << "##############################################\n";
    *OS << "DEBUG:: done running ASaP Effect Normalizer Checker\n\n";
    if (EffectNormalizerChecker.encounteredFatalError()) {
      *OS << "DEBUG:: EFFECT NORMALIZER CHECKER ENCOUNTERED FATAL ERROR!! STOPPING\n";
      return;
    }

    *OS << "DEBUG:: starting ASaP Type Checker\n";
    StmtVisitorInvoker<AssignmentCheckerVisitor> TypeChecker;
    TypeChecker.TraverseDecl(TUDecl);
    *OS << "##############################################\n";
    *OS << "DEBUG:: done running ASaP Type Checker\n\n";
    if (TypeChecker.encounteredFatalError()) {
      *OS << "DEBUG:: Type Checker ENCOUNTERED FATAL ERROR!! STOPPING\n";
      return;
    }
    // Check that Effect Summaries cover effects
    *OS << "DEBUG:: starting ASaP Effect Constraint Generator\n";
    //StmtVisitorInvoker<EffectCollectorVisitor> EffectChecker;
    StmtVisitorInvoker<EffectConstraintVisitor> EffectChecker;
    EffectChecker.TraverseDecl(TUDecl);
    *OS << "##############################################\n";
    *OS << "DEBUG:: done running ASaP Effect Constraint Generator\n\n";
    if (EffectChecker.encounteredFatalError()) {
      *OS << "DEBUG:: Effect Checker ENCOUNTERED FATAL ERROR!! STOPPING\n";
      return;
    }

    *OS << "DEBUG:: starting ASaP Non-Interference Checking\n";
    StmtVisitorInvoker<NonInterferenceChecker> NonIChecker;
    NonIChecker.TraverseDecl(TUDecl);
    *OS << "##############################################\n";
    *OS << "DEBUG:: done running ASaP Non-Interference Checking\n\n";
    if (NonIChecker.encounteredFatalError()) {
      *OS << "DEBUG:: NON-INTERFERENCE CHECKING ENCOUNTERED FATAL ERROR!! STOPPING\n";
      return;
    }
    if (DoEffectInference || DoFullInference) {
      setupProlog();
      SymbolTable::Table->solveConstraints();
    }
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
