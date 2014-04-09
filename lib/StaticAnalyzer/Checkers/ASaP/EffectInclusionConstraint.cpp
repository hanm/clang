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

namespace clang {
namespace asap {

EffectInclusionConstraint::EffectInclusionConstraint(const EffectSummary* Rhs)
                                                    : RHS(Rhs) {
  LHS = new EffectVector();
}

void EffectInclusionConstraint::addEffect(Effect* Eff){
  LHS->push_back(Eff);
}

}
}
