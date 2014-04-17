//=== EffectNIConstraint.cpp - Effect Non-Interference Constraint *- C++ -*===//
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

#include "Effect.h"
#include "EffectNIConstraint.h"

namespace clang {
namespace asap {

EffectNIConstraint::EffectNIConstraint(const EffectSummary *ES1,
                                       const EffectSummary *ES2)
                                       : First(ES1), Second(ES2) {}

}
}
