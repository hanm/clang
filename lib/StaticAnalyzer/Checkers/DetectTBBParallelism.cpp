//=== DetectTBBParallelism.cpp - Safe Parallelism checker ----*- C++ -*--===//
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

#include "clang/AST/ASTContext.h"

#include "ASaPUtil.h"
#include "ASaPSymbolTable.h"
#include "DetectTBBParallelism.h"
#include "SpecificNIChecker.h"

namespace clang {
namespace asap {

// Static

void DetectTBBParallelism::
emitUnexpectedTBBParallelFor(const FunctionDecl *D) {
  StringRef Str = "";
  StringRef BugName = "unexpected tbb::parallel_for method: parallelism"
                      " invoked through it will not be checked";
  helperEmitDeclarationWarning(BR, D, Str, BugName, false);
}

DetectTBBParallelism::
DetectTBBParallelism()
  : BR(*SymbolTable::VB.BR),
    Ctx(*SymbolTable::VB.Ctx),
    OS(*SymbolTable::VB.OS),
    SymT(*SymbolTable::Table),
    FatalError(false) {}

bool DetectTBBParallelism::
VisitFunctionDecl(FunctionDecl *D) {

  std::string Str = D->getNameInfo().getAsString();
  StringRef Name(Str);
  OS << "DEBUG:: VisitFunctionDecl (" << D << "). Name = " << Name << "\n";
  OS << "D->isThisDeclarationADefinition() = "
     << D->isThisDeclarationADefinition() << "\n";
  OS << "D->getTypeSourceInfo() = " << D->getTypeSourceInfo() << "\n";

  OS << "DEBUG:: D TemplateKind = " << D->getTemplatedKind() << "\n";
  OS << "DEBUG:: D " << (D->isTemplateDecl() ? "IS " : "is NOT ")
      << "a template\n";
  OS << "DEBUG:: D " << (D->isTemplateParameter() ? "IS " : "is NOT ")
      << "a template PARAMETER\n";
  OS << "DEBUG:: D " << (D->isFunctionTemplateSpecialization() ? "IS " : "is NOT ")
      << "a function template SPECIALIZATION\n";
  OS << "DEBUG:: D " << (D->isTemplateInstantiation() ? "IS " : "is NOT ")
      << "a template INSTANTIATION\n";
  FunctionTemplateDecl *FTD1 = D->getPrimaryTemplate();
  OS << "DEBUG:: D->getPrimaryTemplate() = " << FTD1 << "\n";
  FunctionTemplateDecl *FTD2 = D->getDescribedFunctionTemplate();
  OS << "DEBUG:: D->getDescribedTemplate() = " << FTD2 << "\n";
  //D->getDeclName().printName(OS);
  D->print(OS, Ctx.getPrintingPolicy());
  OS << "\n";
  D->dump(OS);
  OS << "'\n";

  // Detect TBB functions
  // As far as I know so far, all the TBB APIs are templates, so we will
  // simply add the template function and not all the instantiations.
  // We will also have to change acoordingly the call-site detection
  // code in NonInterferenceChecker so that if the FunctionDecl kind
  // is FunctionTemplateSpecialization, we look-up the Function template
  // in the ParTable of the SymbolTable.
  FunctionDecl *FunD = D;
  if (FunD->getTemplatedKind() ==
      FunctionDecl::TK_FunctionTemplateSpecialization) {
    FunD = FunD->getPrimaryTemplate()->getTemplatedDecl();
  }
  // Detect tbb::parallel_for
  OS << "DEBUG:: Name = " << Name << "\n";
  DeclContext *NamespaceCtx = FunD->getEnclosingNamespaceContext();
  if (isa<NamespaceDecl>(NamespaceCtx)) {
    NamespaceDecl *NamespaceD = dyn_cast<NamespaceDecl>(NamespaceCtx);
    assert(NamespaceD);
    StringRef NamespaceStr = NamespaceD->getName();
    OS << "DEBUG:: enclosing namespace = " << NamespaceStr << "\n";
    if (!NamespaceStr.compare("tbb")) {
      if (!Name.compare("parallel_for")) {
        // Find the namespace, make sure it's 'tbb'
        OS << "DEBUG:: Found one!\n";
        // TODO:  Add D to Symboltable Map.
        ParmVarDecl *Parm1 = FunD->getParamDecl(0);
        StringRef ParmTypeStr = Parm1->getType().getAsString();
        OS << "DEBUG:: 1st Param Type = " << ParmTypeStr << "\n";
        // Case 1. parallel_for(Range, Body, ...)
        //if (ParmTypeStr.startswith("const class tbb::blocked_range")) {
        if (!ParmTypeStr.compare("const Range &")) {
          ParmVarDecl *Body = D->getParamDecl(1);
          OS << "DEBUG:: 2nd parameter should be a Body: ";
          Body->print(OS, Ctx.getPrintingPolicy());
          OS << "\n";
          // Add to SymT
          OS << "DEBUG:: Adding a parallel_for<Range> to SymT (" << FunD << ")\n";

          SymT.addParallelFun(FunD, new TBBParallelForRangeNIChecker());
          //assert(Result && "failed adding SpecificNIChecker to ParTable");
        }
        // Case 2. parallel_for(Index, ..., Function, ...)
        else if (!ParmTypeStr.compare("Index")) {
          // Add to SymT
          OS << "DEBUG:: Adding a parallel_for<Index> to SymT (" << FunD << ")\n";
          SymT.addParallelFun(FunD, new TBBParallelForIndexNIChecker());
          //assert(Result && "failed adding SpecificNIChecker to ParTable");
        }
        else {
          // TODO emit warning that we found an unexpected tbb::parallel_for
        }
      } else if (!Name.compare("parallel_invoke")) {
        // Add to SymT
        OS << "DEBUG:: Adding a parallel_invoke to SymT (" << FunD << ")\n";
        SymT.addParallelFun(FunD, new TBBParallelInvokeNIChecker());
        //assert(Result && "failed adding SpecificNIChecker to ParTable");
      }
    } // end if tbb
  }
  OS << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n";
  return true;
}

} // end namespace asap
} // end namespace clang
