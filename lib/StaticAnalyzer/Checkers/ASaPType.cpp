//=== ASaPType.cpp - Safe Parallelism checker -----*- C++ -*---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This file defines the ASaPType class used by the Safe Parallelism
// checker, which tries to prove the safety of parallelism given region
// and effect annotations.
//
//===----------------------------------------------------------------===//

#include "clang/AST/ASTContext.h"
#include "clang/AST/CXXInheritance.h"

#include "ASaPUtil.h"
#include "ASaPType.h"
#include "Effect.h"
#include "Rpl.h"
#include "Substitution.h"

namespace clang {
namespace asap {

// Static function
bool ASaPType::
isDerivedFrom(QualType Derived, QualType Base) {

  //OSv2 << "DEBUG:: isDerived: let's start...\n";
  CXXRecordDecl *DerivedRD = Derived->getAsCXXRecordDecl();
  if (!DerivedRD)
    return false;

  CXXRecordDecl *BaseRD = Base->getAsCXXRecordDecl();
  if (!BaseRD)
    return false;

  //OSv2 << "DEBUG:: isDerived: both non null...\n";
  // If either the base or the derived type is invalid, don't try to
  // check whether one is derived from the other.
  if (BaseRD->isInvalidDecl() || DerivedRD->isInvalidDecl())
    return false;

  //OSv2 << "DEBUG:: isDerived: about to compare...";
  // FIXME: instantiate DerivedRD if necessary.  We need a PoI for this.
  bool Result = DerivedRD->hasDefinition() &&
                DerivedRD->isDerivedFrom(BaseRD);
  //OSv2 << "Result:" << Result << "\n";
  return Result;
}

// Non-Static Functions
void ASaPType::adjust() {
  // Check if we might need to set InRpl.
  if (!this->InRpl) {
    // Figure out based on QT if we need an InRpl.
    if (this->QT->isScalarType() && !this->QT->isReferenceType()) {
      // Get it from the head of ArgV.
      assert(this->ArgV && this->ArgV->size() > 0);
      this->InRpl = this->ArgV->pop_front().release();
    }
  }
}

// Constructor
ASaPType::ASaPType(QualType QT, const InheritanceMapT *InheritanceMap,
                   RplVector *ArgV, Rpl *InRpl, bool Simple)
                   : QT(QT), InheritanceMap(InheritanceMap) {
  // 1. Set InRpl & ArgV.
  if (InRpl)
    this->InRpl = new Rpl(*InRpl);
  else
    this->InRpl = 0;
  if (ArgV)
    this->ArgV = new RplVector(*ArgV);
  else
    this->ArgV = new RplVector();
  if (!Simple) {
    adjust();
  } // End if (!Simple).
}

// Copy Constructor
ASaPType::ASaPType(const ASaPType &T)
  : QT(T.QT), InheritanceMap(T.InheritanceMap) {
  // 1. Copy QT
  this->QT = T.QT;
  // 2. Copy InRpl
  if (T.InRpl)
    this->InRpl = new Rpl(*T.InRpl);
  else
    this->InRpl = 0;
  // 3. Copy ArgV
  if (T.ArgV)
    this->ArgV = new RplVector(*T.ArgV);
  else
    this->ArgV = new RplVector();
}

// Destructor
ASaPType::~ASaPType() {
  delete InRpl;
  delete ArgV;
}

int ASaPType::getArgVSize() const {
  if (ArgV)
    return ArgV->size();
  else
    return 0;
}

const RplVector *ASaPType::getArgV() const { return ArgV; }

const Rpl *ASaPType::getInRpl() const { return InRpl; }

const Rpl *ASaPType::getInRpl(int DerefNum) const {
  assert(DerefNum >= -1);
  if (DerefNum == -1) return 0;
  if (DerefNum == 0) return InRpl;
  return this->ArgV->getRplAt(DerefNum-1);
}

const Rpl *ASaPType::getSubstArg(int DerefNum) const {
  assert(DerefNum >= -1);
  if (DerefNum == -1) return InRpl;
  if (InRpl) {
    if (DerefNum == 0)
      assert(false);
    else
      return this->ArgV->getRplAt(DerefNum-1);
  } else
    return this->ArgV->getRplAt(DerefNum);
}

QualType ASaPType::getQT(int DerefNum) const {
  assert(DerefNum >= 0);
  QualType Result = QT;
  while (DerefNum > 0) {
    assert(Result->isPointerType());
    Result = Result->getPointeeType();
    --DerefNum;
  }
  return Result;
}

ASaPType *ASaPType::getReturnType() {
  if (QT->isFunctionType()) {
    const FunctionType *FT = QT->getAs<FunctionType>();
    QT = FT->getResultType();
    adjust();
    return this;
  } else {
    return 0;
  }
}

void ASaPType::deref(int DerefNum) {
  assert(DerefNum >= 0);
  if (DerefNum == 0)
    return;
  assert(ArgV);
  while (DerefNum > 0) {
    if (InRpl)
      delete InRpl;
    assert(QT->isPointerType() || QT->isReferenceType());
    QT = QT->getPointeeType();

    // if the dereferenced type is scalar, we place the head of the
    // args vector in the InRpl field; otherwise InRpl stays empty
    // as in the case of C++ object types.
    if (QT->isScalarType())
      InRpl = ArgV->deref().release();
    else
      InRpl = 0;
    DerefNum--;
  }
}

void ASaPType::addrOf(QualType RefQT) {
  assert(RefQT->isPointerType() || RefQT->isReferenceType());
  assert(areUnqualQTsEqual(this->QT, RefQT->getPointeeType()));
  this->QT = RefQT;

  if (InRpl) {
    ArgV->push_front(InRpl);
    InRpl = 0;
  }
}

void ASaPType::dropInRpl() {
  delete InRpl;
  InRpl = 0;
}

std::string ASaPType::toString(ASTContext &Ctx) const {
  std::string SBuf;
  llvm::raw_string_ostream OS(SBuf);
  QT.print(OS, Ctx.getPrintingPolicy());
  OS << ", ";
  if (InRpl)
    OS << "IN:" << InRpl->toString();
  else
    OS << "IN:<empty>";
  OS << ", ArgV:" << ArgV->toString();
  return std::string(OS.str());
}

std::string ASaPType::toString() const {
  std::string SBuf;
  llvm::raw_string_ostream OS(SBuf);
  OS << QT.getAsString();
  OS << ", ";
  if (InRpl)
    OS << "IN:" << InRpl->toString();
  else
    OS << "IN:<empty>";
  OS << ", ArgV:" << ArgV->toString();
  return std::string(OS.str());
}

bool ASaPType::
isAssignableTo(const ASaPType &That, SymbolTable &SymT,
               ASTContext &Ctx, bool IsInit) const {
  OSv2 << "DEBUG:: isAssignable [IsInit=" << IsInit
  << "]\n";
  OSv2 << "RHS:" << this->toString() << "\n";
  OSv2 << "LHS:" << That.toString() << "\n";

  ASaPType ThisCopy(*this);
  if (ThisCopy.QT->isReferenceType()) {
    ThisCopy.deref();
  }

  ASaPType ThatCopy(That);
  if (ThatCopy.QT->isReferenceType()) {
    ThatCopy.deref();
  }

  if (IsInit) {
    if (That.QT->isReferenceType()) {
      ThatCopy.addrOf(Ctx.getPointerType(ThatCopy.QT));
      QualType ThisRef = Ctx.getPointerType(ThisCopy.QT);
      OSv2 << "DEBUG:: ThisRef:" << ThisRef.getAsString() << "\n";
      ThisCopy.addrOf(ThisRef);
    }
  } // end IsInit == true
  return ThisCopy.isSubtypeOf(ThatCopy, SymT);
}

bool ASaPType::
isSubtypeOf(const ASaPType &That, SymbolTable &SymT) const {
  if (! areUnqualQTsEqual(QT, That.QT) ) {
    /// Typechecking has passed so we assume that this->QT <= that->QT
    /// but we have to find follow the mapping and substitute Rpls....
    if (QT->isPointerType() && That.QT->isPointerType()) {
      // Recursively check
      ASaPType ThisCopy(*this), ThatCopy(That);
      ThisCopy.deref(); ThatCopy.deref();
      // both null or both non-null
      assert((ThisCopy.InRpl && ThatCopy.InRpl) ||
             (ThisCopy.InRpl==0 && ThatCopy.InRpl==0));

      return (ThisCopy.isSubtypeOf(ThatCopy, SymT)) &&
             ((ThisCopy.InRpl == 0 && ThatCopy.InRpl == 0) ||
              (ThisCopy.InRpl && ThatCopy.InRpl &&
               ThisCopy.InRpl->isIncludedIn(*ThatCopy.InRpl)));
    } else {
      assert(isDerivedFrom(QT, That.QT));
      ASaPType ThisCopy(*this);
      if (!ThisCopy.implicitCastToBase(That.QT, SymT))
        return false;
      else
        return ThisCopy.ArgV->isIncludedIn(*That.ArgV);
    }
  }
  /// Note that we're ignoring InRpl on purpose.
  assert(That.ArgV);
  return this->ArgV->isIncludedIn(*That.ArgV);
}

bool ASaPType::implicitCastToBase(QualType BaseQT, SymbolTable &SymT) {
  OSv2 << "DEBUG:: implicitCastToBase [" << this->toString() << "]\n";
  CXXRecordDecl *DerivedRD = QT->getAsCXXRecordDecl();
  CXXRecordDecl *BaseRD = BaseQT->getAsCXXRecordDecl();

  if (!BaseRD || !DerivedRD || !DerivedRD->hasDefinition() ||
      BaseRD->isInvalidDecl() || DerivedRD->isInvalidDecl())
    return false;

  CXXBasePaths *Paths = new CXXBasePaths();
  bool Result = DerivedRD->isDerivedFrom(BaseRD, *Paths);
  /// Count number of paths
  int c = 0;
  for (CXXBasePaths::paths_iterator I = Paths->begin(), E = Paths->end();
        I != E; ++I) {
    c++;
  }
  OSv2 << "DEBUG:: implicitCastToBase: #Paths = " << c << "\n";
  ///
  CXXBasePath &Path = Paths->front();
  OSv2 << "DEBUG:: implicitCastToBase: Paths.front.size()== "
       << Paths->front().size() << "\n";

  const ParameterVector *ParV = SymT.getParameterVector(DerivedRD);
  SubstitutionVector SubVec;
  SubVec.buildSubstitutionVector(ParV, ArgV);

  const InheritanceMapT *CurrMap = InheritanceMap;
  for (CXXBasePath::iterator I = Path.begin(), E = Path.end();
       I != E; ++I) {
    assert(CurrMap);
    //get Sub
    QualType DirectBaseQT = I->Base->getType();

    const RecordType *BaseRT = DirectBaseQT->getAs<RecordType>();
    assert(BaseRT);
    RecordDecl *BaseRD = BaseRT->getDecl();
    assert(BaseRD);
    IMapPair Pair = CurrMap->lookup(BaseRD);
    const SubstitutionVector *SV = Pair.second;

    SubVec.push_back_vec(SV);
    // prepare for next iter
    SymbolTableEntry *STE = Pair.first;
    assert(STE);
    CurrMap = STE->getInheritanceMap();
  }
  delete Paths;
  delete ArgV;
  // Apply substitutions in reverse order starting from the parameters
  // of the base class all the way to the substitution for the actual
  // variable of the derived type, then set ArgV
  const ParameterVector *PV = SymT.getParameterVector(BaseRD);
  assert(PV);
  RplVector *NewArgV = new RplVector(*PV);
  SubVec.reverseApplyTo(NewArgV);
  ArgV = NewArgV;
  QT = BaseQT;

  return Result;
}

void ASaPType::join(ASaPType *That) {
  if (!That)
    return;
  if (! areUnqualQTsEqual(this->QT, That->QT) ) {
    /// Typechecking has passed so we assume that this->QT <= that->QT
    /// but we have to find follow the mapping and substitute Rpls....
    /// TODO :)
    assert(false); // ...just fail
    return; // until we support inheritance this is good enough
  }
  if (this->InRpl)
    this->InRpl->join(That->InRpl);
  else if (That->InRpl)
    this->InRpl = new Rpl(*That->InRpl);
  // else this->InRpl = That->InRpl == null. Nothing to do.

  if (this->ArgV)
    this->ArgV->join(That->ArgV);
  else if (That->ArgV)
    this->ArgV = new RplVector(*That->ArgV);
  return;
}

void ASaPType::substitute(const SubstitutionVector *SubV) {
  if (SubV)
    SubV->applyTo(this);
}

void ASaPType::substitute(const Substitution *Sub) {
  if (!Sub)
    return;

  if (InRpl)
    InRpl->substitute(Sub);
  if (ArgV)
    ArgV->substitute(Sub);
}

} // end namespace clang
} // end namespace asap
