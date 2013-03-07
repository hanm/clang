//=== Rpl.cpp - Safe Parallelism checker -----*- C++ -*--------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This files defines the Rpl and RplVector classes used by the Safe
// Parallelism checker, which tries to prove the safety of parallelism
// given region and effect annotations.
//
//===----------------------------------------------------------------===//

#include "Rpl.h"
#include "llvm/Support/Casting.h"

using namespace llvm;
using namespace clang::asap;

/// Static
std::pair<StringRef, StringRef> clang::asap::Rpl::splitRpl(StringRef &String) {
  size_t Idx = 0;
  do {
    Idx = String.find(RPL_SPLIT_CHARACTER, Idx);
    OSv2 << "Idx = " << Idx << ", size = " << String.size() << "\n";
    if (Idx == StringRef::npos)
      break;

  } while (Idx < String.size() - 2
    && String[Idx+1] == RPL_SPLIT_CHARACTER && Idx++ && Idx++);

  if (Idx == StringRef::npos)
    return std::pair<StringRef, StringRef>(String, "");
  else
    return std::pair<StringRef, StringRef>(String.slice(0,Idx),
    String.slice(Idx+1, String.size()));
}

namespace clang {
namespace asap {

const StarRplElement *STARRplElmt = new StarRplElement();
const SpecialRplElement *ROOTRplElmt = new SpecialRplElement("Root");
const SpecialRplElement *LOCALRplElmt = new SpecialRplElement("Local");
Effect *WritesLocal = 0;

const RplElement* getSpecialRplElement(const llvm::StringRef& s) {
  if (!s.compare(STARRplElmt->getName()))
    return STARRplElmt;
  else if (!s.compare(ROOTRplElmt->getName()))
    return ROOTRplElmt;
  else if (!s.compare(LOCALRplElmt->getName()))
    return LOCALRplElmt;
  else
    return 0;
}

bool isSpecialRplElement(const llvm::StringRef& s) {
  if (!s.compare("*"))
    return true;
  else
    return false;
}

bool isValidRegionName(const llvm::StringRef& s) {
  // false if it is one of the Special Rpl Elements
  // => it is not allowed to redeclare them
  if (isSpecialRplElement(s)) return false;

  // must start with [_a-zA-Z]
  const char c = s.front();
  if (c != '_' &&
    !( c >= 'a' && c <= 'z') &&
    !( c >= 'A' && c <= 'Z'))
    return false;
  // all remaining characters must be in [_a-zA-Z0-9]
  for (size_t i=0; i < s.size(); i++) {
    const char c = s[i];
    if (c != '_' &&
      !( c >= 'a' && c <= 'z') &&
      !( c >= 'A' && c <= 'Z') &&
      !( c >= '0' && c <= '9'))
      return false;
  }
  return true;
}
} // End namespace asap.
} // End namespace clang.

void Rpl::print(raw_ostream &OS) const {
  RplElementVectorTy::const_iterator I = RplElements.begin();
  RplElementVectorTy::const_iterator E = RplElements.end();
  for (; I < E-1; I++) {
    OS << (*I)->getName() << Rpl::RPL_SPLIT_CHARACTER;
  }
  // print last element
  if (I < E)
    OS << (*I)->getName();
}

std::string Rpl::toString() const {
  std::string SBuf;
  llvm::raw_string_ostream OS(SBuf);
  print(OS);
  return std::string(OS.str());
}

bool Rpl::isUnder(const Rpl& That) const {
  const CaptureRplElement *cap = dyn_cast<CaptureRplElement>(RplElements.front());
  if (cap) {
    cap->upperBound().isUnder(That);
  }

  RplRef* LHS = new RplRef(*this);
  RplRef* RHS = new RplRef(That);
  bool Result = LHS->isIncludedIn(*RHS);
  delete LHS; delete RHS;
  return Result;
}

