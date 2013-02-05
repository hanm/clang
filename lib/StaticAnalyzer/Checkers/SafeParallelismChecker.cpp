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

// The following definition selects code that uses the
// specific_attr_reverse_iterator functionality. This option is kept around
// because I suspect *not* using it may actually perform better... The
// alternative being to build an llvm::SmallVector using the fwd iterator,
// then traversing it through the reverse iterator. The reason this might be
// preferable is because after building it, the SmallVector is 'reverse
// iterated' multiple times.
#define ATTR_REVERSE_ITERATOR_SUPPORTED

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

namespace ASaP {
  template<typename T>
  void destroyVector(T &V) {
    for (typename T::const_iterator I = V.begin(), E = V.end(); I != E; ++I)
      delete(*I);
  }

  template<typename T>
  void destroyVectorVector(T &V) {
    for (typename T::const_iterator I = V.begin(), E = V.end(); I != E; ++I) {
      destroyVector(*(*I));
      delete(*I);
    }
  }

} /// End namespace ASaP

inline bool isNonPointerScalarType(QualType QT) {
  return (QT->isScalarType() && !QT->isPointerType());
}
#include "asap/RplsAndEffects.cpp"
#include "asap/ASaPType.cpp"

namespace {
  /// \brief The default region parameter "P" used for implicitly boxed types
  /// such as int or pointers.
  RegionParamAttr *BuiltinDefaulrRegionParam;


  // TODO pass arg attr as parameter
  /*
  inline bool hasValidRegionParamAttr(const Type* T) {
    bool result = false;
    if (const TagType* tt = dyn_cast<TagType>(T)) {
      const TagDecl* td = tt->getDecl();
      const RegionParamAttr* rpa = td->getAttr<RegionParamAttr>();
      // TODO check the number of params is equal to arg attr.
      if (rpa) result = true;
    } else if (T->isBuiltinType() || T->isPointerType()) {
      // TODO check the number of parameters of the arg attr to be 1
      result = true;
    }
    return result;
  }*/

  // FIXME
  inline const RegionParamAttr *getRegionParamAttr(const Type* T) {
    const RegionParamAttr* Result = 0; // null
    if (const TagType* TT = dyn_cast<TagType>(T)) {
      const TagDecl* TD = TT->getDecl();
      Result = TD->getAttr<RegionParamAttr>();
      // TODO check the number of params is equal to arg attr.
    } else if (T->isBuiltinType() || T->isPointerType()) {
      // TODO check the number of parameters of the arg attr to be 1
      Result = BuiltinDefaulrRegionParam;
    } /// else result = NULL;

    return Result;
  }

typedef std::map<const Attr*, RplElement*> RplElementAttrMapTy;
/// FIXME it might be better to map declarations to vectors of Rpls
/// and RplElements as we did for effect summaries...
typedef std::map<const Attr*, Rpl*> RplAttrMapTy;
typedef std::map<const Decl*, ASaPType*> ASaPTypeDeclMapTy; /// TODO: populate
typedef std::map<const FunctionDecl*, Effect::EffectVector*> EffectSummaryMapTy;


void destroyEffectSummaryMap(EffectSummaryMapTy &EffectSummaryMap) {
  for(EffectSummaryMapTy::iterator I = EffectSummaryMap.begin(),
      E = EffectSummaryMap.end(); I != E;) {
    Effect::EffectVector *EV = (*I).second;
    ASaP::destroyVector(*EV);
    delete EV;
    EffectSummaryMap.erase(I++);
  }
  assert(EffectSummaryMap.size()==0);
}

void destroyRplAttrMap(RplAttrMapTy &RplAttrMap) {
  for(RplAttrMapTy::iterator I = RplAttrMap.begin(),
      E = RplAttrMap.end(); I != E;) {
    Rpl *R = (*I).second;
    delete R;
    RplAttrMap.erase(I++);
  }
  assert(RplAttrMap.size()==0);
}

void destroyRplElementAttrMap(RplElementAttrMapTy &RplElementAttrMap) {
  for(RplElementAttrMapTy::iterator I = RplElementAttrMap.begin(),
      E = RplElementAttrMap.end(); I != E;) {
    RplElement *RpEl = (*I).second;
    delete RpEl;
    RplElementAttrMap.erase(I++);
  }
  assert(RplElementAttrMap.size()==0);
}

///-///////////////////////////////////////////////////////////////////
/// GENERIC VISITORS
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

  RplElementAttrMapTy RplElementMap;
  RplAttrMapTy &RplAttrMap;
  ASaPTypeDeclMapTy &ASaPTypeDeclMap;
  EffectSummaryMapTy &EffectSummaryMap;

  bool FatalError;

public:

  typedef RecursiveASTVisitor<StmtVisitorInvoker> BaseClass;

