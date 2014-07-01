//=== EffectInclusionConstraint.cpp - Effect Inclusion Constraint *- C++ -*===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This file defines Effect Inlcusion Constraints used by the Effect
// Constraint Generation of the Safe Parallelism checker.
//
//===----------------------------------------------------------------===//

#include "Effect.h"
#include "EffectInclusionConstraint.h"
#include "ASaPSymbolTable.h"

namespace clang {
namespace asap {

EffectInclusionConstraint::EffectInclusionConstraint(const EffectSummary *Rhs,
                                                     const FunctionDecl *Def,
                                                     const Stmt *S)
                                                     : RHS(Rhs), Def(Def), S(S) {
  LHS = new EffectVector();
}

void EffectInclusionConstraint::addEffect(Effect *Eff) {
  LHS->push_back(Eff);
}

void EffectInclusionConstraint::print() {
  llvm::raw_ostream &OS = *SymbolTable::VB.OS;
  OS << "**** Effect Inclusion Constraint for:" << Def->getNameAsString() <<
 "*****"<< "\n";
  OS << "--------LHS-------\n";
  for (EffectVector::iterator It=LHS->begin(); It!=LHS->end(); ++It)
    OS << (*It)->toString() << "\n";
  OS << "--------RHS--------\n";
  OS << RHS->toString() << "\n";
  OS << "**********************************\n";
}

term_t EffectInclusionConstraint::getPLTerm() {
  assert(Def && "Internal Error: Trying to create prolog term with mising declaration");
  StringRef FName = SymbolTable::Table->getPrologName(Def);

  // Build prolog term effect_var(ev_xxx)
  std::string EVstr;
  llvm::raw_string_ostream EV(EVstr);
  EV << "ev_" << FName; // FName is of the form dXXX, which is a unique decl number
  term_t EVName = PL_new_term_ref();
  term_t EVTerm = PL_new_term_ref();
  functor_t EVFunctor = PL_new_functor(PL_new_atom(PL_EffectVar.c_str()), 1);
  PL_put_atom_chars(EVName, EV.str().c_str()); // variable name needs to be fresh
  int Res = PL_cons_functor(EVTerm, EVFunctor, EVName);
  assert(Res && "Failed to build 'effect_var' Prolog term");

  // Build ESI_ID term
  term_t ESIID = PL_new_term_ref();
  std::string ESIstr;
  llvm::raw_string_ostream ESI(ESIstr);
  ESI << "esi" << FName;
  PL_put_atom_chars(ESIID, ESI.str().c_str()); // fresh constraint name

  // Build Function name term
  term_t FNameTerm = PL_new_term_ref();
  PL_put_atom_chars(FNameTerm, FName.data());

  // Build LHS term (list of effects that must be inlcuded)
  term_t LHSTerm = LHS->getPLTerm();

  term_t ESITerm  = PL_new_term_ref();
  functor_t ESIFunctor =
    PL_new_functor(PL_new_atom(PL_ESIConstraint.c_str()), 4);
  Res = PL_cons_functor(ESITerm, ESIFunctor, ESIID, FNameTerm, LHSTerm, EVTerm);
  assert(Res && "Failed to build 'esi_constraint' Prolog term");

  //assertzTermProlog(ESITerm, "Failed to assert 'esi_constraint' to Prolog facts");
  return ESITerm;
}

}
}
