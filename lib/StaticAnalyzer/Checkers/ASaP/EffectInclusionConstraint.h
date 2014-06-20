//=== EffectInclusionConstraint.h - Effect Inclusion Constraint *- C++ -*===//
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

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_EFFECT_INCLUSION_CONSTRAINT_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_EFFECT_INCLUSION_CONSTRAINT_H


#include "ASaPFwdDecl.h"
#include "Effect.h"

namespace clang {
namespace asap {

//class that represents an effect inclusion constraint
class EffectInclusionConstraint {
  EffectVector *LHS;
  const EffectSummary *RHS;
  const FunctionDecl *Def;
  const Stmt *S;

 public:
  EffectInclusionConstraint(const EffectSummary *Rhs,
                            const FunctionDecl *Def,
                            const Stmt *S);
  void addEffect(Effect *Eff);
  EffectVector *getLHS()  {return LHS;}
  const EffectSummary *getRHS() const {return RHS;}
  const FunctionDecl *getDef() const {return Def;}
  const Stmt *getS() const {return S;}
  void print();
};

} // End namespace asap.
} // End namespace clang.
#endif
