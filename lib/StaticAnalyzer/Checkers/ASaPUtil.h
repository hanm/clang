//=== ASaPUtil.h - Safe Parallelism checker -----*- C++ -*-----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This files defines various utility methods used by the Safe Parallelism
// checker, which tries to prove the safety of parallelism given region
// and effect annotations.
//
//===----------------------------------------------------------------===//

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_UTIL_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_UTIL_H

#include "ASaPFwdDecl.h"

namespace clang {
namespace asap {

#define ASAP_DEBUG
#define ASAP_DEBUG_VERBOSE2
extern raw_ostream &os;
extern raw_ostream &OSv2;


struct VisitorBundle {
  BugReporter &BR;
  ASTContext &Ctx;
  AnalysisManager &Mgr;
  AnalysisDeclContext *AC;
  raw_ostream &OS;
  SymbolTable &SymT;
};

/// \brief Issues Warning: '<str>': <bugName> on Declaration.
void helperEmitDeclarationWarning(BugReporter &BR,
                                  const Decl *D,
                                  const StringRef &Str,
                                  const StringRef &BugName,
                                  bool AddQuotes = true);

/// \brief Issues Warning: '<str>': <bugName> on Attribute.
void helperEmitAttributeWarning(BugReporter &BR,
                                const Decl *D,
                                const Attr *A,
                                const StringRef &Str,
                                const StringRef &BugName,
                                bool AddQuotes = true);

void helperEmitStatementWarning(BugReporter &BR,
                                AnalysisDeclContext *AC,
                                const Stmt *S,
                                const Decl *D,
                                const StringRef &Str,
                                const StringRef &BugName);

void helperEmitInvalidAssignmentWarning(BugReporter &BR,
                                        AnalysisDeclContext *AC,
                                        ASTContext &Ctx,
                                        const Stmt *S,
                                        const ASaPType *LHS,
                                        const ASaPType *RHS,
                                        StringRef &BugName);

} // end namespace asap
} // end namespace clang

#endif
