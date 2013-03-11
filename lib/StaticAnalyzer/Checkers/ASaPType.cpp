//=== ASaPType.cpp - Safe Parallelism checker -----*- C++ -*---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This files defines the ASaPType class used by the Safe Parallelism
// checker, which tries to prove the safety of parallelism given region
// and effect annotations.
//
//===----------------------------------------------------------------===//

#include "Rpl.h"
#include "ASaPType.h"
#include "clang/AST/ASTContext.h"

using namespace clang;
using namespace clang::asap;

ASaPType::ASaPType(QualType QT, RplVector *ArgV, Rpl *InRpl,
                   bool Simple): QT(QT) {
  // 1. Set InRpl & ArgV.
  if (InRpl)
    this->InRpl = new Rpl(*InRpl);
  else
    this->InRpl = 0;
  if (ArgV)
    this->ArgV = new RplVector(*ArgV);
  else
    this->ArgV = 0;
  if (!Simple) {
    // 2. If QT is a function type, we're really interested in the
    // return type.
    if (QT->isFunctionType()) {
      const FunctionType *FT = dyn_cast<FunctionType>(QT.getTypePtr());
      QT = FT->getResultType();
    }
    // 3. Check if we might need to set InRpl.
    if (!this->InRpl) {
      // Figure out based on QT if we need an InRpl.
      if (QT->isScalarType() && !QT->isReferenceType()) {
        // Get it from the head of ArgV.
        assert(this->ArgV && this->ArgV->size() > 0);
        this->InRpl = this->ArgV->pop_front();
      }
    }
  } // End if (!Simple).
}

ASaPType::ASaPType(const ASaPType &T) : QT(T.QT) {
  this->QT = T.QT;
  if (T.InRpl)
    this->InRpl = new Rpl(*T.InRpl);
  else
    this->InRpl = 0;
  if (T.ArgV)
    this->ArgV = new RplVector(*T.ArgV);
  else
    this->ArgV = 0;
}

ASaPType::~ASaPType() {
  delete InRpl;
  delete ArgV;
}

bool ASaPType::isFunctionType() const { return QT->isFunctionType(); }

int ASaPType::getArgVSize() const { return ArgV->size(); }

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

ASaPType *ASaPType::getReturnType() const {
  if (QT->isFunctionType()) {
    const FunctionType *FT = dyn_cast<FunctionType>(QT.getTypePtr());
    QualType ResultQT = FT->getResultType();
    return new ASaPType(ResultQT, ArgV, InRpl);
  } else
    return 0;
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
      InRpl = ArgV->deref();
    else
      InRpl = 0;
    DerefNum--;
  }
}

void ASaPType::addrOf(QualType RefQT) {
  assert(RefQT->isPointerType() || RefQT->isReferenceType());
  assert(this->QT == RefQT->getPointeeType());
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

bool ASaPType::isAssignableTo(const ASaPType &That, bool IsInit) const {
  OSv2 << "DEBUG:: isAssignable\n";
  if (this->isSubtypeOf(That))
    return true;
  // else check if That is a reference
  if (That.QT->isReferenceType()) {
    OSv2 << "DEBUG:: isAssignable (LHS is a reference)[IsInit=" << IsInit
    << "]\n";
    OSv2 << "LHS:" << That.toString() << "\n";
    OSv2 << "RHS:" << this->toString() << "\n";
    if (IsInit) {
      // TODO: support this->QT isSubtypeOf QT->getPointeeType.
      if (That.QT->getPointeeType() == this->QT) {
        ASaPType ThisRef(*this);
        ThisRef.addrOf(That.QT);
        return ThisRef.isSubtypeOf(That);
      }
    } else {
      ASaPType ThatDeref(That);
      ThatDeref.deref();
      return this->isSubtypeOf(ThatDeref);
    }
  }
  return false; // TODO case for references
}

bool ASaPType::isSubtypeOf(const ASaPType &That) const { return *this <= That; }

bool ASaPType::operator <= (const ASaPType &That) const {
  if (this->QT!=That.QT) {
    /// Typechecking has passed so we assume that this->QT <= that->QT
    /// but we have to find follow the mapping and substitute Rpls....
    /// TODO :)
    OSv2 << "DEBUG:: Failing ASaP::subtype because QT != QT'\n";
    return false; // until we support inheritance this is good enough.
  }
  assert(this->QT == That.QT);
  /// Note that we're ignoring InRpl on purpose.
  assert(That.ArgV);
  return this->ArgV->isIncludedIn(*That.ArgV);
}

void ASaPType::join(ASaPType *That) {
  if (!That)
    return;
  if (this->QT!=That->QT) {
    /// Typechecking has passed so we assume that this->QT <= that->QT
    /// but we have to find follow the mapping and substitute Rpls....
    /// TODO :)
    assert(false); // ...just fail
    return; // until we support inheritance this is good enough
  }
  assert(this->QT == That->QT);
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

void ASaPType::substitute(const SubstitutionVector &SubV) {
  for(SubstitutionVector::SubstitutionVecT::const_iterator
      I = SubV.begin(),
      E = SubV.end();
      I != E; ++I) {
    Substitution &S = *(*I);
    substitute(S);
  }
}

void ASaPType::substitute(Substitution &S) {
  const RplElement *FromEl = S.getFrom();
  const Rpl *ToRpl = S.getTo();
  assert(FromEl);
  assert(ToRpl);
  if (InRpl)
    InRpl->substitute(*FromEl, *ToRpl);
  if (ArgV)
    ArgV->substitute(*FromEl, *ToRpl);
}



