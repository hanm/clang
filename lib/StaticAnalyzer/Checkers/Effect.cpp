//=== Effect.cpp - Safe Parallelism checker -----*- C++ -*--------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This files defines the Effect and EffectSummary classes used by the Safe
// Parallelism checker, which tries to prove the safety of parallelism
// given region and effect annotations.
//
//===----------------------------------------------------------------===//

#include "Effect.h"
#include "Rpl.h"

using namespace clang;
using namespace clang::asap;

void Substitution::applyTo(Rpl *R) const {
  if (FromEl && ToRpl) {
    assert(R);
    R->substitute(*FromEl, *ToRpl);
  }
}

void Substitution::print(llvm::raw_ostream &OS) const {
  llvm::StringRef FromName = "<MISSING>";
  llvm::StringRef ToName = "<MISSING>";
  if (FromEl)
    FromName = FromEl->getName();
  if (ToRpl)
    ToName = ToRpl->toString();
  OS << "[" << FromName << "<-" << ToName << "]";
}

std::string Substitution::toString() const {
  std::string SBuf;
  llvm::raw_string_ostream OS(SBuf);
  print(OS);
  return std::string(OS.str());
}

Effect::Effect(EffectKind EK, const Rpl* R, const Attr* A)
  : Kind(EK), Attribute(A) {
  this->R = (R) ? new Rpl(*R) : 0;
}

Effect::Effect(const Effect &E): Kind(E.Kind), Attribute(E.Attribute) {
  R = (E.R) ? new Rpl(*E.R) : 0;
}

Effect::~Effect() {
  delete R;
}

bool Effect::isSubEffectOf(const Effect &That) const {
  bool Result = (isNoEffect() || (isSubEffectKindOf(That) && R->isIncludedIn(*(That.R))));
  OSv2  << "DEBUG:: ~~~isSubEffect(" << this->toString() << ", "
    << That.toString() << ")=" << (Result ? "true" : "false") << "\n";
  return Result;
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

