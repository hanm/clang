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
#include "DetectTBBParallelism.h"

namespace clang {
namespace asap {

DetectTBBParallelism::
DetectTBBParallelism(VisitorBundle &_VB)
  : VB(_VB),
  BR(VB.BR),
  Ctx(VB.Ctx),
  OS(VB.OS),
  SymT(VB.SymT),
  FatalError(false) {}

bool DetectTBBParallelism::VisitFunctionDecl(FunctionDecl *D) {

  OS << "DEBUG:: VisitFunctionDecl (" << D << ")\n";
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
  OS << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"
    << "DEBUG:: printing ASaP attributes for method or function '";
  //D->getDeclName().printName(OS);
  D->print(OS, Ctx.getPrintingPolicy());
  OS << "':\n";

  StringRef Name = D->getNameInfo().getAsString();
  OS << "DEBUG:: FunctionDecl Name = " << Name << "\n";
  if (!Name.compare("parallel_for")) {
    // Find the namespace, make sure it's 'tbb'
    DeclContext *NamespaceCtx = D->getEnclosingNamespaceContext();
    assert(isa<NamespaceDecl>(NamespaceCtx));
    NamespaceDecl *NamespaceD = dyn_cast<NamespaceDecl>(NamespaceCtx);
    assert(NamespaceD);
    StringRef NamespaceStr = NamespaceD->getName();
    OS << "DEBUG:: enclosing namespace = " << NamespaceStr << "\n";
    if (!NamespaceStr.compare("tbb")) {
      OS << "Found one!\n\n";
      // TODO:  Add D to Symboltable Map.
    }

  }
  return true;
}

} // end namespace asap
} // end namespace clang
