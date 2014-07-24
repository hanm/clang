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
#include <SWI-Prolog.h>
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"

#include "ASaPAnnotationScheme.h"
#include "ASaPSymbolTable.h"
#include "ASaPType.h"
#include "ASaPUtil.h"
#include "Effect.h"
#include "Rpl.h"
#include "SpecificNIChecker.h"
#include "Substitution.h"
#include "ASaPUtil.h"

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
const ConcreteEffectSummary *SymbolTable::PURE_EffSum = 0;
const Effect *SymbolTable::WritesLocal = 0;

SymbolTable *SymbolTable::Table = 0;
VisitorBundle SymbolTable::VB;

/// Static Functions
void SymbolTable::Initialize(VisitorBundle &VisB) {
  if (!Initialized) {
    STAR_RplElmt = new StarRplElement();
    ROOT_RplElmt = new SpecialRplElement(SpecialRplElement::SRK_Root);
    LOCAL_RplElmt = new SpecialRplElement(SpecialRplElement::SRK_Local);
    GLOBAL_RplElmt = new SpecialRplElement(SpecialRplElement::SRK_Global);
    IMMUTABLE_RplElmt = new SpecialRplElement(SpecialRplElement::SRK_Immutable);

    Effect Pure(Effect::EK_NoEffect,0);
    PURE_EffSum = new ConcreteEffectSummary(Pure);

    ConcreteRpl R(*LOCAL_RplElmt);
    R.appendElement(STAR_RplElmt);
    WritesLocal = new Effect(Effect::EK_WritesEffect, &R);
    Table = new SymbolTable();
    VB = VisB;
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
    delete PURE_EffSum;

    delete WritesLocal;
    delete Table;

    STAR_RplElmt = 0;
    ROOT_RplElmt = 0;
    LOCAL_RplElmt = 0;
    GLOBAL_RplElmt = 0;
    IMMUTABLE_RplElmt = 0;
    PURE_EffSum = 0;

    WritesLocal = 0;
    Table = 0;
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
SymbolTable::SymbolTable()
    : AnnotScheme(0), ParamIDNumber(0),
      RegionIDNumber(0), DeclIDNumber(0),
      RVIDNumber(0), ESVIDNumber(0),
      RplDomIDNumber(0), ConstraintIDNumber(0) {
  // FIXME: make this static like the other default Regions etc
  ParamRplElement Param("P","p");
  BuiltinDefaultRegionParameterVec = new ParameterVector(Param);
  //AnnotScheme = 0; // must be set via SetAnnotationScheme();
}

SymbolTable::~SymbolTable() {
  delete BuiltinDefaultRegionParameterVec;

  for(SymbolTableMapT::iterator I = SymTable.begin(), E = SymTable.end();
      I != E; ++I) {
    // for this key, delete the value
    delete (*I).second;
  }

  for(ParallelismMapT::iterator I = ParTable.begin(), E = ParTable.end();
      I != E; ++I) {
    // for this key, delete the value
    delete (*I).second;
  }
}

ResultTriplet SymbolTable::getRegionParamCount(QualType QT) {
  if (isNonPointerScalarType(QT)) {
    OSv2 << "DEBUG:: getRegionParamCount::isNonPointerScalarType\n";
    return ResultTriplet(RK_OK, 1, 0);
  } else if (QT->isAtomicType()) {
    const AtomicType *AT = QT->getAs<AtomicType>();
    return getRegionParamCount(AT->getValueType());
  } else if (QT->isArrayType()) {
    OSv2 << "DEBUG:: getRegionParamCount::isArrayType\n";
    // It is not allowed to use getAs<T> with T = ArrayType,
    // so we use getAsArrayTypeUnsafe
    const ArrayType *AT = QT->getAsArrayTypeUnsafe();
    assert(AT);
    QualType ElQT = AT->getElementType();
    return getRegionParamCount(ElQT);
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

    QualType ResultQT = FT->getReturnType();
    return getRegionParamCount(ResultQT);
  } else if (QT->isVoidType()) {
    OSv2 << "DEBUG:: getRegionParamCount::isVoidType\n";
    return ResultTriplet(RK_OK, 0, 0);
  } else if (QT->isTemplateTypeParmType()) {
    OSv2 << "DEBUG:: getRegionParamCount::isTemplateParmType\n";
    return ResultTriplet(RK_VAR, 0, 0);
  } else if (QT->isDependentType()) {
    OSv2 << "DEBUG:: getRegionParamCount::isDependentType\n";
    return ResultTriplet(RK_VAR, 0, 0);
  } else if (QT->isUnionType()) {
    OSv2 << "DEBUG:: getRegionParamCount::isUnionType ";
    OSv2 << (QT->hasUnnamedOrLocalType() ? "(Named Union)" : "(ANONYMOUS Union)") << "\n";
    return ResultTriplet(RK_OK, 0, 0);
  } else {
    OSv2 << "DEBUG:: getRegionParamCount::UnexpectedType!! QT = "
         << QT.getAsString() << "\n";
    OSv2 << "DEBUG:: QT.dump:\n";
    QT.dump();
    OSv2 << "isAtomicType = " << QT->isAtomicType() << "\n";
    OSv2 << "isBuiltinType = " << QT->isBuiltinType() << "\n";
    //OSv2 << "isSpecificBuiltinType = " << QT->isSpecificBuiltinType() << "\n";
    OSv2 << "isPlaceholderType = " << QT->isPlaceholderType() << "\n";
    //OSv2 << "isSpecificPlaceholderType = " << QT->isSpecificPlaceholderType() << "\n";

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

bool SymbolTable::hasInheritanceMap(const Decl* D) const {
  if (!SymTable.lookup(D))
    return false;
  return SymTable.lookup(D)->hasInheritanceMap();
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

const InheritanceMapT *SymbolTable::getInheritanceMap(QualType QT) const {
  const InheritanceMapT *Result = 0;
  // QT might be a pointer or reference, so follow it
  while (QT->isPointerType() || QT->isReferenceType()) {
    QT = QT->getPointeeType();
  }
  if (const CXXRecordDecl *RecD = QT->getAsCXXRecordDecl()) {
      /*OSv2 << "DEBUG:: the type is";
      OSv2 << (RecD->hasNameForLinkage() ? "(Named)" : "(ANONYMOUS)") << ":\n";
      RecD->dump(OSv2);
      OSv2 << "\n";*/

      assert(hasDecl(RecD) && "Internal error: type missing declaration");
      Result = getInheritanceMap(RecD);
  }
  return Result;
}

const InheritanceMapT *SymbolTable::
getInheritanceMap(const ValueDecl *D) const {
  return getInheritanceMap(D->getType());
}

const InheritanceMapT *SymbolTable::
getInheritanceMap(const CXXRecordDecl *D) const {
  if (!SymTable.lookup(D))
    return 0;
  else
    return SymTable.lookup(D)->getInheritanceMap();
}

const SubstitutionVector *SymbolTable::
getInheritanceSubVec(const Decl *D) const {
  if (!SymTable.lookup(D))
    return 0;
  else
    return SymTable.lookup(D)->getInheritanceSubVec();
}

const StringRef SymbolTable::getPrologName(const Decl *D) const {
  if (!SymTable.lookup(D)) {
    assert(false && "Internal Error: Decl missing from Symbol Table");
    return PL_UnNamedDecl;
  } else
    return SymTable.lookup(D)->getPrologName();
}

const RegionNameSet *SymbolTable::getRegionNameSet(const Decl *D) const {
  if (!SymTable.lookup(D))
    return 0;
  else
    return SymTable.lookup(D)->getRegionNameSet();
}

RplDomain *SymbolTable::getRplDomain(const Decl *D) const {
  if (!SymTable.lookup(D))
    return 0;
  return SymTable.lookup(D)->getRplDomain();
}

RplDomain *SymbolTable::buildDomain(const ValueDecl *D) {
  const DeclContext *DC = D->getDeclContext();
  const Decl *EnclosingDecl = getDeclFromContext(DC);
  //add fresh region name to enclosing scope
  StringRef Name;
  if (const NamedDecl *ND = dyn_cast<NamedDecl>(D)) {
    Name = ND->getNameAsString();
  } else {
    Name = "";
  }
  StringRef RegName = makeFreshRegionName(Name);
  bool Res = addRegionName(EnclosingDecl, RegName, false);
  assert(Res && "Internal Error: failed to add fresh region name");

  const RplDomain *ParentDom = getRplDomain(EnclosingDecl);
  if (ParentDom)
    return new RplDomain(*ParentDom);
  else
    return 0;
}

const EffectSummary *SymbolTable::getEffectSummary(const Decl *D) const {
  if (!SymTable.lookup(D))
    return 0;
  else
    return SymTable.lookup(D)->getEffectSummary();
}

EffectInclusionConstraint *SymbolTable::getEffectInclusionConstraint(const Decl *D) const {
  if (!SymTable.lookup(D))
    return 0;
  else
    return SymTable.lookup(D)->getEffectInclusionConstraint();
}

bool SymbolTable::setType(const Decl* D, ASaPType *T) {
  if (!SymTable.lookup(D))
    createSymbolTableEntry(D);
  // invariant: SymTable[D] not null
  if (SymTable[D]->hasType())
    return false;
  else {
    SymTable[D]->setType(T);
    return true;
  }
}

bool SymbolTable::initParameterVector(const Decl *D) {
  if (!SymTable.lookup(D))
    createSymbolTableEntry(D);
  // invariant: SymTable[D] not null
  if (SymTable[D]->hasParameterVector())
    return false;
  else {
    SymTable[D]->setParameterVector(new ParameterVector());
    return true;
  }
}

bool SymbolTable::setParameterVector(const Decl *D, ParameterVector *PV) {
  if (!SymTable.lookup(D))
    createSymbolTableEntry(D);
  // invariant: SymTable[D] not null
  if (SymTable[D]->hasParameterVector())
    return false;
  else {
    SymTable[D]->setParameterVector(PV);
    return true;
  }
}

bool SymbolTable::addToParameterVector(const Decl *D, ParameterVector *&PV) {
  if (!SymTable.lookup(D))
    createSymbolTableEntry(D);
  // invariant: SymTable[D] not null
  SymTable[D]->addToParameterVector(PV);
  return true;
}

bool SymbolTable::setRegionNameSet(const Decl *D, RegionNameSet *RNS) {
  if (!SymTable.lookup(D))
    createSymbolTableEntry(D);
  // invariant: SymTable[D] not null
  if (SymTable[D]->hasRegionNameSet())
    return false;
  else {
    SymTable[D]->setRegionNameSet(RNS);
    return true;
  }
}

bool SymbolTable::setEffectSummary(const Decl *D, EffectSummary *ES) {
  if (!SymTable.lookup(D))
    createSymbolTableEntry(D);
  // invariant: SymTable[D] not null
  if (SymTable[D]->hasEffectSummary())
    return false;
  else {
    SymTable[D]->setEffectSummary(ES);
    return true;
  }
}

bool SymbolTable::setEffectSummary(const Decl *D, const Decl *Dfrom) {
  if (!SymTable.lookup(Dfrom) || !SymTable[Dfrom]->hasEffectSummary())
    return false;

  if (!SymTable.lookup(D))
    createSymbolTableEntry(D);
  // invariant: SymTable[D] not null
  if (SymTable[D]->hasEffectSummary())
    return false;
  else {
    const EffectSummary *From = SymTable[Dfrom]->getEffectSummary();
    EffectSummary *ES = From->clone();
    SymTable[D]->setEffectSummary(ES);
    return true;
  }
}

void SymbolTable::resetEffectSummary(const Decl *D, const EffectSummary *ES) {
  if (!SymTable.lookup(D))
    createSymbolTableEntry(D);
  // invariant: SymTable[D] not null
  if (SymTable[D]->hasEffectSummary())
    SymTable[D]->deleteEffectSummary();

  EffectSummary* Sum = ES->clone();
  SymTable[D]->setEffectSummary(Sum);
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

const RplElement *SymbolTable::
lookupRegionOrParameterName(const Decl *D, StringRef Name) const {
  if (!SymTable.lookup(D))
    return 0;
  const RplElement *Result =  SymTable.lookup(D)->lookupParameterName(Name);
  if (!Result)
    Result = SymTable.lookup(D)->lookupRegionName(Name);
  return Result;
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

bool SymbolTable::hasBase(const Decl *D, const RecordDecl *Base) const {
  if (!SymTable.lookup(D) || !SymTable.lookup(Base))
    return false;
  return (SymTable.lookup(D)->getSubVec(Base) == 0)? false : true;
}

bool SymbolTable::addRegionName(const Decl *D,
                                StringRef Name,
                                bool MakePrologName) {
  if (hasRegionOrParameterName(D, Name))
    return false;
  if (!SymTable.lookup(D))
    createSymbolTableEntry(D);
  if (MakePrologName) {
    StringRef PrologName = makeFreshRegionName(Name);
    SymTable[D]->addRegionName(Name, PrologName);
  } else {
    SymTable[D]->addRegionName(Name, Name);
  }
  return true;
}

bool SymbolTable::addParameterName(const Decl *D, StringRef Name) {
  if (hasRegionOrParameterName(D, Name))
    return false;
  if (!SymTable.lookup(D))
    createSymbolTableEntry(D);
  StringRef PrologName = makeFreshParamName(Name);
  SymTable[D]->addParameterName(Name, PrologName);
  return true;
}

bool SymbolTable::addBaseTypeAndSub(const Decl *D,
                                    const RecordDecl *Base,
                                    SubstitutionVector *&SubV) {
  if (!SubV)
    return true; // Nothing to do here
  if (hasBase(D, Base)) {
    return false; // FIXME: should we instead merge SubVs?...
  }
  if (!SymTable.lookup(D))
    createSymbolTableEntry(D);

  if (!SymTable.lookup(Base))
    createSymbolTableEntry(Base);

  SymTable[D]->addBaseTypeAndSub(Base, SymTable.lookup(Base), SubV);
  return true;
}

bool SymbolTable::
addParallelFun(const FunctionDecl *D, const SpecificNIChecker *NIC) {
  if (!ParTable.lookup(D)) {
    ParTable[D] = NIC;
    return true;
  } else {
    delete NIC;
    return false;
  }
}

void SymbolTable::updateEffectInclusionConstraint(const FunctionDecl *Def,
                                                  ConcreteEffectSummary &CES) {
  EffectInclusionConstraint *EIC = getEffectInclusionConstraint(Def);
  if (EIC) {
    EIC->addEffects(CES);
  } else {
    EIC = new EffectInclusionConstraint(makeFreshConstraintName(),
                                        &CES, getEffectSummary(Def),
                                        Def, getBody(Def));
    addConstraint(EIC);
  }
}

bool SymbolTable::addInclusionConstraint(const FunctionDecl *FunD,
                                         EffectInclusionConstraint *EIC) {
  if (!SymTable.lookup(FunD))
    return false;
  return SymTable[FunD]->addInclusionConstraint(EIC);
}

void SymbolTable::
assertzHasEffectSummary(const NamedDecl *NDec,
                        const ConcreteEffectSummary *EffSum) const {
  term_t EffSumT = EffSum->getPLTerm();
  term_t HasEffSumT = PL_new_term_ref();
  functor_t HasEffSumF =
      PL_new_functor(PL_new_atom(PL_HasEffSum.c_str()),2);
  term_t NameT = PL_new_term_ref();
  PL_put_atom_chars(NameT, getPrologName(NDec).data());
  int Res = PL_cons_functor(HasEffSumT, HasEffSumF, NameT, EffSumT);
  assert(Res && "Failed to build 'has_effect_summary' functor");
  assertzTermProlog(HasEffSumT, "Failed to assert 'has_effect_summary' to Prolog facts");
}

const ParameterVector *SymbolTable::getParameterVectorFromQualType(QualType QT) const {
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

static void emitConstraintSolution(EffectInclusionConstraint *EC,
                                   char* Solution){
  const FunctionDecl *Func = EC->getDef();
  const Stmt  *S = EC->getS();
  StringRef BugName = "Effect Inclusion Constraint Solution";

  std::string BugStr;
  llvm::raw_string_ostream StrOS(BugStr);
  StrOS << "Inferred Effect Summary for " << Func->getNameAsString()
        << ": " << Solution;

  StringRef Str(StrOS.str());
  helperEmitStatementWarning(SymbolTable::VB.Checker,
                             *SymbolTable::VB.BR,
                             SymbolTable::VB.AC,
                             S, Func, Str, BugName, false);

}

void SymbolTable::addConstraint(Constraint *Cons) {
  assert(Cons && "Internal Error: unexpected null-pointer");
  ConstraintSet.insert(Cons);
  if (EffectInclusionConstraint *EIC =
        dyn_cast<EffectInclusionConstraint>(Cons)) {
    addInclusionConstraint(EIC->getDef(), EIC);
  }
}

void SymbolTable::emitFacts() const {
  //PL_action(PL_ACTION_TRACE);
  //iterate through symbol table entries and emit facts
  for (SymbolTableMapT::const_iterator
         MI = SymTable.begin(),
         ME = SymTable.end();
       MI != ME; ++MI) {

    const Decl *Dec = (*MI).first;
    const SymbolTableEntry *Entry = (*MI).second;
    assert(Dec && Entry);

    if (Entry->hasParameterVector()) {
      //OSv2 << "DEBUG:: gonna assert a parameter vector\n";
      Entry->getParameterVector()->assertzProlog();
      //OSv2 << "DEBUG:: asserted a parameter vector\n";
    }
    if (Entry->getRegionNameSet()) {
      //OSv2 << "DEBUG:: gonna assert a region name set\n";
      Entry->getRegionNameSet()->assertzProlog();
      //OSv2 << "DEBUG:: asserted a region name set\n";
    }
    const RplDomain *Dom = Entry->getRplDomain();
    if (Dom && !Dom->isUsed()) {
      Dom->assertzProlog();
    }
    if (isa<FunctionDecl>(Dec) && Entry->hasEffectSummary()) {
      const EffectSummary *EffSum = Entry->getEffectSummary();
      assert(isa<NamedDecl>(Dec) && "Internal Error: Expected NamedDecl");
      const NamedDecl *NDec = dyn_cast<NamedDecl>(Dec);
      *VB.OS << "DEBUG:: NamedDecl = " << NDec->getNameAsString()
            << ", PrologName = " << getPrologName(NDec)
            << ", EffSum = " << Entry->getEffectSummary()->toString() << "\n";
      if (isa<ConcreteEffectSummary>(EffSum)) {
        const ConcreteEffectSummary *CEffSum =
            dyn_cast<ConcreteEffectSummary>(EffSum);
        assertzHasEffectSummary(NDec, CEffSum);
      } else if (isa<VarEffectSummary>(EffSum)) {
        const VarEffectSummary *VarES = dyn_cast<VarEffectSummary>(EffSum);
        if (VarES->hasInclusionConstraint()) {
          term_t InclConsT = VarES->getInclusionConstraint()->getPLTerm();
          //*VB.OS << "DEBUG:: gonna assert a var effect summary\n";
          assertzTermProlog(InclConsT, "Failed to assert 'esi constraint' to Prolog facts");
          //*VB.OS << "DEBUG:: asserted a var effect summary\n";
        } else {
          // Emit pure effect summary to prolog
          //*VB.OS << "DEBUG:: gonna assert a pure effect summary\n";
          assertzHasEffectSummary(NDec, PURE_EffSum);
          //*VB.OS << "DEBUG:: asserted a pure effect summary\n";
        }
      } // end isa<VarEffectSummary>
    }
  } // end for-all symbol table entries
  OSv2 << "DEBUG:: Done emmitting facts to Prolog\n";
}

void SymbolTable::emitConstraints() const {
  //PL_action(PL_ACTION_TRACE);
  // Emit Constraints
  for (ConstraintsSetT::iterator
          I = ConstraintSet.begin(),
          E = ConstraintSet.end();
       I != E; ++I) {
    if (!isa<EffectInclusionConstraint>(*I)) {
      OSv2 << "DEBUG:: Will assert Constraint to Prolog: " << (*I)->toString() << "\n";
      term_t Term = (*I)->getPLTerm();
      OSv2 << "DEBUG:: build term for constraint...\n";
      assertzTermProlog(Term, "Failed to assert constraint to Prolog facts");
      OSv2 << "DEBUG:: Asserted Constraint to Prolog: " << (*I)->toString() << "\n";
    }
  }
}

void SymbolTable::solveConstraints() const {
  emitFacts();
  emitConstraints();
  //loop to call esi_collect (effect inference)
  for (ConstraintsSetT::iterator
          I = ConstraintSet.begin(),
          E = ConstraintSet.end();
       I != E; ++I) {
    if (EffectInclusionConstraint *EIC = dyn_cast<EffectInclusionConstraint>(*I)) {
      const FunctionDecl *FunD = EIC->getDef();
      assert(FunD && "Internal Error: Effect Inclusion Constraint without matching FunctionDecl");
      StringRef FName = getPrologName(FunD);

      OSv2 << "DEBUG:: **** Invoking inference for method '"
          << FunD->getNameAsString() << "' (Prolog Name: "
          << FName << ") ****\n";

      std::string EVstr;
      llvm::raw_string_ostream EV(EVstr);

      EV << "ev" << FName;

      term_t LHS = EIC->getLHS()->getPLTerm();

      predicate_t ESI1 = PL_predicate("esi_collect",4,"user");
      term_t H0 = PL_new_term_refs(4);
      term_t H1 = H0 + 1;
      term_t H2 = H0 + 2;
      term_t H3 = H0 + 3;
      PL_put_atom_chars(H0,EV.str().c_str());
      PL_put_atom_chars(H1,FName.data());

      PL_put_term(H2,LHS);
      PL_put_variable(H3);

      int Rval = PL_call_predicate(NULL, PL_Q_NORMAL, ESI1, H0);

      assert(Rval && "Effect Inference Failed");

      char *Solution;
      int Res = PL_get_chars(H3, &Solution, CVT_WRITE|BUF_RING);
      assert(Res && "Failed to read solution from Prolog");
      OSv2 << "result is "<< Solution << "\n";

      emitConstraintSolution(EIC, Solution);
    }
  }
}

AnnotationSet SymbolTable::makeDefaultType(ValueDecl *ValD, long ParamCount) {
  OSv2 << "DEBUG:: SymbolTable::makeDefaultType\n";
  if (FieldDecl *FieldD = dyn_cast<FieldDecl>(ValD)) {
    OSv2 << "DEBUG:: SymbolTable::makeDefaultType ValD isa FiledDecl\n";
    AnnotationSet AnSe = AnnotScheme->makeFieldType(FieldD, ParamCount);
    assert(AnSe.ParamVec == 0 && "Internal Error: Not allowed to create a "
                                 "region parameter in method makeDefaultType");
    return AnSe;
  } else if (/*ImplicitParamDecl *ImplParamD = */dyn_cast<ImplicitParamDecl>(ValD)) {
    assert(false && "ImplicitParamDecl case not implemented!");
  } else if (ParmVarDecl *ParamD = dyn_cast<ParmVarDecl>(ValD)) {
    OSv2 << "DEBUG::         case ParmVarDecl (ParamCount = " << ParamCount << ")\n";
    AnnotationSet AnSe = AnnotScheme->makeParamType(ParamD, ParamCount);
    if (AnSe.ParamVec) {
      DeclContext *DC = ParamD->getDeclContext();
      OSv2 << "DEBUG:: DeclContext:";
      DC->dumpDeclContext();
      OSv2 << "\n";
      OSv2 << "DEBUG:: DeclContext->isFunctionOrMethod = "  << DC->isFunctionOrMethod() << "\n";
      //assert(DC->isFunctionOrMethod() && "Internal error: ParmVarDecl found "
      //       "outside FunctionDecl Context.");
      if (DC->isFunctionOrMethod()) {
        FunctionDecl *FunD = dyn_cast<FunctionDecl>(DC);
        assert(FunD && "Expected DeclContext of type FunctionDecl");
        addToParameterVector(FunD, AnSe.ParamVec);
        assert(AnSe.ParamVec == 0);
      } else {
        // FIXME:: this is a temporary work-around for function pointers with
        // parameters. The parameters should be added to the fn-pointer type
        // but (a) we don't reason about those correctly yet and (b) I'm not
        // sure how to get the fn-ptr type from the parameter declaration at
        // this point.
        addToParameterVector(ParamD, AnSe.ParamVec);
        assert(AnSe.ParamVec == 0);
      }
    }
    OSv2 << "DEBUG::         case ParmVarDecl = DONE\n";
    return AnSe;
  } else if (VarDecl *VarD = dyn_cast<VarDecl>(ValD)) {
      if (VarD->isStaticLocal() || VarD->isStaticDataMember()
          || VarD->getDeclContext()->isFileContext()) {
        // Global
        return AnnotScheme->makeGlobalType(VarD, ParamCount);
      } else {
        // Local
        return AnnotScheme->makeStackType(VarD, ParamCount);
      }
  } else if (FunctionDecl *FunD = dyn_cast<FunctionDecl>(ValD)) {
    AnnotationSet AnSe = AnnotScheme->makeReturnType(FunD, ParamCount);
    if (AnSe.ParamVec) {
      addToParameterVector(FunD, AnSe.ParamVec);
      assert(AnSe.ParamVec == 0);
    }
    return AnSe;
  } else {
    OSv2 << "DEBUG:: ";
    //ValD->print(OSv2);
    ValD->dump(OSv2);
    OSv2 << "\n";
    assert(false && "Internal error: unknown kind of ValueDecl in "
           "SymbolTable::makeDefaultType");
  }
}

RplVector *SymbolTable::
makeDefaultBaseArgs(const RecordDecl *Derived, long NumArgs) {
  return AnnotScheme->makeBaseTypeArgs(Derived, NumArgs);
}

void SymbolTable::createSymbolTableEntry(const Decl *D) {
  assert(!SymTable.lookup(D) && "Internal Error: trying to create duplicate entry");
  // 1. Make names for decl and domain
  StringRef DeclName = makeFreshDeclName("");
  StringRef DomName = makeFreshRplDomName("");
  // TODO: update DeclName:StringRef -> Decl Map

  // 2. Compute ParentDom
  RplDomain *ParentDom = 0;
  if (const DeclContext *DC = D->getDeclContext()) {
    const Decl *EnclosingDecl = getDeclFromContext(DC);
    ParentDom = getRplDomain(EnclosingDecl);
  }
  SymTable[D] = new SymbolTableEntry(DeclName, DomName, ParentDom);
}

VarRpl *SymbolTable::createFreshRplVar(const ValueDecl *D) {
  StringRef Name = makeFreshRVName(D->getNameAsString().data());
  OSv2 << "DEBUG:: VarRpl Fresh Name created: " << Name << "\n";
  RplDomain *Dom = buildDomain(D);
  return new VarRpl(Name, Dom);
}

VarEffectSummary *SymbolTable::createFreshEffectSumVar(const FunctionDecl *D) {
  // TODO: use name of D after processing to make it a valid Prolog identifier
  StringRef Name = makeFreshESVName("");
  return new VarEffectSummary(Name);
}
//////////////////////////////////////////////////////////////////////////
// SymbolTableEntry

SymbolTableEntry::SymbolTableEntry(StringRef DeclName,
                                   StringRef DomName,
                                   RplDomain *ParentDom)
                                  : PrologName(DeclName),
                                    Typ(0), EffSum(0),
                                    InheritanceMap(0),
                                    ComputedInheritanceSubVec(false),
                                    InheritanceSubVec(0) {
  ParamVec = new ParameterVector();
  RegnNameSet = new RegionNameSet();
  RplDom = new RplDomain(DomName, 0, ParamVec, ParentDom);

}

SymbolTableEntry::~SymbolTableEntry() {
  delete Typ;
  delete ParamVec;
  delete RegnNameSet;
  delete EffSum;
  delete RplDom;
  if (InheritanceMap && InheritanceMap->size() > 0) {
    for(InheritanceMapT::iterator
          I = InheritanceMap->begin(), E = InheritanceMap->end();
        I != E; ++I) {
      delete (*I).second.second; // Delete the SubstitutionVector
    }
    delete InheritanceMap;
  }
  delete InheritanceSubVec;
}

void SymbolTableEntry::
addToParameterVector(ParameterVector *&PV) {
  if (!ParamVec) {
    ParamVec = PV;
    PV = 0;
  } else {
    ParamVec->take(PV);
    assert(PV==0);
  }
}

const NamedRplElement *SymbolTableEntry::lookupRegionName(StringRef Name) {
  if (!RegnNameSet)
    return 0;
  return RegnNameSet->lookup(Name);
}

const ParamRplElement *SymbolTableEntry::
lookupParameterName(StringRef Name) {
  if (!ParamVec)
    return 0;
  return ParamVec->lookup(Name);
}

void SymbolTableEntry::addRegionName(StringRef Name, StringRef PrologName) {
  OSv2 << "in addRegionName 1\n";
  RegnNameSet->insert(NamedRplElement(Name, PrologName));
  RplDom->addRegion(NamedRplElement(Name, PrologName));
  OSv2 << "addRegionName is done\n";
}

void SymbolTableEntry::addParameterName(StringRef Name, StringRef PrologName) {
  if (!ParamVec)
    ParamVec = new ParameterVector();

  ParamVec->push_back(ParamRplElement(Name, PrologName));
}

bool SymbolTableEntry::addInclusionConstraint(EffectInclusionConstraint *EIC) {
  if (!EffSum || !isa<VarEffectSummary>(EffSum))
    return false;
  VarEffectSummary *VES = dyn_cast<VarEffectSummary>(EffSum);
  assert(VES && "Internal Error: unexpected kind of effect summary");
  VES->setInclusionConstraint(EIC);
  return true;
}

EffectInclusionConstraint *SymbolTableEntry::getEffectInclusionConstraint() const {
  if (!EffSum || !isa<VarEffectSummary>(EffSum))
    return 0;
  VarEffectSummary *VES = dyn_cast<VarEffectSummary>(EffSum);
  assert(VES && "Internal Error: unexpected kind of effect summary");
  return VES->getInclusionConstraint();
}

void SymbolTableEntry::deleteEffectSummary() {
  delete EffSum;
  EffSum = 0;
}

bool SymbolTableEntry::addBaseTypeAndSub(const RecordDecl *BaseRD,
                                         SymbolTableEntry *BaseTE,
                                         SubstitutionVector *&SubV) {
  // If we're not adding a substitution then skip it altogether.
  if (!SubV)
    return true;
  // Allocate map if it doesn't exist.
  if (!InheritanceMap)
    InheritanceMap = new InheritanceMapT();
  // Add to map.
  std::pair<SymbolTableEntry *, SubstitutionVector *> p(BaseTE, SubV);
  (*InheritanceMap)[BaseRD] = p;
  SubV = 0;
  return true;
}

const SubstitutionVector * SymbolTableEntry::
getSubVec(const RecordDecl *Base) const {
  if (!InheritanceMap)
    return 0;
  return (*InheritanceMap)[Base].second;
}

void SymbolTableEntry::computeInheritanceSubVec() {

  if (!ComputedInheritanceSubVec
      && InheritanceMap && InheritanceMap->size() > 0) {

    assert(!InheritanceSubVec);
    InheritanceSubVec = new SubstitutionVector();
    for(InheritanceMapT::iterator I = InheritanceMap->begin(),
                                  E = InheritanceMap->end();
        I != E; ++I) {
      // 1. Recursive call: getInheritanceSubVec()
      SymbolTableEntry *STE = (*I).second.first;
      InheritanceSubVec->push_back_vec(STE->getInheritanceSubVec());
      // 2. Last step: add these here substitutions
      const SubstitutionVector *SubV = (*I).second.second;
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

const SubstitutionVector *SymbolTableEntry::getInheritanceSubVec() {
  if (!InheritanceMap)
    return 0;
  if (!ComputedInheritanceSubVec)
    computeInheritanceSubVec();
  return InheritanceSubVec;
}


} // end namespace asap
} // end namespace clang

