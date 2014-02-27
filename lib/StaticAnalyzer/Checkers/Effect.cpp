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
  Exp=NULL;
  decl=NULL;
  SubV=NULL;
}

Effect::Effect(EffectKind EK, const Rpl* R,  const Expr* E)
  : Kind(EK), Exp(E) {
  this->R = (R) ? new Rpl(*R) : 0;
  decl=NULL;
  SubV=NULL;
}


Effect::Effect(const Effect &E)
  : Kind(E.Kind), Attribute(E.Attribute), Exp(E.Exp), decl(E.decl) { 
  R = (E.R) ? new Rpl(*E.R) : 0; 
  SubV=new SubstitutionVector();
  SubV->push_back_vec(E.SubV); 
}

Effect::Effect(EffectKind EK, const Expr* E, FunctionDecl* FunD, const
	       SubstitutionVector* SV) : Kind(EK), Exp(E), decl(FunD) {
  R=NULL;
  SubV=new SubstitutionVector();
  SubV->push_back_vec(SV);
}

Effect::~Effect() {
  delete R;
}

void Effect::substitute(const Substitution *S) {
  if(Kind == EK_InvocEffect && S){
    SubV->push_back(S);
  }
  else if (S && R)
    S->applyTo(R);
}

void Effect::substitute(const SubstitutionVector *S) {
  if(Kind == EK_InvocEffect && S){
    SubV->push_back_vec(S);
  }
  else if (S && R)
    S->applyTo(R);
}

bool Effect::isSubEffectOf(const Effect &That) const {
  bool Result;
  Result= (isNoEffect() || (isSubEffectKindOf(That) &&
                 R->isIncludedIn(*(That.R)))); 
  OSv2  << "DEBUG:: ~~~isSubEffect(" << this->toString() << ", "
    << That.toString() << ")=" << (Result ? "true" : "false") << "\n";
  return Result;
}

bool Effect::isSubEffectKindOf(const Effect &E) const {
  if (Kind == EK_NoEffect) 
    return true; // optimization

  if (Kind == EK_InvocEffect || E.Kind == EK_InvocEffect)
    return false;

  bool Result = false;
  if (!E.isAtomic() || this->isAtomic()) {
    /// if e.isAtomic ==> this->isAtomic [[else return false]]
    switch(E.getEffectKind()) {
    case EK_InvocEffect:
      Result = false;
      break;
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

bool Effect::isNonInterfering(const Effect &That) const {
  switch (Kind) {
  case EK_NoEffect:
    return true;
    break;
  case EK_ReadsEffect:
  case EK_AtomicReadsEffect:
    switch (That.Kind) {
    case EK_NoEffect:
    case EK_ReadsEffect:
    case EK_AtomicReadsEffect:
      return true;
      break;
    case EK_AtomicWritesEffect:
    case EK_WritesEffect:
      assert(R && "Internal ERROR: missing Rpl in non-pure Effect");
      assert(That.R && "Internal ERROR: missing Rpl in non-pure Effect");
      return R->isDisjoint(*That.R);
    case EK_InvocEffect:
      // TODO
      return false;
    }
    break;
  case EK_WritesEffect:
  case EK_AtomicWritesEffect:
    if (That.Kind == EK_NoEffect)
      return true;
    else {
      assert(R && "Internal ERROR: missing Rpl in non-pure Effect");
      // FIXME That.R might be null if it is an invocation effect
      assert(That.R && "Internal ERROR: missing Rpl in non-pure Effect");
      return R->isDisjoint(*That.R);
    }
  case EK_InvocEffect:
    if (That.Kind == EK_NoEffect)
      return true;
    else {
      // TODO
      return false;
    }
  }
}

bool Effect::printEffectKind(raw_ostream &OS) const {
  bool HasRpl = true;
  switch(Kind) {
    case EK_NoEffect: OS << "Pure Effect"; HasRpl = false; break;
    case EK_ReadsEffect: OS << "Reads Effect"; break;
    case EK_WritesEffect: OS << "Writes Effect"; break;
    case EK_AtomicReadsEffect: OS << "Atomic Reads Effect"; break;
    case EK_AtomicWritesEffect: OS << "Atomic Writes Effect"; break;
    case EK_InvocEffect: OS << "Invocation Effect"; HasRpl = false; break;
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
  else   if(this->Kind == EK_InvocEffect){
    SymbolTable* SymT=SymbolTable::Table;
    if(ES.covers(SymT->getEffectSummary(this->getDecl()->getCanonicalDecl())))
      return this;
    return 0;
    
  }

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


bool EffectSummary::isNonInterfering(const Effect *Eff) const {
  if (!Eff || Eff->isNoEffect())
    return true;

  SetT::const_iterator I = begin(), E = end();
  for(; I != E; ++I) {
    if (!Eff->isNonInterfering(*(*I)))
      return false;
  }
  return true;

}

bool EffectSummary::isNonInterfering(const EffectSummary *Sum) const {
  if (!Sum)
    return true;

  SetT::const_iterator I = Sum->begin(), E = Sum->end();
  for(; I != E; ++I) {
    if (!this->isNonInterfering(*I))
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

void EffectSummary::print(raw_ostream &OS,
                          std::string Separator,
                          bool PrintLastSeparator) const {
  SetT::const_iterator I = begin(), E = end(), Ip1 = begin();
  if (Ip1 != E)
    ++Ip1;

  for(; Ip1 != E; ++I, ++Ip1) {
    (*I)->print(OS);
    OS << Separator;
  }
  // print last element
  if (I != E) {
    (*I)->print(OS);
    if (PrintLastSeparator)
      OS << Separator;
  }
}

std::string EffectSummary::toString(std::string Separator,
                                    bool PrintLastSeparator) const {
  std::string SBuf;
  llvm::raw_string_ostream OS(SBuf);
  print(OS, Separator, PrintLastSeparator);
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