bool Rpl::isIncludedIn(const Rpl& That) const {
  const CaptureRplElement *cap = dyn_cast<CaptureRplElement>(RplElements.front());
  if (cap) {
    cap->upperBound().isIncludedIn(That);
  }

  RplRef* LHS = new RplRef(*this);
  RplRef* RHS = new RplRef(That);
  bool Result = LHS->isIncludedIn(*RHS);
  delete LHS; delete RHS;
  OSv2 << "DEBUG:: ~~~~~ isIncludedIn[RPL](" << this->toString() << ", "
    << That.toString() << ")=" << (Result ? "true" : "false") << "\n";
  return Result;
}

void Rpl::substitute(const RplElement& FromEl, const Rpl& ToRpl) {
  os << "DEBUG:: before substitution(" << FromEl.getName() << "<-";
  ToRpl.print(os);
  os <<"): ";
  assert(RplElements.size()>0);
  print(os);
  os << "\n";
  /// A parameter is only allowed at the head of an Rpl
  RplElementVectorTy::iterator I = RplElements.begin();
  if (*(*I) == FromEl) {
    OSv2 << "DEBUG:: found '" << FromEl.getName()
      << "' replaced with '" ;
    ToRpl.print(OSv2);
    I = RplElements.erase(I);
    I = RplElements.insert(I, ToRpl.RplElements.begin(),
      ToRpl.RplElements.end());
    OSv2 << "' == '";
    print(OSv2);
    OSv2 << "'\n";
  }
  os << "DEBUG:: after substitution(" << FromEl.getName() << "<-";
  ToRpl.print(os);
  os << "): ";
  print(os);
  os << "\n";
}

inline void Rpl::appendRplTail(Rpl* That) {
  if (!That)
    return;
  if (That->length()>1)
    RplElements.append(That->length()-1, (*(That->RplElements.begin() + 1)));
}

Rpl *Rpl::upperBound() {
  if (isEmpty() || !isa<CaptureRplElement>(RplElements.front()))
    return this;
  // else
  Rpl* upperBound = & dyn_cast<CaptureRplElement>(RplElements.front())->upperBound();
  upperBound->appendRplTail(this);
  return upperBound;
}

void Rpl::join(Rpl* That) {
  if (!That)
    return;
  Rpl Result;
  // join from the left
  RplElementVectorTy::const_iterator
    ThisI = this->RplElements.begin(),
    ThatI = That->RplElements.begin(),
    ThisE = this->RplElements.end(),
    ThatE = That->RplElements.end();
  for ( ; ThisI != ThisE && ThatI != ThatE && (*ThisI == *ThatI);
    ++ThisI, ++ThatI) {
      Result.appendElement(*ThisI);
  }
  if (ThisI != ThisE) {
    // put a star in the middle and join from the right
    assert(ThatI != ThatE);
    Result.appendElement(STARRplElmt);
    Result.FullySpecified = false;
    RplElementVectorTy::const_reverse_iterator
      ThisI = this->RplElements.rbegin(),
      ThatI = That->RplElements.rbegin(),
      ThisE = this->RplElements.rend(),
      ThatE = That->RplElements.rend();
    int ElNum = 0;
    for(; ThisI != ThisE && ThatI != ThatE && (*ThisI == *ThatI);
      ++ThisI, ++ThatI, ++ElNum);
      if (ElNum > 0) {
        Result.RplElements.append(ElNum,
          *(RplElements.begin() + RplElements.size() - ElNum));
      }
  }
  // return
  this->RplElements = Result.RplElements;
  return;
}

Rpl *Rpl::capture() {
  if (this->isFullySpecified()) return this;
  else return new Rpl(*new CaptureRplElement(*this));
}

