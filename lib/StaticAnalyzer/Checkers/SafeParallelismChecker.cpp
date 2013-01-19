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
static raw_ostream& osv2 = llvm::errs();
#else
static raw_ostream& osv2 = llvm::nulls();
#endif

/// FIXME temporarily just using pre-processor to concatenate code here... UGLY
#include "asap/RplsAndEffects.cpp"

namespace {
  /// \brief the default region parameter "P" used for implicitly boxed types
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


typedef std::map<const FunctionDecl*, Effect::EffectVector*> EffectSummaryMapTy;
/// FIXME it might be better to map declarations to vectors of Rpls
/// and RplElements as we did for effect summaries...
typedef std::map<const Attr*, Rpl*> RplAttrMapTy;
typedef std::map<const Attr*, RplElement*> RplElementAttrMapTy;

namespace ASaP {
  template<typename T>
  void destroyVector(T &V) {
    for (typename T::const_iterator
             I = V.begin(),
             E = V.end();
         I != E; ++I) {
      delete(*I);
    }
  }

  template<typename T>
  void destroyVectorVector(T &V) {
    for (typename T::const_iterator
             I = V.begin(),
             E = V.end();
         I != E; ++I) {
      destroyVector(*(*I));
      delete(*I);
    }
  }

} /// end namespace ASaP

void destroyEffectSummaryMap(EffectSummaryMapTy &EffectSummaryMap) {
  for(EffectSummaryMapTy::iterator I = EffectSummaryMap.begin(),
                                 E = EffectSummaryMap.end();
      I != E; ++I) {
    Effect::EffectVector *EV = (*I).second;
    ASaP::destroyVector(*EV);
    delete EV;
    EffectSummaryMap.erase(I);
  }
  assert(EffectSummaryMap.size()==0);
}

void destroyRplAttrMap(RplAttrMapTy &RplAttrMap) {
  for(RplAttrMapTy::iterator I = RplAttrMap.begin(),
                             E = RplAttrMap.end();
      I != E; ++I) {
    Rpl *R = (*I).second;
    delete R;
    RplAttrMap.erase(I);
  }
  assert(RplAttrMap.size()==0);
}

void destroyRplElementAttrMap(RplElementAttrMapTy &RplElementAttrMap) {
  for(RplElementAttrMapTy::iterator I = RplElementAttrMap.begin(),
                             E = RplElementAttrMap.end();
      I != E; ++I) {
    RplElement *RpEl = (*I).second;
    delete RpEl;
    RplElementAttrMap.erase(I);
  }
  assert(RplElementAttrMap.size()==0);
}


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
    EffectSummaryMapTy EffectsMap;
    RplAttrMapTy RplMap;
    RplElementAttrMapTy RplElementMap;
    ASaPSemanticCheckerTraverser SemanticChecker(BR, D->getASTContext(),
                                                 Mgr.getAnalysisDeclContext(D),
                                                 os, EffectsMap, RplMap,
                                                 RplElementMap);
    /** run checker */
    SemanticChecker.TraverseDecl(const_cast<TranslationUnitDecl*>(D));
    os << "##############################################\n";
    os << "DEBUG:: done running ASaP Semantic Checker\n\n";
    if (SemanticChecker.encounteredFatalError())
      os << "DEBUG:: ENCOUNTERED FATAL ERROR!! STOPPING\n";

    if (!SemanticChecker.encounteredFatalError()) {
      /// Check that Effect Summaries cover effects
      ASaPEffectsCheckerTraverser EffectChecker(BR, D->getASTContext(), Mgr,
                                                Mgr.getAnalysisDeclContext(D),
                                                os, EffectsMap, RplMap);
      EffectChecker.TraverseDecl(const_cast<TranslationUnitDecl*>(D));
    }
    /// TODO: destroy EffectMap & RplMap
    /// Clean-Up
    destroyEffectSummaryMap(EffectsMap);
    destroyRplAttrMap(RplMap); // FIXME: tries to free freed memory (sometimes)
    destroyRplElementAttrMap(RplElementMap);
    /// TODO: deallocate BuiltinDefaulrRegionParam
    delete ROOT_RplElmt;
    delete LOCAL_RplElmt;
    delete STAR_RplElmt;

  }
};
} // end unnamed namespace

void ento::registerSafeParallelismChecker(CheckerManager &mgr) {
  mgr.registerChecker<SafeParallelismChecker>();
}
