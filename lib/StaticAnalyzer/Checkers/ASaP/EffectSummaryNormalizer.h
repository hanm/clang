//=== EffectSummaryNormalizer.h - Safe Parallelism checker *- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This file defines the Effect Summary Normalizer pass of the Safe
// Parallelism checker, which tries to prove the safety of parallelism
// given region and effect annotations.
//
// This pass checks that the effect summary of canonical declarations
// cover the effect summaries of the redeclarations.
//
//===----------------------------------------------------------------===//


#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_EFFECT_SUMMARY_NORMALIZER_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_EFFECT_SUMMARY_NORMALIZER_H

#include "clang/AST/RecursiveASTVisitor.h"

#include "ASaPFwdDecl.h"


namespace clang {
namespace asap {

class EffectSummaryNormalizerTraverser :
  public RecursiveASTVisitor<EffectSummaryNormalizerTraverser> {

  BugReporter &BR;
  ASTContext &Ctx;
  raw_ostream &OS;
  SymbolTable &SymT;

  bool FatalError;

  void emitCanonicalDeclHasSmallerEffectSummary(const Decl *D,
                                                const StringRef S);

public:
  typedef RecursiveASTVisitor<EffectSummaryNormalizerTraverser> BaseClass;

  explicit EffectSummaryNormalizerTraverser ();
  inline bool encounteredFatalError() { return FatalError; }

  bool shouldVisitTemplateInstantiations() const { return true; }
  bool shouldVisitImplicitCode() const { return true; }
  bool shouldWalkTypesOfTypeLocs() const { return true; }

  bool VisitFunctionDecl(FunctionDecl *D);
}; // End class EffectSummaryNormalizerTraverser
} // End namespace asap.
} // End namespace clang.

#endif
