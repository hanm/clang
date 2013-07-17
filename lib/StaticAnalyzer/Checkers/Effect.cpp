//=== Effect.cpp - Safe Parallelism checker -----*- C++ -*--------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This file defines the Effect and EffectSummary classes used by the Safe
// Parallelism checker, which tries to prove the safety of parallelism
// given region and effect annotations.
//
//===----------------------------------------------------------------===//

#include "Effect.h"
#include "ASaPUtil.h"

#include "Rpl.h"
#include "ASaPSymbolTable.h"
#include "Substitution.h"

namespace clang {
namespace asap {

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

void Effect::substitute(const Substitution *S) {
  if (S && R)
    S->applyTo(R);
}

void Effect::substitute(const SubstitutionVector *S) {
  if (S && R)
    S->applyTo(R);
}

bool Effect::isSubEffectOf(const Effect &That) const {
  bool Result = (isNoEffect() ||
                 (isSubEffectKindOf(That) && R->isIncludedIn(*(That.R))));
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

const Effect *Effect::isCoveredBy(const EffectSummary &ES) {
  if (this->isSubEffectOf(*SymbolTable::WritesLocal))
    return SymbolTable::WritesLocal;
  else
    return ES.covers(this);
}


//////////////////////////////////////////////////////////////////////////
// EffectSummary

const Effect *EffectSummary::covers(const Effect *Eff) const {
  assert(Eff);
  if (Eff->isNoEffect())
    return Eff;

  // if the Eff pointer is included in the set, return it
  if (count(const_cast<Effect*>(Eff))) {
    return Eff;
  }

  SetT::const_iterator I = begin(), E = end();
  for(; I != E; ++I) {
    if (Eff->isSubEffectOf(*(*I)))
      return *I;
  }
  return 0;
}

bool EffectSummary::covers(const EffectSummary *Sum) const {
  if (!Sum)
    return true;

  SetT::const_iterator I = Sum->begin(), E = Sum->end();
  for(; I != E; ++I) {
    if (!this->covers(*I))
      return false;
  }
  return true;
}

void EffectSummary::makeMinimal(EffectCoverageVector &ECV) {
  SetT::iterator I = begin(); // not a const iterator
  while (I != end()) { // EffectSum.end() is not loop invariant
    bool found = false;
    for (SetT::iterator J = begin(); J != end(); ++J) {
      if (I != J && (*I)->isSubEffectOf(*(*J))) {
        //emitEffectCovered(D, *I, *J);
        ECV.push_back(new std::pair<const Effect*,
                                    const Effect*>(new Effect(*(*I)),
                                                   new Effect(*(*J))));
        found = true;
        break;
      } // end if
    } // end inner for loop
    /// optimization: remove e from effect Summary
    if (found) {
      bool Success = take(*I);
      assert(Success);
      I = begin();
    } else {
      ++I;
    }
  } // end while loop
}

void EffectSummary::print(raw_ostream &OS, char Separator) const {
  for(SetT::const_iterator I = begin(), E = end(); I != E; ++I) {
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

void EffectSummary::substitute(const Substitution *Sub) {
  if (!Sub)
    return;
  for(SetT::iterator I = begin(), E = end(); I != E; ++I) {
    Effect *Eff = *I;
    Eff->substitute(Sub);
  }
}

void EffectSummary::substitute(const SubstitutionVector *SubV) {
  if (!SubV)
    return;
  for(SetT::iterator I = begin(), E = end(); I != E; ++I) {
    Effect *Eff = *I;
    Eff->substitute(SubV);
  }
}

} // end namespace clang
} // end namespace asap
