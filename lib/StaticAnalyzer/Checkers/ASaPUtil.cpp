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
#include "ASaPType.h"

#include "clang/AST/Attr.h"

#include "clang/StaticAnalyzer/Core/BugReporter/BugReporter.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"

#include "llvm/Support/raw_ostream.h"

namespace clang {

using ento::PathDiagnosticLocation;
using ento::BugReporter;

namespace asap {

StringRef BugCategory = "Safe Parallelism";

void helperEmitDeclarationWarning(BugReporter &BR,
                                  const Decl *D,
                                  const StringRef Str,
                                  std::string BugName,
                                  bool AddQuotes) {
  std::string Description;
  llvm::raw_string_ostream DescrOS(Description);
  DescrOS << (AddQuotes ? "'" : "") << Str
          << (AddQuotes ? "'" : "") << ": " << BugName;

  StringRef BugStr = DescrOS.str();
  PathDiagnosticLocation VDLoc(D->getLocation(), BR.getSourceManager());
  BR.EmitBasicReport(D, BugName, BugCategory,
                     BugStr, VDLoc, D->getSourceRange());
}

void helperEmitAttributeWarning(BugReporter &BR,
                                const Decl *D,
                                const Attr *Attr,
                                const StringRef Str,
                                std::string BugName,
                                bool AddQuotes) {
  std::string Description;
  llvm::raw_string_ostream DescrOS(Description);
  DescrOS << (AddQuotes ? "'" : "") << Str
          << (AddQuotes ? "'" : "") << ": " << BugName;

  StringRef BugStr = DescrOS.str();
  PathDiagnosticLocation VDLoc(Attr->getLocation(), BR.getSourceManager());
  BR.EmitBasicReport(D, BugName, BugCategory,
                     BugStr, VDLoc, Attr->getRange());
}


void helperEmitStatementWarning(BugReporter &BR,
                                AnalysisDeclContext *AC,
                                const Stmt *S,
                                const Decl *D,
                                const StringRef &Str,
                                std::string BugName) {
  std::string Description;
  llvm::raw_string_ostream DescrOS(Description);
  DescrOS << "'" << Str << "' " << BugName;

  StringRef BugStr = DescrOS.str();

  PathDiagnosticLocation VDLoc =
      PathDiagnosticLocation::createBegin(S, BR.getSourceManager(), AC);

  BR.EmitBasicReport(D, BugName, BugCategory,
                     BugStr, VDLoc, S->getSourceRange());
}

void helperEmitInvalidAssignmentWarning(BugReporter &BR,
                                        AnalysisDeclContext *AC,
                                        ASTContext &Ctx,
                                        const Stmt *S,
                                        const ASaPType *LHS,
                                        const ASaPType *RHS,
                                        StringRef BugName) {
  std::string Description;
  llvm::raw_string_ostream DescrOS(Description);

  DescrOS << "The RHS type [" << (RHS ? RHS->toString(Ctx) : "")
          << "] is not assignable to the LHS type ["
          << (LHS ? LHS->toString(Ctx) : "")
          << "] " << BugName << ": ";
  S->printPretty(DescrOS, 0, Ctx.getPrintingPolicy());
  DescrOS << "]";

  StringRef BugStr = DescrOS.str();

  PathDiagnosticLocation VDLoc =
      PathDiagnosticLocation::createBegin(S, BR.getSourceManager(), AC);

  ento::BugType *BT = new ento::BugType(BugName, BugCategory);
  ento::BugReport *R = new ento::BugReport(*BT, BugStr, VDLoc);
  BR.emitReport(R);
}

} // end namespace asap
} // end namespace clang
