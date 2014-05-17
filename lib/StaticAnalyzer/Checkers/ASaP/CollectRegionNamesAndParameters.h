//=== CollectRegionNamesAndParameters.h - Safe Parallelism checker --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This file defines the pass that collects the region names and
// parameters for the Safe Parallelism checker, which tries to prove
// the safety of parallelism given region and effect annotations.
//
//===----------------------------------------------------------------===//

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_COLLECT_REGION_NAMES_AND_PARAMETERS_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_COLLECT_REGION_NAMES_AND_PARAMETERS_H

//#include "llvm/ADT/DenseMap.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Type.h"

#include "ASaPFwdDecl.h"
#include "Effect.h"

namespace clang {
namespace asap {

class CollectRegionNamesAndParametersTraverser :
  public RecursiveASTVisitor<CollectRegionNamesAndParametersTraverser> {

  const CheckerBase *Checker;
  BugReporter &BR;
  ASTContext &Ctx;
  raw_ostream &OS;
  SymbolTable &SymT;

  bool FatalError;

  /// \brief Return the string name of the region or region parameter declaration
  /// based on the Kind of the Attribute (RegionAttr or RegionParamAttr).
  llvm::StringRef getRegionOrParamName(const Attr *Attribute);

  /// \brief Print to the debug output stream (os) the attribute.
  template<typename AttrType> void helperPrintAttributes(Decl *D);
  /// \brief Check that the region name and region parameter declarations
  /// of D are well formed (don't contain illegal characters).
  template<typename AttrType> bool checkRegionOrParamDecls(Decl *D);

  /// \brief Emit error for redeclared region name within scope.
  void emitRedeclaredRegionName(const Decl *D, const llvm::StringRef &Str);
  /// \brief Emit error for redeclared region parameter name within scope.
  void emitRedeclaredRegionParameter(const Decl *D, const llvm::StringRef &Str);
  /// \brief Region name or parameter contains illegal characters.
  void emitIllFormedRegionNameOrParameter(const Decl *D, const Attr *A,
                                          llvm::StringRef Name);

public:
  typedef RecursiveASTVisitor<CollectRegionNamesAndParametersTraverser> BaseClass;

  explicit CollectRegionNamesAndParametersTraverser ();
  //virtual ~CollectRegionNamesAndParametersTraverser ();

  inline bool encounteredFatalError() { return FatalError; }

  bool shouldVisitTemplateInstantiations() const { return true; }
  bool shouldVisitImplicitCode() const { return true; }
  bool shouldWalkTypesOfTypeLocs() const { return true; }

  bool VisitFunctionDecl(FunctionDecl *D);
  bool VisitRecordDecl (RecordDecl *D);
  bool VisitEmptyDecl(EmptyDecl *D);
  bool VisitNamespaceDecl (NamespaceDecl *D);
  bool VisitValueDecl(ValueDecl *D);

}; // End class CollectRegionNamesAndParametersTraverser.
} // End namespace asap.
} // End namespace clang.


#endif
