//=== ASaPUtil.cpp - Safe Parallelism checker -----*- C++ -*---------===//
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
#include "ASaPUtil.h"

#include "clang/AST/Attr.h"

#include "clang/StaticAnalyzer/Core/BugReporter/BugReporter.h"
#include "llvm/Support/raw_ostream.h"

namespace clang {
namespace asap {

void helperEmitDeclarationWarning(ento::BugReporter &BR,
                                  const Decl *D,
                                  const StringRef Str,
                                  std::string BugName,
                                  bool AddQuotes) {
  std::string Description = "";
  if (AddQuotes)
    Description.append("'");
  Description.append(Str);
  if (AddQuotes)
    Description.append("': ");
  else
    Description.append(": ");
  Description.append(BugName);
  StringRef BugCategory = "Safe Parallelism";
  StringRef BugStr = Description;
  ento::PathDiagnosticLocation VDLoc(D->getLocation(), BR.getSourceManager());
  BR.EmitBasicReport(D, BugName, BugCategory,
                     BugStr, VDLoc, D->getSourceRange());
}

void helperEmitAttributeWarning(ento::BugReporter &BR,
                                const Decl *D,
                                const Attr *Attr,
                                const StringRef Str,
                                std::string BugName,
                                bool AddQuotes) {
  std::string Description = "";
  if (AddQuotes)
    Description.append("'");
  Description.append(Str);
  if (AddQuotes)
    Description.append("': ");
  else
    Description.append(": ");
  Description.append(BugName);
  StringRef BugCategory = "Safe Parallelism";
  StringRef BugStr = Description;
  ento::PathDiagnosticLocation VDLoc(Attr->getLocation(), BR.getSourceManager());
  BR.EmitBasicReport(D, BugName, BugCategory,
                     BugStr, VDLoc, Attr->getRange());
}



} // end namespace asap
} // end namespace clang
