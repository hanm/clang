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
const Effect *SymbolTable::WritesLocal = 0;
SymbolTable *SymbolTable::Table = 0;
VisitorBundle SymbolTable::VB;

/// Static Functions
void SymbolTable::Initialize(VisitorBundle &VisB) {
  if (!Initialized) {
    STAR_RplElmt = new StarRplElement();
    ROOT_RplElmt = new SpecialRplElement("Root");
    LOCAL_RplElmt = new SpecialRplElement("Local");
    GLOBAL_RplElmt = new SpecialRplElement("Global");
    IMMUTABLE_RplElmt = new SpecialRplElement("Immutable");
    Rpl R(*LOCAL_RplElmt);
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
    delete WritesLocal;
    delete Table;

    STAR_RplElmt = 0;
    ROOT_RplElmt = 0;
    LOCAL_RplElmt = 0;
    GLOBAL_RplElmt = 0;
    IMMUTABLE_RplElmt = 0;
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
SymbolTable::SymbolTable() {
  // FIXME: make this static like the other default Regions etc
  ParamRplElement Param("P");
  BuiltinDefaultRegionParameterVec = new ParameterVector(Param);
  AnnotScheme = 0; // must be set via SetAnnotationScheme();
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

const InheritanceMapT *SymbolTable::
getInheritanceMap(QualType QT) const {
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

bool SymbolTable::addToParameterVector(const Decl *D, ParameterVector *&PV) {
  if (!SymTable[D])
    SymTable[D] = new SymbolTableEntry();
  // invariant: SymTable[D] not null
  SymTable[D]->addToParameterVector(PV);
  return true;
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
    EffectSummary *ES = From->clone();
    SymTable[D]->setEffectSummary(ES);
    return true;
  }
}

void SymbolTable::resetEffectSummary(const Decl *D, const EffectSummary *ES) {
  if (!SymTable[D])
    SymTable[D] = new SymbolTableEntry();
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

bool SymbolTable::addBaseTypeAndSub(const Decl *D,
                                    const RecordDecl *Base,
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

void SymbolTable::solveInclusionConstraints(){
  unsigned Num=1;

  for (InclusionConstraintsSetT::iterator I=InclusionConstraints.begin(); I!=InclusionConstraints.end(); ++I)
    {
      std::string FName=(**I)->getDef()->getNameAsString();
      OSv2 << "****" << FName << "*****"<< "\n";
      (**I)->print();
      //The effect variable  
      term_t A = PL_new_term_ref();
      term_t T2  = PL_new_term_ref();
      functor_t EVFunctor;
      EVFunctor = PL_new_functor(PL_new_atom("effect_var"), 1);
      
      std::string EVstr;
      llvm::raw_string_ostream EV(EVstr);
 
      EV << "ev" << Num;

      PL_put_atom_chars(A, EV.str().c_str()); // variable name needs to be fresh
      int Res=PL_cons_functor(T2, EVFunctor, A);
      assert(Res);

      term_t A1 = PL_new_term_ref();
      term_t A2 = PL_new_term_ref();
      term_t A3 = PL_new_term_ref();
      term_t A4 = PL_new_term_ref();
      term_t T  = PL_new_term_ref();

      functor_t ESIFunctor;
      ESIFunctor = PL_new_functor(PL_new_atom("esi_constraint"), 4);
      
 
      std::string ESIstr;
      llvm::raw_string_ostream ESI(ESIstr);
 
      ESI << "esi" << Num;
      PL_put_atom_chars(A1, ESI.str().c_str()); // fresh constraint name
      PL_put_atom_chars(A2, FName.c_str());
      std::string List = "[";
      EffectVector* Lhs=(**I)->getLHS();
      for (EffectVector::iterator It=Lhs->begin(); It!=Lhs->end(); ++It){
        if(It!=Lhs->begin())
          List += ",";
        List +=  (*It)->toString();
      }
      List += "]";
      OSv2 << "LHS is "<< List << "\n";
      Res=PL_put_list_chars(A3, List.c_str());
      assert(Res);

      PL_put_term(A4, T2);
      Res=PL_cons_functor(T, ESIFunctor, A1, A2, A3, A4);
      assert(Res);
      
      predicate_t ESIpred1 = PL_predicate("assertz",1,"user");
      int Rval = PL_call_predicate(NULL, PL_Q_NORMAL, ESIpred1, T);
      
      assert(Rval && "first rval is false");

      predicate_t ESI1 = PL_predicate("esi_collect",4,"user");
      term_t H0 = PL_new_term_refs(4);
      term_t H1 = H0 + 1;
      term_t H2 = H0 + 2;
      term_t H3 = H0 + 3;
      PL_put_atom_chars(H0,EV.str().c_str());
      PL_put_atom_chars(H1,FName.c_str());
      Res=PL_put_list_chars(H2,List.c_str());
      assert(Res);
      
      PL_put_variable(H3);

      Rval = PL_call_predicate(NULL, PL_Q_NORMAL, ESI1, H0);

      assert(Rval && "rval is false");

      char* Solution;
      Res=PL_get_list_chars(H3, &Solution, 0);
      assert(Res);
      OSv2 << "result is "<< Solution << "\n";

      const FunctionDecl *Func=(**I)->getDef();
      const Stmt  *S=(**I)->getS();
      StringRef BugName = "Effect Inclusion Constraint Solution";
  
      std::string BugStr;
      llvm::raw_string_ostream StrOS(BugStr);
      StrOS << "Solution for " << Func->getNameAsString() << ": " << 
        Solution << "\n";
      
      StringRef Str(StrOS.str());
      helperEmitStatementWarning(*VB.BR, VB.AC, S, Func, Str, BugName, false);
      
      OSv2 << "After call\n";
      ++Num;
    }
}

AnnotationSet SymbolTable::makeDefaultType(ValueDecl *ValD, long ParamCount) {
  OSv2 << "DEBUG:: SymbolTable::makeDefaultType\n";
  if (FieldDecl *FieldD = dyn_cast<FieldDecl>(ValD)) {
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
      assert(DC->isFunctionOrMethod() && "Internal error: ParmVarDecl found "
             "outside FunctionDecl Context.");
      FunctionDecl *FunD = dyn_cast<FunctionDecl>(DC);
      assert(FunD);
      addToParameterVector(FunD, AnSe.ParamVec);
      assert(AnSe.ParamVec == 0);
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
//////////////////////////////////////////////////////////////////////////
// SymbolTableEntry

SymbolTableEntry::SymbolTableEntry() :
    Typ(0), ParamVec(0), RegnNameSet(0), EffSum(0), InheritanceMap(0),
    ComputedInheritanceSubVec(false), InheritanceSubVec(0) {}

SymbolTableEntry::~SymbolTableEntry() {
  delete Typ;
  delete ParamVec;
  delete RegnNameSet;
  delete EffSum;
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

const NamedRplElement *SymbolTableEntry::
lookupRegionName(StringRef Name) {
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

void SymbolTableEntry::
addRegionName(StringRef Name) {
  if (!RegnNameSet)
    RegnNameSet = new RegionNameSet();
  RegnNameSet->insert(NamedRplElement(Name));
}

void SymbolTableEntry::
addParameterName(StringRef Name) {
  if (!ParamVec)
    ParamVec = new ParameterVector();
  ParamVec->push_back(ParamRplElement(Name));
}

void SymbolTableEntry::
deleteEffectSummary() {
  delete EffSum;
  EffSum = 0;
}

bool SymbolTableEntry::
addBaseTypeAndSub(const RecordDecl *BaseRD, SymbolTableEntry *BaseTE,
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

void SymbolTableEntry::
computeInheritanceSubVec() {

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

const SubstitutionVector *SymbolTableEntry::
getInheritanceSubVec() {
  if (!InheritanceMap)
    return 0;
  if (!ComputedInheritanceSubVec)
    computeInheritanceSubVec();
  return InheritanceSubVec;
}


} // end namespace asap
} // end namespace clang