  /// Constructor
  explicit StmtVisitorInvoker(
    ento::BugReporter &BR, ASTContext &Ctx,
    AnalysisManager &Mgr, AnalysisDeclContext *AC, raw_ostream &OS,
    RplElementAttrMapTy RplElementMap, RplAttrMapTy &RplAttrMap,
    ASaPTypeDeclMapTy &ASaPTypeDeclMap, EffectSummaryMapTy &ESM)
      : BR(BR),
        Ctx(Ctx),
        Mgr(Mgr),
        AC(AC),
        OS(OS),
        RplElementMap(RplElementMap),
        RplAttrMap(RplAttrMap),
        ASaPTypeDeclMap(ASaPTypeDeclMap),
        EffectSummaryMap(ESM),
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

      StmtVisitorTy StmtVisitor(BR, Ctx, Mgr, AC, OS,
                                RplElementMap, RplAttrMap, ASaPTypeDeclMap,
                                EffectSummaryMap, Definition, S);
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

  RplElementAttrMapTy &RplElementMap;
  RplAttrMapTy &RplMap;
  ASaPTypeDeclMapTy &ASaPTypeDeclMap;
  EffectSummaryMapTy &EffectSummaryMap;

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
    RplElementAttrMapTy &RplElementMap,
    RplAttrMapTy &RplMap,
    ASaPTypeDeclMapTy &ASaPTypeDeclMap,
    EffectSummaryMapTy &EffectSummaryMap,
    const FunctionDecl *Def,
    Stmt *S
    ) : BR(BR),
        Ctx(Ctx),
        Mgr(Mgr),
        AC(AC),
        OS(OS),
        RplElementMap(RplElementMap),
        RplMap(RplMap),
        ASaPTypeDeclMap(ASaPTypeDeclMap),
        EffectSummaryMap(EffectSummaryMap),
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
#include "asap/TypeChecker.cpp"
#include "asap/EffectChecker.cpp"
#include "asap/SemanticChecker.cpp"

class  SafeParallelismChecker
  : public Checker<check::ASTDecl<TranslationUnitDecl> > {

public:
  void checkASTDecl(const TranslationUnitDecl *D, AnalysisManager &Mgr,
                    BugReporter &BR) const {
    os << "DEBUG:: starting ASaP Semantic Checker\n";
    BuiltinDefaulrRegionParam = ::new(D->getASTContext())
      RegionParamAttr(D->getSourceRange(), D->getASTContext(), "P");

    /** initialize traverser */
    RplElementAttrMapTy RplElementMap;
    RplAttrMapTy RplMap;
    ASaPTypeDeclMapTy ASaPTypeMap;
    EffectSummaryMapTy EffectsMap;
    ASaPSemanticCheckerTraverser
      SemanticChecker(BR, D->getASTContext(),
                      Mgr.getAnalysisDeclContext(D),
                      os, RplElementMap, RplMap, ASaPTypeMap, EffectsMap);
    /** run checker */
    SemanticChecker.TraverseDecl(const_cast<TranslationUnitDecl*>(D));
    os << "##############################################\n";
    os << "DEBUG:: done running ASaP Semantic Checker\n\n";
    if (SemanticChecker.encounteredFatalError()) {
      os << "DEBUG:: SEMANTIC CHECKER ENCOUNTERED FATAL ERROR!! STOPPING\n";
    } else {
      // else continue with Typechecking
      StmtVisitorInvoker<AssignmentCheckerVisitor>
        TypeChecker(BR, D->getASTContext(), Mgr,
                    Mgr.getAnalysisDeclContext(D),
                    os, RplElementMap, RplMap, ASaPTypeMap, EffectsMap);
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
          EffectChecker(BR, D->getASTContext(), Mgr,
                        Mgr.getAnalysisDeclContext(D),
                        os, RplElementMap, RplMap, ASaPTypeMap, EffectsMap);

        EffectChecker.TraverseDecl(const_cast<TranslationUnitDecl*>(D));
        os << "##############################################\n";
        os << "DEBUG:: done running ASaP Effect Checker\n\n";
        if (EffectChecker.encounteredFatalError()) {
          os << "DEBUG:: Effect Checker ENCOUNTERED FATAL ERROR!! STOPPING\n";
        }
      }
    }

    /// Clean-Up
    destroyEffectSummaryMap(EffectsMap);
    destroyRplAttrMap(RplMap); // FIXME: tries to free freed memory (sometimes)
    destroyRplElementAttrMap(RplElementMap);
    /// TODO: deallocate BuiltinDefaulrRegionParam
    delete ROOTRplElmt;
    delete LOCALRplElmt;
    delete STARRplElmt;
  }
}; // end class SafeParallelismChecker
} // end unnamed namespace

void ento::registerSafeParallelismChecker(CheckerManager &mgr) {
  mgr.registerChecker<SafeParallelismChecker>();
}
