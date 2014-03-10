//=== EffectSummaryNormalizer.cpp - Safe Parallelism checker *-C++-*-===//
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

#include "clang/AST/ASTContext.h"

#include "ASaPSymbolTable.h"
#include "ASaPUtil.h"
#include "Effect.h"
#include "EffectSummaryNormalizer.h"

namespace clang {
namespace asap {

EffectSummaryNormalizerTraverser::
EffectSummaryNormalizerTraverser()
  : BR(*SymbolTable::VB.BR),
    Ctx(*SymbolTable::VB.Ctx),
    OS(*SymbolTable::VB.OS),
    SymT(*SymbolTable::Table),
    FatalError(false) {}

void EffectSummaryNormalizerTraverser::
emitCanonicalDeclHasSmallerEffectSummary(const Decl *D, const StringRef Str) {

  StringRef BugName = "effect summary of canonical declaration does not cover"\
    " the summary of this declaration";
  helperEmitDeclarationWarning(BR, D, Str, BugName);
  FatalError = true;
}

// Visitors

bool EffectSummaryNormalizerTraverser::VisitFunctionDecl(FunctionDecl *D) {
  OS << "DEBUG:: VisitFunctionDecl (" << D << ")\n";
  OS << "D->isThisDeclarationADefinition() = "
     << D->isThisDeclarationADefinition() << "\n";
  OS << "D->getTypeSourceInfo() = " << D->getTypeSourceInfo() << "\n";

  OS << "DEBUG:: D " << (D->isTemplateDecl() ? "IS " : "is NOT ")
      << "a template\n";
  OS << "DEBUG:: D " << (D->isTemplateParameter() ? "IS " : "is NOT ")
      << "a template PARAMETER\n";
  OS << "DEBUG:: D " << (D->isFunctionTemplateSpecialization() ? "IS " : "is NOT ")
      << "a function template SPECIALIZATION\n";

  OS << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"
    << "DEBUG:: printing ASaP attributes for method or function '";
  //D->getDeclName().printName(OS);
  D->print(OS, Ctx.getPrintingPolicy());
  OS << "':\n";

    /// C. Check effect summary
    /// C.1. Build Effect Summary
    const EffectSummary *ES = SymT.getEffectSummary(D);
    assert(ES && "Function missing effect summary");
    /// C.2. Check Effects covered by canonical Declaration
    const FunctionDecl *CanFD = D->getCanonicalDecl();
    if (CanFD && CanFD != D && !D->isFunctionTemplateSpecialization()) {
      // Case 1: We are not visiting the canonical Decl
      OS << "DEBUG:: CanFD != D\n";
      OS << "DEBUG:: D="; D->print(OS); OS << "\n";
      OS << "DEBUG:: CanFD="; CanFD->print(OS); OS << "\n";

      OS << "DEBUG:: D " << (D->isTemplateDecl() ? "IS " : "is NOT ")
         << "a template\n";
      OS << "DEBUG:: D " << (D->isTemplateParameter() ? "IS " : "is NOT ")
         << "a template PARAMETER\n";
      OS << "DEBUG:: D " << (D->isFunctionTemplateSpecialization() ? "IS " : "is NOT ")
         << "a function template SPECIALIZATION\n";

      OS << "DEBUG:: CanFD " << (CanFD->isTemplateDecl() ? "IS" : "is NOT ")
         << "a template\n";
      OS << "DEBUG:: CanFD " << (CanFD->isTemplateParameter() ? "IS " : "is NOT ")
         << "a template PARAMETER\n";
      OS << "DEBUG:: CanFD " << (CanFD->isFunctionTemplateSpecialization() ? "IS " : "is NOT ")
         << "a function template SPECIALIZATION\n";

      OS << "DEBUG:: D="; D->dump(OS); OS << "\n";
      OS << "DEBUG:: CanFD="; CanFD->dump(OS); OS << "\n";

      const EffectSummary *CanES = SymT.getEffectSummary(CanFD);
      assert(CanES && "Function missing effect summary");
      EffectSummary::ResultKind RK=CanES->covers(ES);
      if (RK == EffectSummary::RK_FALSE) {
        std::string Name = D->getNameInfo().getAsString();
        emitCanonicalDeclHasSmallerEffectSummary(D, Name);
      } else if (RK == EffectSummary::RK_DUNNO){
	assert(false && "Found variable effect summary");
      }else { // The effect summary of the canonical decl covers this.

        // Set the Effect summary of this declaration to be the same
        // as that of the canonical declaration.
        // (Makes a copy of the effect summary).

        // Actually why not keep the original effect summary?
        // -> commenting out the next line.
        //SymT.resetEffectSummary(D, CanFD);
      }
    }
  return true;
}

} // end namespace asap
} // end namespace clang
