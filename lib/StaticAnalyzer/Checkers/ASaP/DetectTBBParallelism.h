//=== DetectTBBParallelism.h   - Safe Parallelism checker ----*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===--------------------------------------------------------------------===//
//
// This file defines pass that detects the declarations of the parallel_for
// TBB methods. This pass is part of the Safe Parallelism checker, which tries
// to prove the safety of parallelism given region and effect annotations.
//
//===--------------------------------------------------------------------===//

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_DETECT_TBB_PARALLELISM_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_DETECT_TBB_PARALLELISM_H

#include "clang/AST/RecursiveASTVisitor.h"

#include "ASaPFwdDecl.h"

namespace clang {
namespace asap {

class DetectTBBParallelism :
  public RecursiveASTVisitor<DetectTBBParallelism> {

  const CheckerBase *Checker;
  BugReporter &BR;
  ASTContext &Ctx;
  raw_ostream &OS;
  SymbolTable &SymT;

  bool FatalError;

  void emitUnexpectedTBBParallelFor(const FunctionDecl *D);

public:
  typedef RecursiveASTVisitor<DetectTBBParallelism> BaseClass;

  explicit DetectTBBParallelism ();
  //virtual ~DetectTBBParallelism ();
  inline bool encounteredFatalError() { return FatalError; }

  bool shouldVisitTemplateInstantiations() const { return true; }
  bool shouldVisitImplicitCode() const { return true; }
  bool shouldWalkTypesOfTypeLocs() const { return true; }

  bool VisitFunctionDecl(FunctionDecl *D);

}; // End class DetectTBBParallelism.
} // End namespace asap.
} // End namespace clang.

#endif
