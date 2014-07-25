//=== Constraints.cpp - Effect Inclusion Constraint *- C++ ---------*===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This file defines Constraints generated by the Safe Parallelism checker
// and emmitted to Prolog for solving.
//
//===----------------------------------------------------------------===//

#include "ASaPSymbolTable.h"
#include "Constraints.h"
#include "Effect.h"

namespace clang {
namespace asap {

//////////////////////////////////////////////////////////////////////////
//  Constraint

term_t Constraint::getIDPLTerm() const {
  term_t IDTerm = PL_new_term_ref();
  PL_put_atom_chars(IDTerm, ConstraintID.data());
  return IDTerm;
}

//////////////////////////////////////////////////////////////////////////
//  RplInclusionConstraint

term_t RplInclusionConstraint::getPLTerm() const {

  term_t RICTerm  = PL_new_term_ref();
  functor_t RICFunctor =
    PL_new_functor(PL_new_atom(PL_RIConstraint.c_str()), 3);
  int Res = PL_cons_functor(RICTerm, RICFunctor, getIDPLTerm(),
                            LHS->getPLTerm(), RHS->getPLTerm());
  assert(Res && "Failed to build 'esi_constraint' Prolog term");

  return RICTerm;
}

void RplInclusionConstraint::print(llvm::raw_ostream &OS) const {
  OS << "RplInclusionConstraint: "
     << LHS->toString() << " <=(Incl) "
     << RHS->toString();
}

RplInclusionConstraint::~RplInclusionConstraint() {
  delete LHS;
  delete RHS;
}
//////////////////////////////////////////////////////////////////////////
//  EffectInclusionConstraint

EffectInclusionConstraint::
EffectInclusionConstraint(StringRef ID,
                          const ConcreteEffectSummary *Lhs,
                          const EffectSummary *Rhs,
                          const FunctionDecl *Def,
                          const Stmt *S)
                         : Constraint(CK_EffectInclusion, ID),
                           RHS(Rhs), Def(Def), S(S) {
  LHS = new EffectVector();
  if (Lhs) {
    LHS->addEffects(*Lhs);
  }
}

EffectInclusionConstraint::~EffectInclusionConstraint() {
  delete LHS;
}

void EffectInclusionConstraint::addEffect(const Effect *Eff) {
  LHS->push_back(Eff);
}

void EffectInclusionConstraint::addEffects(const ConcreteEffectSummary &ES) {
  LHS->addEffects(ES);
}

term_t EffectInclusionConstraint::getPLTerm() const {
  assert(Def && "Internal Error: Trying to create prolog term with mising declaration");
  StringRef FName = SymbolTable::Table->getPrologName(Def);

  // Build ESI_ID term
  term_t ESIID = getIDPLTerm();

  // Build Function name term
  term_t FNameTerm = PL_new_term_ref();
  PL_put_atom_chars(FNameTerm, FName.data());

  // Build LHS term (list of effects that must be inlcuded)
  term_t LHSTerm = LHS->getPLTerm();

  // Build RHS term (usually an effect summary variable)
  term_t RHSTerm = RHS->getPLTerm();

  term_t ESITerm  = PL_new_term_ref();
  functor_t ESIFunctor =
    PL_new_functor(PL_new_atom(PL_ESIConstraint.c_str()), 4);
  int Res = PL_cons_functor(ESITerm, ESIFunctor, ESIID, FNameTerm, LHSTerm, RHSTerm);
  assert(Res && "Failed to build 'esi_constraint' Prolog term");

  return ESITerm;
}

void EffectInclusionConstraint::print(llvm::raw_ostream &OS) const {
  assert(LHS && "Unexpected null-pointer LHS in EffectInclusionConstraint");
  assert(RHS && "Unexpected null-pointer RHS in EffectInclusionConstraint");
  OS << "EffectInclusionConstraint: {";
  for (EffectVector::const_iterator I = LHS->begin(), E = LHS->end();
         I != E; ++I) {
    OS << (*I)->toString();
    if (I+1 != E)
      OS << ", ";
  }
  OS << "} <=(Incl) " << RHS->toString();
}

void EffectInclusionConstraint::makeMinimal() {
  if (LHS)
    LHS->makeMinimal();
}

//////////////////////////////////////////////////////////////////////////
//  EffectNIConstraint

EffectNIConstraint::
EffectNIConstraint( StringRef ID,
                    const EffectSummary &ES1,
                    const EffectSummary &ES2)
                  : Constraint(CK_EffectNonInterference, ID),
                    LHS(ES1.clone()),
                    RHS(ES2.clone()) {}

term_t EffectNIConstraint::getPLTerm() const {
  term_t ENITerm = PL_new_term_ref();
  functor_t ENIFunctor =
    PL_new_functor(PL_new_atom(PL_ENIConstraint.c_str()), 3);
  assert(LHS && "Unexpected null-pointer LHS");
  assert(RHS && "Unexpected null-pointer RHS");
  int Res = PL_cons_functor(ENITerm, ENIFunctor, getIDPLTerm(),
                            LHS->getPLTerm(), RHS->getPLTerm());
  assert(Res && "Failed to build 'esi_constraint' Prolog term");

  return ENITerm;

}

void EffectNIConstraint::print(llvm::raw_ostream &OS) const {
  assert(LHS && "Unexpected null-pointer LHS in EffectNIConstraint");
  assert(RHS && "Unexpected null-pointer RHS in EffectNIConstraint");
  OS << "EffectNonInterferenceConstraint: "
     << LHS->toString() << " # " << RHS->toString();
}

EffectNIConstraint::~EffectNIConstraint() {
  delete LHS;
  delete RHS;
}

} // end namespace asap
} // end namespace clang