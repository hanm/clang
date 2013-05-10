//=== ASaPSymbolTable.cpp - Safe Parallelism checker -----*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This file defines the SymbolTable and SymbolTableEntry classes used by
// the Safe Parallelism checker, which tries to prove the safety of
// parallelism given region and effect annotations.
//
//===----------------------------------------------------------------===//

/// Maps AST Decl* nodes to ASaP info that appertains to the node
/// such information includes ASaPType*, ParamVector, RegionNameSet,
/// and EffectSummary
#include "clang/AST/Decl.h"

#include "ASaPUtil.h"
#include "ASaPSymbolTable.h"
#include "ASaPType.h"
#include "Effect.h"
#include "Rpl.h"
#include "Substitution.h"

namespace clang {
namespace asap {

StringRef stringOf(ResultKind R) {
  switch(R) {
  case RK_OK: return "OK";
  case RK_ERROR: return "ERROR";
  case RK_NOT_VISITED: return "NOT_VISITED";
  case RK_VAR: return "VAR";
  }
  return "UNKNOWN!";
}

/// Static Constants
int SymbolTable::Initialized = 0;

const StarRplElement *SymbolTable::STAR_RplElmt = 0;
const SpecialRplElement *SymbolTable::ROOT_RplElmt = 0;
const SpecialRplElement *SymbolTable::LOCAL_RplElmt = 0;
const SpecialRplElement *SymbolTable::GLOBAL_RplElmt = 0;
const SpecialRplElement *SymbolTable::IMMUTABLE_RplElmt = 0;
const Effect *SymbolTable::WritesLocal = 0;

/// Static Functions
void SymbolTable::Initialize() {
  if (!Initialized) {
    STAR_RplElmt = new StarRplElement();
    ROOT_RplElmt = new SpecialRplElement("Root");
    LOCAL_RplElmt = new SpecialRplElement("Local");
    GLOBAL_RplElmt = new SpecialRplElement("Global");
    IMMUTABLE_RplElmt = new SpecialRplElement("Immutable");
    Rpl R(*LOCAL_RplElmt);
    R.appendElement(STAR_RplElmt);
    WritesLocal = new Effect(Effect::EK_WritesEffect, &R);
    Initialized = 1;
  }
}

void SymbolTable::Destroy() {
  if (Initialized) {
    Initialized = 0;

    delete STAR_RplElmt;
    delete ROOT_RplElmt;
    delete LOCAL_RplElmt;
    delete GLOBAL_RplElmt;
    delete IMMUTABLE_RplElmt;
    delete WritesLocal;

    STAR_RplElmt = 0;
    ROOT_RplElmt = 0;
    LOCAL_RplElmt = 0;
    GLOBAL_RplElmt = 0;
    IMMUTABLE_RplElmt = 0;
    WritesLocal = 0;
  }
}

const RplElement *SymbolTable::
getSpecialRplElement(const llvm::StringRef& Str) {
  if (!Str.compare(SymbolTable::STAR_RplElmt->getName()))
    return SymbolTable::STAR_RplElmt;
  else if (!Str.compare(SymbolTable::ROOT_RplElmt->getName()))
    return SymbolTable::ROOT_RplElmt;
  else if (!Str.compare(SymbolTable::LOCAL_RplElmt->getName()))
    return SymbolTable::LOCAL_RplElmt;
  else if (!Str.compare(SymbolTable::GLOBAL_RplElmt->getName()))
    return SymbolTable::GLOBAL_RplElmt;
  else if (!Str.compare(SymbolTable::IMMUTABLE_RplElmt->getName()))
    return SymbolTable::IMMUTABLE_RplElmt;
  else
    return 0;
}

bool SymbolTable::
isSpecialRplElement(const llvm::StringRef& Str) {
  if (!Str.compare("*")
      || !Str.compare("Local")
      || !Str.compare("Global")
      || !Str.compare("Immutable")
      || !Str.compare("Root"))
    return true;
  else
    return false;
}


/// Non-Static Functions
SymbolTable::SymbolTable() {
  // FIXME: make this static like the other default Regions etc
  ParamRplElement Param("P");
  BuiltinDefaultRegionParameterVec = new ParameterVector(Param);
  //BuiltinDefaultRegionParameterVec->push_back(Param);
}

SymbolTable::~SymbolTable() {
  delete BuiltinDefaultRegionParameterVec;

  for(SymbolTableMapT::iterator I = SymTable.begin(), E = SymTable.end();
    I != E; ++I) {
      // for this key, delete the value
      delete (*I).second;
  }
}

ResultTriplet SymbolTable::getRegionParamCount(QualType QT) {
  if (isNonPointerScalarType(QT)) {
    OSv2 << "DEBUG:: getRegionParamCount::isNonPointerScalarType\n";
    return ResultTriplet(RK_OK, 1, 0);
  } else if (QT->isPointerType()) {
    OSv2 << "DEBUG:: getRegionParamCount::isPointerType\n";
    ResultTriplet Result = getRegionParamCount(QT->getPointeeType());
    Result.NumArgs += 1;
    return Result;
  } else if (QT->isReferenceType()) {
    OSv2 << "DEBUG:: getRegionParamCount::isReferenceType\n";
    return getRegionParamCount(QT->getPointeeType());
  } else if (QT->isStructureOrClassType()) {
    OSv2 << "DEBUG:: getRegionParamCount::isStructureOrClassType\n";
    const RecordType *RT = QT->getAs<RecordType>();
    assert(RT);
    RecordDecl *D = RT->getDecl();
    const ParameterVector *ParamV = getParameterVector(D);
    if (ParamV)
      return ResultTriplet(RK_OK, ParamV->size(), D);
    else
      return ResultTriplet(RK_NOT_VISITED, 0, D);
  } else if (QT->isFunctionType()) {
    OSv2 << "DEBUG:: getRegionParamCount::isFunctionType\n";
    const FunctionType *FT = QT->getAs<FunctionType>();
    assert(FT);

    QualType ResultQT = FT->getResultType();
    return getRegionParamCount(ResultQT);
  } else if (QT->isVoidType()) {
    OSv2 << "DEBUG:: getRegionParamCount::isVoidType\n";
    return ResultTriplet(RK_OK, 0, 0);
  } else if (QT->isTemplateTypeParmType()) {
    OSv2 << "DEBUG:: getRegionParamCount::isTemplateParmType\n";
    return ResultTriplet(RK_VAR, 0, 0);
  } else {
    OSv2 << "DEBUG:: getRegionParamCount::UnexpectedType\n";
    // This should not happen: unknown number of region arguments for type
    return ResultTriplet(RK_ERROR, 0, 0);
  }
}

bool SymbolTable::hasDecl(const Decl* D) const {
  if (!SymTable.lookup(D))
    return false;
  else
    return true;
}

bool SymbolTable::hasType(const Decl* D) const {
  if (!SymTable.lookup(D))
    return false;
  return SymTable.lookup(D)->hasType();
}

bool SymbolTable::hasParameterVector(const Decl* D) const {
  if (!SymTable.lookup(D))
    return false;
  return SymTable.lookup(D)->hasParameterVector();
}

bool SymbolTable::hasRegionNameSet(const Decl* D) const {
  if (!SymTable.lookup(D))
    return false;
  return SymTable.lookup(D)->hasParameterVector();
}

bool SymbolTable::hasEffectSummary(const Decl* D) const {
  if (!SymTable.lookup(D))
    return false;
  return SymTable.lookup(D)->hasEffectSummary();
}

const ASaPType *SymbolTable::getType(const Decl* D) const {
  if (!SymTable.lookup(D))
    return 0;
  else
    return SymTable.lookup(D)->getType();
}

const ParameterVector *SymbolTable::getParameterVector(const Decl *D) const {
  if (!SymTable.lookup(D))
    return 0;
  else
    return SymTable.lookup(D)->getParameterVector();
}

const SubstitutionVector *SymbolTable::
getInheritanceSubVec(const Decl *D) const {
  if (!SymTable.lookup(D))
    return 0;
  else
    return SymTable.lookup(D)->getInheritanceSubVec();
}

const RegionNameSet *SymbolTable::getRegionNameSet(const Decl *D) const {
  if (!SymTable.lookup(D))
    return 0;
  else
    return SymTable.lookup(D)->getRegionNameSet();
}

const EffectSummary *SymbolTable::getEffectSummary(const Decl *D) const {
  if (!SymTable.lookup(D))
    return 0;
  else
    return SymTable.lookup(D)->getEffectSummary();
}

bool SymbolTable::setType(const Decl* D, ASaPType *T) {
  if (!SymTable[D])
    SymTable[D] = new SymbolTableEntry();
  // invariant: SymTable[D] not null
  if (SymTable[D]->hasType())
    return false;
  else {
    SymTable[D]->setType(T);
    return true;
  }
}

bool SymbolTable::initParameterVector(const Decl *D) {
  if (!SymTable[D])
    SymTable[D] = new SymbolTableEntry();
  // invariant: SymTable[D] not null
  if (SymTable[D]->hasParameterVector())
    return false;
  else {
    SymTable[D]->setParameterVector(new ParameterVector());
    return true;
  }
}

bool SymbolTable::setParameterVector(const Decl *D, ParameterVector *PV) {
  if (!SymTable[D])
    SymTable[D] = new SymbolTableEntry();
  // invariant: SymTable[D] not null
  if (SymTable[D]->hasParameterVector())
    return false;
  else {
    SymTable[D]->setParameterVector(PV);
    return true;
  }
}

bool SymbolTable::setRegionNameSet(const Decl *D, RegionNameSet *RNS) {
  if (!SymTable[D])
    SymTable[D] = new SymbolTableEntry();
  // invariant: SymTable[D] not null
  if (SymTable[D]->hasRegionNameSet())
    return false;
  else {
    SymTable[D]->setRegionNameSet(RNS);
    return true;
  }
}

bool SymbolTable::setEffectSummary(const Decl *D, EffectSummary *ES) {
  if (!SymTable[D])
    SymTable[D] = new SymbolTableEntry();
  // invariant: SymTable[D] not null
  if (SymTable[D]->hasEffectSummary())
    return false;
  else {
    SymTable[D]->setEffectSummary(ES);
    return true;
  }
}

bool SymbolTable::setEffectSummary(const Decl *D, const Decl *Dfrom) {
  if (!SymTable[Dfrom] || !SymTable[Dfrom]->hasEffectSummary())
    return false;

  if (!SymTable[D])
    SymTable[D] = new SymbolTableEntry();
  // invariant: SymTable[D] not null
  if (SymTable[D]->hasEffectSummary())
    return false;
  else {
    const EffectSummary *From = SymTable[Dfrom]->getEffectSummary();
    SymTable[D]->setEffectSummary(new EffectSummary(*From));
    return true;
  }
}

const NamedRplElement *SymbolTable::
lookupRegionName(const Decl* D, StringRef Name) const {
  if (!SymTable.lookup(D))
    return 0;
  return SymTable.lookup(D)->lookupRegionName(Name);
}

const ParamRplElement *SymbolTable::
lookupParameterName(const Decl *D, StringRef Name) const {
  if (!SymTable.lookup(D))
    return 0;
  return SymTable.lookup(D)->lookupParameterName(Name);
}

bool SymbolTable::
hasRegionName(const Decl *D, StringRef Name) const {
  return lookupRegionName(D, Name) ? true : false;
}

bool SymbolTable::
hasParameterName(const Decl *D, StringRef Name) const {
  return lookupParameterName(D, Name) ? true : false;
}

bool SymbolTable::
hasRegionOrParameterName(const Decl *D, StringRef Name) const {
  return hasRegionName(D, Name) || hasParameterName(D, Name);
}

bool SymbolTable::hasBase(const Decl *D, const Decl *Base) const {
  if (!SymTable.lookup(D) || !SymTable.lookup(Base))
    return false;
  return (SymTable.lookup(D)->
            getSubVec(SymTable.lookup(Base)) == 0)? false : true;
}

bool SymbolTable::addRegionName(const Decl *D, StringRef Name) {
  if (hasRegionOrParameterName(D, Name))
    return false;
  if (!SymTable.lookup(D))
    SymTable[D] = new SymbolTableEntry();
  SymTable[D]->addRegionName(Name);
  return true;
}

bool SymbolTable::addParameterName(const Decl *D, StringRef Name) {
  if (hasRegionOrParameterName(D, Name))
    return false;
  if (!SymTable.lookup(D))
    SymTable[D] = new SymbolTableEntry(); // FIXME Check all calls to new
  SymTable[D]->addParameterName(Name);
  return true;
}

bool SymbolTable::addBaseTypeAndSub(const Decl *D, const Decl *Base,
                                    SubstitutionVector *&SubV) {
  if (!SubV)
    return true; // Nothing to do here
  if (hasBase(D, Base)) {
    return false; // FIXME: should we instead merge SubVs?...
  }
  if (!SymTable.lookup(D))
    SymTable[D] = new SymbolTableEntry();

  if (!SymTable.lookup(Base))
    SymTable[Base] = new SymbolTableEntry();

  SymTable[D]->addBaseTypeAndSub(SymTable.lookup(Base), SubV);
  return true;
}


const ParameterVector *SymbolTable::getParameterVectorFromQualType(QualType QT) {
  const ParameterVector *ParamVec = 0;
  if (QT->isReferenceType()) {
    ParamVec = getParameterVectorFromQualType(QT->getPointeeType());
  } else if (const TagType* TT = dyn_cast<TagType>(QT.getTypePtr())) {
    const TagDecl* TD = TT->getDecl();
    //TD->dump(OSv2);
    ParamVec = this->getParameterVector(TD);
  } else if (QT->isBuiltinType() || QT->isPointerType()) {
    // TODO check the number of parameters of the arg attr to be 1
    ParamVec = BuiltinDefaultRegionParameterVec;
  } /// else result = NULL;
  return ParamVec;
}

const SubstitutionVector *SymbolTable::
getInheritanceSubVec(QualType QT) {
  const SubstitutionVector *SubV = 0;
  if (QT->isReferenceType()) {
    SubV = getInheritanceSubVec(QT->getPointeeType());
  } else if (const TagType* TT = dyn_cast<TagType>(QT.getTypePtr())) {
    const TagDecl* TD = TT->getDecl();
    //TD->dump(OSv2);
    SubV = this->getInheritanceSubVec(TD);
  } else if (QT->isBuiltinType() || QT->isPointerType()) {
    SubV = 0;
  } /// else result = NULL;
  return SubV;
}

//////////////////////////////////////////////////////////////////////////
// SymbolTableEntry

SymbolTable::SymbolTableEntry::SymbolTableEntry() :
    Typ(0), ParamVec(0), RegnNameSet(0), EffSum(0), InheritanceMap(0),
    ComputedInheritanceSubVec(false), InheritanceSubVec(0) {}

SymbolTable::SymbolTableEntry::~SymbolTableEntry() {
  // FIXME uncommenting these lines causes some tests to fail
  delete Typ;
  delete ParamVec;
  delete RegnNameSet;
  delete EffSum;
  if (InheritanceMap && InheritanceMap->size() > 0) {
    for(InheritanceMapT::iterator
          I = InheritanceMap->begin(), E = InheritanceMap->end();
        I != E; ++I) {
      delete (*I).second;
    }
    delete InheritanceMap;
  }
  delete InheritanceSubVec;
}

const NamedRplElement *SymbolTable::SymbolTableEntry::lookupRegionName(StringRef Name) {
  if (!RegnNameSet)
    return 0;
  return RegnNameSet->lookup(Name);
}

const ParamRplElement *SymbolTable::SymbolTableEntry::lookupParameterName(StringRef Name) {
  if (!ParamVec)
    return 0;
  return ParamVec->lookup(Name);
}

void SymbolTable::SymbolTableEntry::addRegionName(StringRef Name) {
  if (!RegnNameSet)
    RegnNameSet = new RegionNameSet();
  RegnNameSet->insert(new NamedRplElement(Name));
}

void SymbolTable::SymbolTableEntry::addParameterName(StringRef Name) {
  if (!ParamVec)
    ParamVec = new ParameterVector();
  ParamVec->push_back(new ParamRplElement(Name));
}

bool SymbolTable::SymbolTableEntry::
addBaseTypeAndSub(SymbolTableEntry *Base, SubstitutionVector *&SubV) {
  // If we're not adding a substitution then skip it altogether.
  if (!SubV)
    return true;
  // Allocate map if it doesn't exist.
  if (!InheritanceMap)
    InheritanceMap = new InheritanceMapT();
  // Add to map.
  (*InheritanceMap)[Base] = SubV;
  SubV = 0;
  return true;
}

const SubstitutionVector *SymbolTable::SymbolTableEntry::
getSubVec(SymbolTableEntry *Base) const {
  if (!InheritanceMap)
    return 0;
  return (*InheritanceMap)[Base];
}

void SymbolTable::SymbolTableEntry::
computeInheritanceSubVec() {

  if (!ComputedInheritanceSubVec
      && InheritanceMap && InheritanceMap->size() > 0) {

    assert(!InheritanceSubVec);
    InheritanceSubVec = new SubstitutionVector();
    for(InheritanceMapT::iterator
          I = InheritanceMap->begin(), E = InheritanceMap->end();
        I != E; ++I) {
      SymbolTableEntry *STE = (*I).first;
      InheritanceSubVec->push_back_vec(STE->getInheritanceSubVec());
      const SubstitutionVector *SubV = (*I).second;
      InheritanceSubVec->push_back_vec(SubV);
      // Note: this order is important (i.e., first push_back the
      // base class inheritance substitutions (recursively) then
      // the substitution from for this base class to the next.
      // FIXME: for performance, we should collapse chains of
      // substitutions by applying one to the next. E.g.,
      // [P1<-P2][P2<-P3][P3<-P4] => [P1<-P4][P2<-P4][P3<-P4];
    }
  }
  ComputedInheritanceSubVec = true;
}

const SubstitutionVector *SymbolTable::SymbolTableEntry::
getInheritanceSubVec() {
  if (!InheritanceMap)
    return 0;
  if (!ComputedInheritanceSubVec)
    computeInheritanceSubVec();
  return InheritanceSubVec;
}


} // end namespace asap
} // end namespace clang

