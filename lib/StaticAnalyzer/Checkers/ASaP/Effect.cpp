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
#include <fstream>

#include "Effect.h"
#include "ASaPUtil.h"
#include "Rpl.h"
#include "ASaPSymbolTable.h"
#include "Substitution.h"

namespace clang {
namespace asap {

Effect::Effect(EffectKind EK, const Rpl *R, const Attr *A)
  : Kind(EK), Attribute(A), Exp(0), SubV(0), FunD(0) {
  this->R = (R) ? R->clone() : 0;
}

Effect::Effect(EffectKind EK, const Rpl *R,  const Expr *E)
  : Kind(EK), Exp(E), SubV(0), FunD(0) {
  this->R = (R) ? R->clone() : 0;
}


Effect::Effect(const Effect &E)
  : Kind(E.Kind), Attribute(E.Attribute), Exp(E.Exp), FunD(E.FunD) {
  R = (E.R) ? E.R->clone() : 0;
  SubV = new SubstitutionVector();
  SubV->push_back_vec(E.SubV);
}

Effect::Effect(EffectKind EK, const Expr *E,
               const FunctionDecl *FunD,
               const SubstitutionVector *SV)
              : Kind(EK), R(0), Exp(E), FunD(FunD) {
  SubV = new SubstitutionVector();
  SubV->push_back_vec(SV);
}

Effect::~Effect() {
  delete R;
}

void Effect::substitute(const Substitution *S) {
  if (!S)
    return; // Nothing to do

  if (Kind == EK_InvocEffect) {
    SubstitutionSet SubS;
    SubS.insert(S);
    SubV->push_back(SubS);
  } else if (R)
    R->substitute(S);
}

void Effect::substitute(const SubstitutionSet *SubS) {
  if (!SubS)
    return; // Nothing to do
  if (Kind == EK_InvocEffect)
    SubV->push_back(SubS);
  else if (R)
    R->substitute(SubS);
}

void Effect::substitute(const SubstitutionVector *S) {
  if (!S)
    return; // Nothing to do
  if (Kind == EK_InvocEffect)
    SubV->push_back_vec(S);
  else if (R)
    S->applyTo(R);

}

Trivalent Effect::isSubEffectOf(const Effect &That) const {
  Trivalent Result;
  if (isNoEffect()) {
    Result = RK_TRUE;
  } else if (isSubEffectKindOf(That)) {
    Result = R->isIncludedIn(*(That.R));
  } else {
    Result = RK_FALSE;
  }
  *OSv2  << "DEBUG:: ~~~isSubEffect(" << this->toString() << ", "
    << That.toString() << ")="
    << (Result==RK_TRUE ? "true" : "false-or-dunno") << "\n";
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

Trivalent Effect::isNonInterfering(const Effect &That) const {
  switch (Kind) {
  case EK_NoEffect:
    return RK_TRUE;
    break;
  case EK_ReadsEffect:
  case EK_AtomicReadsEffect:
    switch (That.Kind) {
    case EK_NoEffect:
    case EK_ReadsEffect:
    case EK_AtomicReadsEffect:
      return RK_TRUE;
      break;
    case EK_AtomicWritesEffect:
    case EK_WritesEffect:
      assert(R && "Internal ERROR: missing Rpl in non-pure Effect");
      assert(That.R && "Internal ERROR: missing Rpl in non-pure Effect");
      return R->isDisjoint(*That.R);
    case EK_InvocEffect:
      return That.isNonInterfering(*this);
    }
    break;
  case EK_WritesEffect:
  case EK_AtomicWritesEffect:
    if (That.Kind == EK_NoEffect) {
      return RK_TRUE;
    } else {
      assert(R && "Internal ERROR: missing Rpl in non-pure Effect");
      // FIXME That.R might be null if it is an invocation effect
      assert(That.R && "Internal ERROR: missing Rpl in non-pure Effect");
      return R->isDisjoint(*That.R);
    }
  case EK_InvocEffect:
    if (That.Kind == EK_NoEffect) {
      return RK_TRUE;
    } else {
      return isInvokeNonInterfering(That);
    }
  } // end switch(Kind)
}

Trivalent Effect::isInvokeNonInterfering(const Effect &That) const {
  assert(EK_InvocEffect && "Internal Error: isInvokeNonInterfering "
                           "called on non invoke effect");
  const EffectSummary *ES = SymbolTable::Table->getEffectSummary(FunD);
  assert(ES && "Internal Error: invoke effect declaration without effect summary");
  if (const ConcreteEffectSummary *CES = dyn_cast<ConcreteEffectSummary>(ES)) {
    ConcreteEffectSummary CESTmp(*CES);
    CESTmp.substitute(SubV); // apply substitutions
    return CESTmp.isNonInterfering(&That);
  } else {
    assert(isa<VarEffectSummary>(ES) && "Internal Error: unexpected kind of effect summary");
    return RK_DUNNO;
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
  if (Kind == EK_InvocEffect) {
    OS << ": " << FunD->getNameAsString()
       << "[" << SubV->toString() << "]";
  }
}

std::string Effect::toString() const {
  std::string SBuf;
  llvm::raw_string_ostream OS(SBuf);
  print(OS);
  return std::string(OS.str());
}

term_t Effect::getPLTerm() const {
  term_t Result = PL_new_term_ref();
  functor_t EffectFunctor;
  int Res = 0;
  switch(Kind) {
    case EK_NoEffect:
      Res = PL_put_atom_chars(Result, PL_NoEffect.c_str());
      assert(Res && "Failed to create Prolog term for 'no_effect'");
      break;
    case EK_ReadsEffect:
      assert(R && "Reads effect missing Rpl object");
      EffectFunctor =
        PL_new_functor(PL_new_atom(PL_ReadsEffect.c_str()), 1);
      Res = PL_cons_functor(Result, EffectFunctor, R->getPLTerm());
      assert(Res && "Failed to create Prolog term for 'reads' effect");
      break;
    case EK_WritesEffect:
      assert(R && "Writes effect missing Rpl object");
      EffectFunctor =
        PL_new_functor(PL_new_atom(PL_WritesEffect.c_str()), 1);
      Res = PL_cons_functor(Result, EffectFunctor, R->getPLTerm());
      assert(Res && "Failed to create Prolog term for 'writes' effect");
      break;
    case EK_AtomicReadsEffect:
      assert(R && "Atomic-Reads effect missing Rpl object");
      EffectFunctor =
        PL_new_functor(PL_new_atom(PL_AtomicReadsEffect.c_str()), 1);
      Res = PL_cons_functor(Result, EffectFunctor, R->getPLTerm());
      assert(Res && "Failed to create Prolog term for 'atomic_reads' effect");
      break;
    case EK_AtomicWritesEffect:
      assert(R && "Atomic-Writes effect missing Rpl object");
      EffectFunctor =
        PL_new_functor(PL_new_atom(PL_AtomicWritesEffect.c_str()), 1);
      Res = PL_cons_functor(Result, EffectFunctor, R->getPLTerm());
      assert(Res && "Failed to create Prolog term for 'atomic-writes' effect");
      break;
    case EK_InvocEffect:
      EffectFunctor = PL_new_functor(PL_new_atom(PL_InvokesEffect.c_str()), 2);
      term_t CalleeName = PL_new_term_ref();
      PL_put_atom_chars(CalleeName, SymbolTable::Table->getPrologName(FunD).data());
      Res = PL_cons_functor(Result, EffectFunctor, CalleeName, SubV->getPLTerm());
      assert(Res && "Failed to create Prolog term for 'invokes' effect");
      break;
  }
  return Result;
}

VarRplSetT *Effect::collectRplVars() const {
  VarRplSetT *Result = 0;
  if (R)
    Result = R->collectRplVars();
  if (SubV) {
    VarRplSetT *SubRVs = SubV->collectRplVars();
    Result = mergeRVSets(Result, SubRVs);
  }
  return Result;
}

VarEffectSummarySetT *Effect::collectEffectSummaryVars() const {
  VarEffectSummarySetT *Result = 0;
  if (isCompound()) {
    const EffectSummary *ES = SymbolTable::Table->getEffectSummary(FunD);
    assert(ES && "Internal Error: invoke effect declaration without effect summary");
    Result = ES->collectEffectSummaryVars();
  }
  return Result;
}

//////////////////////////////////////////////////////////////////////////
// EffectVector

void EffectVector::substitute(const Substitution *S) {
  if (!S)
    return; // Nothing to do.
  for (VectorT::const_iterator
          I = begin(),
          E = end();
       I != E; ++I) {
    Effect *Eff = *I;
    Eff->substitute(S);
  }
}

void EffectVector::substitute(const SubstitutionSet *SubS) {
  if (!SubS)
    return; // Nothing to do.
  for (VectorT::const_iterator
          I = begin(),
          E = end();
       I != E; ++I) {
    Effect *Eff = *I;
    Eff->substitute(SubS);
    }
}

void EffectVector::substitute(const SubstitutionVector *SubV) {
  if (!SubV)
    return; // Nothing to do.
  for (VectorT::const_iterator
          I = begin(),
          E = end();
       I != E; ++I) {
    Effect *Eff = *I;
    Eff->substitute(SubV);
    }
}

void EffectVector::makeMinimal() {
  VectorT::iterator I = begin(); // not a const iterator
  while (I != end()) { // end is not loop invariant
    bool found = false;
    for (VectorT::iterator J = begin(); J != end(); ++J) {
      if (I != J && (*I)->isSubEffectOf(**J) == RK_TRUE) {
        found = true;
        break;
      }
    }
    if (found) {
      //bool Success = take(*I);
      //assert(Success && "Internal Error: failed to minimize EffectVector");
      I = erase(I);
      //I = begin();
    } else {
      ++I;
    }
  }
}

void EffectVector::addEffects(const ConcreteEffectSummary &ES) {
  for(ConcreteEffectSummary::SetT::const_iterator I = ES.begin(), E = ES.end();
        I != E; ++ I) {
    push_back(*I);
  }
}

VarRplSetT *EffectVector::collectRplVars() const {
  VarRplSetT *Result = new VarRplSetT;
  for (VectorT::const_iterator I = begin(), E = end(); I != E; ++I) {
       VarRplSetT *RSet = (*I)->collectRplVars();
       Result = mergeRVSets(Result, RSet);
  }
  return Result;
}

VarEffectSummarySetT *EffectVector::collectEffectSummaryVars() const {
  VarEffectSummarySetT *Result = new VarEffectSummarySetT ;
  for (VectorT::const_iterator I = begin(), E = end(); I != E; ++I) {
       VarEffectSummarySetT *RSet = (*I)->collectEffectSummaryVars();
       Result = mergeESVSets(Result, RSet);
  }
  return Result;
}
//////////////////////////////////////////////////////////////////////////
// EffectSummary

std::string EffectSummary::toString() const {
  std::string SBuf;
  llvm::raw_string_ostream OS(SBuf);
  print(OS);
  return std::string(OS.str());
}

//ConcreteEffectSummary
Trivalent ConcreteEffectSummary::covers(const Effect *Eff) const {
  assert(Eff);
  Trivalent Result = RK_FALSE;
  if (!Eff->isCompound()) {
    if (Eff->isSubEffectOf(*SymbolTable::WritesLocal) == RK_TRUE)
      return RK_TRUE;
    if (Eff->isNoEffect())
      return RK_TRUE;
    // if the Eff pointer is included in the set, return it
    if (count(const_cast<Effect*>(Eff))) {
      return RK_TRUE;
    }

    SetT::const_iterator I = begin(), E = end();
    for(; I != E; ++I) {
      Trivalent Tmp = Eff->isSubEffectOf(*(*I));
      if (Tmp == RK_TRUE) {
        return RK_TRUE;
      } else if (Tmp == RK_DUNNO) {
        Result = RK_DUNNO;
      }
    }
    return Result;
  } else { // EFf is a compound effect (i.e. an invocation)
    const FunctionDecl *FunD = Eff->getDecl();
    const EffectSummary *ES = SymbolTable::Table->getEffectSummary(FunD);
    SubstitutionVector *SubV = Eff->getSubV();
    assert(SubV && "Internal Error: unexpected null-ptr");

    if (!ES)
      return RK_TRUE;
    if (isa<VarEffectSummary>(ES))
      return RK_DUNNO;

    const ConcreteEffectSummary *CES =
                  dyn_cast<ConcreteEffectSummary>(ES);
    assert(CES && "Expected either Var or Concrete Summary");
    ConcreteEffectSummary FunEffects(*CES);
    FunEffects.substitute(SubV);
    return covers(&FunEffects);
  } // end if Eff is a compound effect
}


Trivalent ConcreteEffectSummary::covers(const EffectSummary *Sum) const {
  if (!Sum)
    return RK_TRUE;
  if (isa<VarEffectSummary>(Sum))
    return RK_DUNNO;
  const ConcreteEffectSummary *CES = dyn_cast<ConcreteEffectSummary>(Sum);
  assert(CES && "Expected Sum would either be Var or Concrete");

  Trivalent Result = RK_TRUE;
  SetT::const_iterator I = CES->begin(), E = CES->end();
  for(; I != E; ++I) {
    Trivalent Tmp = this->covers(*I);
    if (Tmp == RK_FALSE) {
      return RK_FALSE;
    } else if (Tmp == RK_DUNNO) {
      Result = RK_DUNNO;
    }
  }
  return Result;
}


Trivalent ConcreteEffectSummary::isNonInterfering(const Effect *Eff) const {
  if (!Eff || Eff->isNoEffect())
    return RK_TRUE;
  Trivalent Result = RK_TRUE;
  SetT::const_iterator I = begin(), E = end();
  for(; I != E; ++I) {
    Trivalent TmpRes = Eff->isNonInterfering(*(*I));
    if (TmpRes == RK_FALSE)
      return RK_FALSE;
    else if (TmpRes == RK_DUNNO)
      Result = RK_DUNNO;
  }
  return Result;

}

Trivalent ConcreteEffectSummary::
isNonInterfering(const EffectSummary *Sum) const {
  if (!Sum)
    return RK_TRUE;
  if (isa<VarEffectSummary>(Sum))
    return RK_DUNNO;
  const ConcreteEffectSummary *CES = dyn_cast<ConcreteEffectSummary>(Sum);
  assert(CES && "Expected Sum would either be Var or Concrete");
  Trivalent Result = RK_TRUE;
  SetT::const_iterator I = CES->begin(), E = CES->end();
  for(; I != E; ++I) {
    Trivalent TmpRes = this->isNonInterfering(*I);
    if (TmpRes == RK_FALSE)
      return RK_FALSE;
    else if (TmpRes == RK_DUNNO)
      Result = RK_DUNNO;
  }
  return Result;
}

void ConcreteEffectSummary::makeMinimal(EffectCoverageVector &ECV) {
  SetT::iterator I = begin(); // not a const iterator
  while (I != end()) { // EffectSum.end() is not loop invariant
    bool found = false;
    for (SetT::iterator J = begin(); J != end(); ++J) {
      if (I != J && (*I)->isSubEffectOf(*(*J)) == RK_TRUE) {
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

void ConcreteEffectSummary::substitute(const Substitution *Sub) {
  if (!Sub || size() <= 0)
    return;
  for(SetT::iterator I = begin(), E = end(); I != E; ++I) {
    Effect *Eff = *I;
    Eff->substitute(Sub);
  }
}

void ConcreteEffectSummary::substitute(const SubstitutionSet *SubS) {
  if (!SubS || size() <= 0)
    return;
  for(SetT::iterator I = begin(), E = end(); I != E; ++I) {
    Effect *Eff = *I;
    Eff->substitute(SubS);
  }
}

void ConcreteEffectSummary::substitute(const SubstitutionVector *SubV) {
  if (!SubV || size()<=0)
    return;
  llvm::raw_ostream &OS = *SymbolTable::VB.OS;
  OS << "before iterating\n";
  for(SetT::iterator I = begin(), E = end(); I != E; ++I) {
    OS << "before dereference\n";
    Effect *Eff = *I;
    OS << "before substituting\n";
    Eff->substitute(SubV);
  }
  OS << "after iterating\n";
}

term_t ConcreteEffectSummary::getPLTerm() const {
  int Res = 0;
  term_t EffectSumT = PL_new_term_ref();
  functor_t EffectSumF = PL_new_functor(PL_new_atom(PL_EffectSummary.c_str()), 2);
  term_t SimpleL = buildPLEmptyList();
  term_t CompoundL = buildPLEmptyList();

  for(SetT::iterator I = begin(), E = end(); I != E; ++I) {
    Effect *Eff = *I;
    term_t Term = Eff->getPLTerm();
    if (Eff->isCompound()) {
      Res = PL_cons_list(CompoundL, Term, CompoundL);
      assert(Res && "Failed to add Compound Effect to Prolog list term");
    } else {
      Res = PL_cons_list(SimpleL, Term, SimpleL);
      assert(Res && "Failed to add Simple Effect to Prolog list term");
    }
  }

  Res = PL_cons_functor(EffectSumT, EffectSumF, SimpleL, CompoundL);
  assert(Res && "Failed to create 'effect_summary' Prolog term");
  return EffectSumT;
}

VarRplSetT *ConcreteEffectSummary::collectRplVars() const {
  VarRplSetT *Result = new VarRplSetT;
  for(SetT::iterator I = begin(), E = end(); I != E; ++I) {
    VarRplSetT *RSet = (*I)->collectRplVars();
    Result = mergeRVSets(Result, RSet);
  }
  return Result;
}

VarEffectSummarySetT *ConcreteEffectSummary::collectEffectSummaryVars() const {
  VarEffectSummarySetT *Result = new VarEffectSummarySetT ;
  for(SetT::iterator I = begin(), E = end(); I != E; ++I) {
       VarEffectSummarySetT *RSet = (*I)->collectEffectSummaryVars();
       Result = mergeESVSets(Result, RSet);
  }
  return Result;
}

//VarEffectSummary
void VarEffectSummary::print(raw_ostream &OS) const {
  OS << ID << "(Effect Summary Variable)";
}

term_t VarEffectSummary::getIDPLTerm() const {
  term_t IDTerm = PL_new_term_ref();
  PL_put_atom_chars(IDTerm, ID.data());
  return IDTerm;
}

term_t VarEffectSummary::getPLTerm() const {
  term_t EffectSumT = PL_new_term_ref();
  functor_t EffectSumF = PL_new_functor(PL_new_atom(PL_EffectVar.c_str()), 1);
  int Res = PL_cons_functor(EffectSumT, EffectSumF, getIDPLTerm());
  assert(Res && "Failed to create 'effect_var' Prolog term");
  return EffectSumT;
}

void VarEffectSummary::emitGraphNode(std::ofstream &OutF) const {
  OutF << ID.data() << std::endl;
}

VarRplSetT *VarEffectSummary::collectRplVars() const {
  return new VarRplSetT;
}

VarEffectSummarySetT *VarEffectSummary::collectEffectSummaryVars() const {
  VarEffectSummarySetT *Result = new VarEffectSummarySetT;
  Result->insert(this);
  return Result;
}

} // end namespace clang
} // end namespace asap
