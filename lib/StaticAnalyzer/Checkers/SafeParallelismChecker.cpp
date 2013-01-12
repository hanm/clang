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

  /**
   * Return the string name of the region or region parameter declaration
   * based on the Kind of the Attribute (RegionAttr or RegionParamAttr)
   */
  // FIXME
  inline StringRef getRegionOrParamName(const Attr *Attribute) {
    StringRef Result = "";
    switch(Attribute->getKind()) {
    case attr::Region:
      Result = dyn_cast<RegionAttr>(Attribute)->getName(); break;
    case attr::RegionParam:
      Result = dyn_cast<RegionParamAttr>(Attribute)->getName(); break;
    default:
      Result = "";
    }

    return Result;
  }

  inline RplElement *createRegionOrParamElement(const Attr *Attribute) {
    RplElement* Result = 0;
    switch(Attribute->getKind()) {
    case attr::Region:
      Result = new NamedRplElement(dyn_cast<RegionAttr>(Attribute)->getName());
      break;
    case attr::RegionParam:
      Result = new ParamRplElement(dyn_cast<RegionParamAttr>(Attribute)->
                                   getName());
      break;
    default:
      Result = 0;
    }
    return Result;
  }

  /**
   *  Return true if any of the attributes of type AttrType of the
   *  declaration Decl* D have a region name or a param name that is
   *  the same as the 'name' provided as the second argument
   */
  template<typename AttrType>
  bool scanAttributesForRegionDecl(Decl* D, const StringRef &Name)
  {
    for (specific_attr_iterator<AttrType>
         I = D->specific_attr_begin<AttrType>(),
         E = D->specific_attr_end<AttrType>();
         I != E; ++I) {
      if (getRegionOrParamName(*I) == Name)
        return true;
    }

    return false;
  }

  /// \brief  Return an RplElement if any of the attributes of type AttrType
  /// of the declaration Decl* D have a region name or a param name that is
  /// the same as the 'name' provided as the 2nd argument.
  template<typename AttrType>
  RplElement* scanAttributes(Decl* D, const StringRef &Name)
  {
    for (specific_attr_iterator<AttrType>
         I = D->specific_attr_begin<AttrType>(),
         E = D->specific_attr_end<AttrType>();
         I != E; ++ I) {
      if (getRegionOrParamName(*I) == Name) {
        return createRegionOrParamElement(*I);
      }
    }

    return 0;
  }


                                    
typedef std::map<const FunctionDecl*, Effect::EffectVector*> EffectSummaryMapTy;
typedef std::map<const Attr*, Rpl*> RplAttrMapTy;

void destroyEffectSummaryMap(EffectSummaryMapTy &EffectSummaryMap) {
  for(EffectSummaryMapTy::iterator I = EffectSummaryMap.begin(),
                                 E = EffectSummaryMap.end();
      I != E; ++I) {
    Effect::EffectVector *EV = (*I).second;
    ASaP::destroyVector(*EV);
    EffectSummaryMap.erase(I);
  }
  assert(EffectSummaryMap.size()==0);
}

void destroyRplAttrMap(RplAttrMapTy &RplAttrMap) {
  for(RplAttrMapTy::iterator I = RplAttrMap.begin(),
                             E = RplAttrMap.end();
      I != E; ++I) {
    Rpl *R = (*I).second;
    R->destroyElements();
    RplAttrMap.erase(I);
  }
  assert(RplAttrMap.size()==0);
}


/// FIXME temporarily just using pre-processor to concatenate code here... UGLY 
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
    ASaPSemanticCheckerTraverser SemanticChecker(BR, D->getASTContext(),
                                                 Mgr.getAnalysisDeclContext(D),
                                                 os, EffectsMap, RplMap);
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
    destroyRplAttrMap(RplMap);
    destroyEffectSummaryMap(EffectsMap);
  }
};
} // end unnamed namespace

void ento::registerSafeParallelismChecker(CheckerManager &mgr) {
  mgr.registerChecker<SafeParallelismChecker>();
}
