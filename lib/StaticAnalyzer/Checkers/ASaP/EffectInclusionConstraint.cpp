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

EffectInclusionConstraint::EffectInclusionConstraint(const EffectSummary* Rhs,
                                                     const FunctionDecl* Def,
                                                     const Stmt* S)
                                                     : RHS(Rhs), Def(Def), S(S) {
  LHS = new EffectVector();
}

void EffectInclusionConstraint::addEffect(Effect* Eff){
  LHS->push_back(Eff);
}

void EffectInclusionConstraint::print(){
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

}
}
