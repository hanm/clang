//=== ASaPSymbolTable.cpp - Safe Parallelism checker -----*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This files defines the SymbolTable and SymbolTableEntry classes used by
// the Safe Parallelism checker, which tries to prove the safety of
// parallelism given region and effect annotations.
//
//===----------------------------------------------------------------===//

/// Maps AST Decl* nodes to ASaP info that appertains to the node
/// such information includes ASaPType*, ParamVector, RegionNameSet,
/// and EffectSummary

#include "ASaPSymbolTable.h"
#include "Rpl.h"
#include "Effect.h"
#include "clang/AST/Decl.h"

using namespace clang;
using namespace clang::asap;

/// Static Constants
const StarRplElement *SymbolTable::STAR_RplElmt =
    new StarRplElement();
const SpecialRplElement *SymbolTable::ROOT_RplElmt =
    new SpecialRplElement("Root");
const SpecialRplElement *SymbolTable::LOCAL_RplElmt =
    new SpecialRplElement("Local");
const Effect *SymbolTable::WritesLocal =
    new Effect(Effect::EK_WritesEffect,new Rpl(*LOCAL_RplElmt));


SymbolTable::SymbolTable() {
  BuiltinDefaultRegionParameterVec =
    new ParameterVector(new ParamRplElement("P"));
}

SymbolTable::~SymbolTable() {
  delete BuiltinDefaultRegionParameterVec;

  for(SymbolTableMapT::iterator I = SymTable.begin(), E = SymTable.end();
    I != E; ++I) {
      // for this key, delete the value
      delete (*I).second;
  }
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

const NamedRplElement *SymbolTable::lookupRegionName(const Decl* D, StringRef Name) {
  if (!SymTable.lookup(D))
    return 0;
  return SymTable.lookup(D)->lookupRegionName(Name);
}

const ParamRplElement *SymbolTable::lookupParameterName(const Decl *D, StringRef Name) {
  if (!SymTable.lookup(D))
    return 0;
  return SymTable.lookup(D)->lookupParameterName(Name);
}

bool SymbolTable::hasRegionName(const Decl *D, StringRef Name) {
  return lookupRegionName(D, Name) ? true : false;
}

bool SymbolTable::hasParameterName(const Decl *D, StringRef Name) {
  return lookupParameterName(D, Name) ? true : false;
}

bool SymbolTable::hasRegionOrParameterName(const Decl *D, StringRef Name) {
  return hasRegionName(D, Name) || hasParameterName(D, Name);
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
    SymTable[D] = new SymbolTableEntry();
  SymTable[D]->addParameterName(Name);
  return true;
}

const ParameterVector *SymbolTable::getParameterVectorFromQualType(QualType QT) {
  const ParameterVector *ParamVec = 0;
  if (QT->isReferenceType()) {
    ParamVec = getParameterVectorFromQualType(QT->getPointeeType());
  } else if (const TagType* TT = dyn_cast<TagType>(QT.getTypePtr())) {
    const TagDecl* TD = TT->getDecl();
    TD->dump(OSv2);
    ParamVec = getParameterVector(TD);
  } else if (QT->isBuiltinType() || QT->isPointerType()) {
    // TODO check the number of parameters of the arg attr to be 1
    ParamVec = BuiltinDefaultRegionParameterVec;
  } /// else result = NULL;
  return ParamVec;
}

SymbolTable::SymbolTableEntry::SymbolTableEntry() : Typ(0), ParamVec(0), RegnNameSet(0), EffSum(0) {}

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
