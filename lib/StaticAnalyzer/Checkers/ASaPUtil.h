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

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"


namespace clang {

class Attr;
class Decl;
class Stmt;
class CXXConstructorDecl;
class ASTContext;
class AnalysisDeclContext;

namespace ento {
  class BugReporter;
}

namespace asap {
class ASaPType;

#define ASAP_DEBUG
#define ASAP_DEBUG_VERBOSE2

#ifdef ASAP_DEBUG
  static llvm::raw_ostream& os = llvm::errs();
#else
  static llvm::raw_ostream &os = llvm::nulls();
#endif

#ifdef ASAP_DEBUG_VERBOSE2
  static llvm::raw_ostream &OSv2 = llvm::errs();
#else
  static llvm::raw_ostream &OSv2 = llvm::nulls();
#endif

/// \brief Issues Warning: '<str>': <bugName> on Declaration.
void helperEmitDeclarationWarning(ento::BugReporter &BR,
                                  const Decl *D,
                                  const llvm::StringRef Str,
                                  std::string BugName,
                                  bool AddQuotes = true);

/// \brief Issues Warning: '<str>': <bugName> on Attribute.
void helperEmitAttributeWarning(ento::BugReporter &BR,
                                const Decl *D,
                                const Attr *A,
                                const llvm::StringRef Str,
                                std::string BugName,
                                bool AddQuotes = true);

void helperEmitStatementWarning(ento::BugReporter &BR,
                                AnalysisDeclContext *AC,
                                const Stmt *S,
                                const Decl *D,
                                const llvm::StringRef &Str,
                                std::string BugName);

void helperEmitInvalidAssignmentWarning(ento::BugReporter &BR,
                                        AnalysisDeclContext *AC,
                                        ASTContext &Ctx,
                                        const Stmt *S,
                                        const ASaPType *LHS,
                                        const ASaPType *RHS,
                                        llvm::StringRef BugName);

} // end namespace asap
} // end namespace clang

#endif
