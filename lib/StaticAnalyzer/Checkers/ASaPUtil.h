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


namespace clang {

class Attr;
class Decl;
namespace ento {
  class BugReporter;
}

namespace asap {

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

} // end namespace asap
} // end namespace clang

#endif
