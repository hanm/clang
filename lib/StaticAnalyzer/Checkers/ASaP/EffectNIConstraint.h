//=== EffectNIConstraint.h - Effect Non-Interference Constraint *- C++ -*===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This file defines Effect Non-Interference Constraints used by the Effect
// Constraint Generation of the Safe Parallelism checker.
//
//===----------------------------------------------------------------===//

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_EFFECT_NI_CONSTRAINT_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_EFFECT_NI_CONSTRAINT_H


#include "ASaPFwdDecl.h"
#include "Effect.h"

namespace clang {
namespace asap {

//class that represents an effect inclusion constraint
class EffectNIConstraint {
  const EffectSummary *First;
  const EffectSummary *Second;

 public:
  EffectNIConstraint(const EffectSummary *ES1, const EffectSummary *ES2);
  const EffectSummary* getFirst() {return First;}
  const EffectSummary* getSecond() {return Second;}
};

} // End namespace asap.
} // End namespace clang.
#endif