bool Effect::isSubEffectKindOf(const Effect &E) const {
  if (Kind == EK_NoEffect) return true; // optimization

  bool Result = false;
  if (!E.isAtomic() || this->isAtomic()) {
    /// if e.isAtomic ==> this->isAtomic [[else return false]]
    switch(E.getEffectKind()) {
    case EK_WritesEffect:
      if (Kind == EK_WritesEffect) Result = true;
      // intentional fall through (lack of 'break')
    case EK_AtomicWritesEffect:
      if (Kind == EK_AtomicWritesEffect) Result = true;
      // intentional fall through (lack of 'break')
    case EK_ReadsEffect:
      if (Kind == EK_ReadsEffect) Result = true;
      // intentional fall through (lack of 'break')
    case EK_AtomicReadsEffect:
      if (Kind == EK_AtomicReadsEffect) Result = true;
      // intentional fall through (lack of 'break')
    case EK_NoEffect:
      if (Kind == EK_NoEffect) Result = true;
    }
  }
  return Result;
}

bool Effect::printEffectKind(raw_ostream &OS) const {
  bool HasRpl = true;
  switch(Kind) {
    case EK_NoEffect: OS << "Pure Effect"; HasRpl = false; break;
    case EK_ReadsEffect: OS << "Reads Effect"; break;
    case EK_WritesEffect: OS << "Writes Effect"; break;
    case EK_AtomicReadsEffect: OS << "Atomic Reads Effect"; break;
    case EK_AtomicWritesEffect: OS << "Atomic Writes Effect"; break;
  }
  return HasRpl;
}

void Effect::print(raw_ostream &OS) const {
  bool HasRpl = printEffectKind(OS);
  if (HasRpl) {
    OS << " on ";
    assert(R && "NULL RPL in non-pure effect");
    R->print(OS);
  }
}

std::string Effect::toString() const {
  std::string SBuf;
  llvm::raw_string_ostream OS(SBuf);
  print(OS);
  return std::string(OS.str());
}

const Effect *EffectSummary::covers(const Effect *Eff) const {
  if (EffectSum.count(Eff))
    return Eff;

  EffectSummarySetT::const_iterator
    I = EffectSum.begin(),
    E = EffectSum.end();
  for(; I != E; ++I) {
    if (Eff->isSubEffectOf(*(*I)))
      return *I;
  }
  return 0;
}

void EffectSummary::makeMinimal(EffectCoverageVector &ECV) {
  EffectSummarySetT::iterator I = EffectSum.begin(); // not a const iterator
  while (I != EffectSum.end()) { // EffectSum.end() is not loop invariant
    bool found = false;
    for (EffectSummarySetT::iterator
         J = EffectSum.begin(); J != EffectSum.end(); ++J) {
      if (I != J && (*I)->isSubEffectOf(*(*J))) {
        //emitEffectCovered(D, *I, *J);
        ECV.push_back(new std::pair<const Effect*, const Effect*>(*I, *J));
        found = true;
        break;
      } // end if
    } // end inner for loop
    /// optimization: remove e from effect Summary
    if (found) {
      bool Success = EffectSum.erase(*I);
      assert(Success);
      I = EffectSum.begin();
    }
    else
      ++I;
  } // end while loop
}

void EffectSummary::print(raw_ostream &OS, char Separator) const {
  EffectSummarySetT::const_iterator
  I = EffectSum.begin(),
  E = EffectSum.end();
  for(; I != E; ++I) {
    (*I)->print(OS);
    OS << Separator;
  }
}

std::string EffectSummary::toString() const {
  std::string SBuf;
  llvm::raw_string_ostream OS(SBuf);
  print(OS);
  return std::string(OS.str());
}

const Effect *Effect::isCoveredBy(const EffectSummary &ES, const RplElement *LocalRplElement) {
  if (!WritesLocal)
    WritesLocal = new Effect(Effect::EK_WritesEffect, new Rpl(*LocalRplElement));
  bool Result = this->isSubEffectOf(*WritesLocal);
  if (Result)
    return WritesLocal;
  else
    return ES.covers(this);
}
