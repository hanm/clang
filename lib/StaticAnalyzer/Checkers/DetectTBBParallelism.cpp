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
//#include "clang/AST/Decl.h"

#include "ASaPUtil.h"
#include "ASaPSymbolTable.h"
#include "DetectTBBParallelism.h"
#include "SpecificNIChecker.h"

namespace clang {
namespace asap {

DetectTBBParallelism::
DetectTBBParallelism()
  : BR(*SymbolTable::VB.BR),
    Ctx(*SymbolTable::VB.Ctx),
    OS(*SymbolTable::VB.OS),
    SymT(*SymbolTable::Table),
    FatalError(false) {}

bool DetectTBBParallelism::VisitFunctionDecl(FunctionDecl *D) {

  StringRef Name = D->getNameInfo().getAsString();
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
  OS << "':\n";
  OS << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

  // Detect tbb::parallel_for
  OS << "DEBUG:: Name = " << Name << "\n";
  if (!Name.compare("parallel_for")) {
    // Find the namespace, make sure it's 'tbb'
    DeclContext *NamespaceCtx = D->getEnclosingNamespaceContext();
    assert(isa<NamespaceDecl>(NamespaceCtx));
    NamespaceDecl *NamespaceD = dyn_cast<NamespaceDecl>(NamespaceCtx);
    assert(NamespaceD);
    StringRef NamespaceStr = NamespaceD->getName();
    OS << "DEBUG:: enclosing namespace = " << NamespaceStr << "\n";
    if (!NamespaceStr.compare("tbb")) {
      OS << "DEBUG:: Found one!\n";
      // TODO:  Add D to Symboltable Map.
      ParmVarDecl *Parm1 = D->getParamDecl(0);
      StringRef ParmTypeStr = Parm1->getType().getAsString();
      OS << "DEBUG:: 1st Param Type = " << ParmTypeStr << "\n";
      // Case 1. parallel_for(Range, Body, ...)
      if (ParmTypeStr.startswith("const class tbb::blocked_range")) {
        // Find Body
        ParmVarDecl *Body = D->getParamDecl(1);
        OS << "DEBUG:: 2nd parameter should be a Body: ";
        Body->print(OS, Ctx.getPrintingPolicy());
        OS << "\n";
        // Add to SymT
        OS << "DEBUG:: Adding a 'Range' parallel_for to SymT\n";

        bool Result = SymT.addParallelFun(D, new TBBParallelForRangeNIChecker());
        assert(Result && "failed adding SpecificNIChecker to ParTable");
      }
      // Case 2. parallel_for(Index, ..., Function, ...)
      else {
        // TODO
      }
    }

  }
  return true;
}

} // end namespace asap
} // end namespace clang
