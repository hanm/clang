//=== SemanticChecker.h - Safe Parallelism checker -----*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This files defines the Semantic Checker pass of the Safe Parallelism
// checker, which tries to prove the safety of parallelism given region
// and effect annotations.
//
//===----------------------------------------------------------------===//

/// Traversal that checks semantic properties of the ASaP Annotations
/// 1. Region and parameter declarations are well formed
/// 2. Rpls are valid
/// 2.a Rpl Elements are declared
/// 2.b Parameters only appear at the first position
/// 3. Correct number of region argument
/// 4. Declaration has too many region arguments
/// 5. Declaration has too few region arguments (ignored when default
///    arguments are enabled).
/// 6. Check that effect summaries are minimal
/// 7. Build map from FunctionDecl to effect summaries

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_SEMANTIC_CHECKER_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_SEMANTIC_CHECKER_H

#include "clang/AST/ASTContext.h"
#include "clang/AST/Type.h"
#include "clang/AST/RecursiveASTVisitor.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include "ASaPSymbolTable.h"
#include "Rpl.h"
#include "Effect.h"
#include "clang/AST/Attr.h"


namespace clang {

class Attr;
class AnalysisDeclContext;
class Decl;
class DeclContext;
class FunctionDecl;
class ValueDecl;

namespace asap {

class SymbolTable;

class ASaPSemanticCheckerTraverser :
  public RecursiveASTVisitor<ASaPSemanticCheckerTraverser> {

  typedef llvm::DenseMap<const Attr*, RplVector*> RplVecAttrMapT;
  ento::BugReporter &BR;
  ASTContext &Ctx;
  AnalysisDeclContext *AC;
  llvm::raw_ostream &OS;
  SymbolTable &SymT;
  RplVecAttrMapT RplVecAttrMap;
  bool FatalError;
  bool NextFunctionIsATemplatePattern;

  void addASaPTypeToMap(ValueDecl *D, RplVector *RV, Rpl *InRpl);
  void addASaPBaseTypeToMap(CXXRecordDecl *CXXRD, QualType QT, RplVector *RplVec);

  /// \brief Emit error for redeclared region name within scope.
  void emitRedeclaredRegionName(const Decl *D, const llvm::StringRef &Str);
  /// \brief Emit error for redeclared region parameter name within scope.
  void emitRedeclaredRegionParameter(const Decl *D, const llvm::StringRef &Str);
  /// \brief Emit error for misplaced region parameter within RPL.
  void emitMisplacedRegionParameter(const Decl *D,
                                    const Attr* A,
                                    const llvm::StringRef &S);
  /// \brief Emit a Warning when the input string (which is assumed to be an
  /// RPL element) is not declared.
  void emitUndeclaredRplElement(const Decl *D,
                                const Attr *A,
                                const llvm::StringRef &Str);
  /// \brief Attribute A contains an undeclared nested name specifier
  void emitNameSpecifierNotFound(const Decl *D, const Attr *A, StringRef Name);
  /// \brief Declaration D is missing region argument(s).
  void emitMissingRegionArgs(const Decl *D, const Attr* A, int Expects);
  /// \brief The number of region argument(s) of D is unknown.
  void emitUnknownNumberOfRegionParamsForType(const Decl *D);
  /// \brief Region arguments Str on declaration D are superfluous for its type.
  void emitSuperfluousRegionArg(const Decl *D, const Attr *A,
                                int Expects, llvm::StringRef Str);
  /// \brief Region name or parameter contains illegal characters.
  void emitIllFormedRegionNameOrParameter(const Decl *D, const Attr *A,
                                          llvm::StringRef Name);
  void emitCanonicalDeclHasSmallerEffectSummary(const Decl *D,
                                                const StringRef &S);
  /// \brief Effect summary not minimal: effect E1 is covered by effect E2.
  void emitEffectCovered(const Decl *D, const Effect *E1, const Effect *E2);
  void emitNoEffectInNonEmptyEffectSummary(const Decl *D, const Attr *A);
  void emitMissingBaseClassArgument(const Decl *D, StringRef Str);
  void emitAttributeMustReferToDirectBaseClass(const Decl *D,
                                               const RegionBaseArgAttr *A);
  void emitDuplicateBaseArgAttributesForSameBase(const Decl *D,
                                                 const RegionBaseArgAttr *A1,
                                                 const RegionBaseArgAttr *A2);

  void emitMissingBaseArgAttribute(const Decl *D, StringRef BaseClass);
  void emitEmptyStringRplDisallowed(const Decl *D, Attr *A);

  /// \brief Print to the debug output stream (os) the attribute.
  template<typename AttrType>
  inline void helperPrintAttributes(Decl *D) {
    for (specific_attr_iterator<AttrType>
         I = D->specific_attr_begin<AttrType>(),
         E = D->specific_attr_end<AttrType>();
         I != E; ++I) {
      (*I)->printPretty(OS, Ctx.getPrintingPolicy());
      OS << "\n";
    }
  }

  /// \brief Return the string name of the region or region parameter declaration
  /// based on the Kind of the Attribute (RegionAttr or RegionParamAttr).
  llvm::StringRef getRegionOrParamName(const Attr *Attribute);

  /// \brief Check that the region name and region parameter declarations
  /// of D are well formed (don't contain illegal characters).
  template<typename AttrType>
  bool checkRegionOrParamDecls(Decl *D) {
    bool Result = true;

    specific_attr_iterator<AttrType>
        I = D->specific_attr_begin<AttrType>(),
        E = D->specific_attr_end<AttrType>();
    for ( ; I != E; ++I) {
      assert(isa<RegionAttr>(*I) || isa<RegionParamAttr>(*I));
      const llvm::StringRef ElmtNames = getRegionOrParamName(*I);

      llvm::SmallVector<StringRef, 8> RplElmtVec;
      ElmtNames.split(RplElmtVec, Rpl::RPL_LIST_SEPARATOR);
      for (size_t Idx = 0 ; Idx != RplElmtVec.size(); ++Idx) {
        llvm::StringRef Name = RplElmtVec[Idx].trim();
        if (Rpl::isValidRegionName(Name)) {
          /// Add it to the vector.
          OS << "DEBUG:: creating RPL Element called " << Name << "\n";
          if (isa<RegionAttr>(*I)) {
            const Decl *ScopeDecl = D;
            if (isa<EmptyDecl>(D)) {
              // An empty declaration is typically at global scope
              // E.g., [[asap::name("X")]];
              ScopeDecl = getDeclFromContext(D->getDeclContext());
              assert(ScopeDecl);
            }
            if (!SymT.addRegionName(ScopeDecl, Name)) {
              // Region name already declared at this scope.
              emitRedeclaredRegionName(D, Name);
              Result = false;
            }
          } else if (isa<RegionParamAttr>(*I)) {
            if (!SymT.addParameterName(D, Name)) {
              // Region parameter already declared at this scope.
              emitRedeclaredRegionParameter(D, Name);
              Result = false;
            }
          }
        } else {
          /// Emit bug report: ill formed region or parameter name.
          emitIllFormedRegionNameOrParameter(D, *I, Name);
          Result = false;
        }
      } // End for each Element of Attribute.
    } // End for each Attribute of type AttrType.
    return Result;
  }

  const RplElement *findRegionOrParamName(const Decl *D, llvm::StringRef Name);
  const Decl *getDeclFromContext(const DeclContext *DC);
  /// \brief Looks for 'Name' in the declaration 'D' and its parent scopes.
  const RplElement *recursiveFindRegionOrParamName(const Decl *D,
                                                   llvm::StringRef Name);
  void checkTypeRegionArgs(ValueDecl *D, const Rpl *DefaultInRpl);
  void checkBaseTypeRegionArgs(NamedDecl *D, const RegionBaseArgAttr *Att,
                        QualType BaseQT, const Rpl *DefaultInRpl);
  void checkParamAndArgCounts(NamedDecl *D, const Attr* Att, QualType QT,
                              const SymbolTable::ResultTriplet &ResTriplet,
                              RplVector *RplVec, const Rpl *DefaultInRpl);

  /// \brief Check that the annotations of type AttrType of declaration
  /// D have RPLs whose elements have been declared, and if so, add RPL
  /// to the map from Attrs to Rpls.
  bool checkRpls(Decl *D, Attr *A, llvm::StringRef RplsStr);
  /// \brief Check that the annotations of type AttrType of declaration
  /// D have RPLs whose elements have been declared, and if so, add RPL
  /// to the map from Attrs to Rpls.
  Rpl *checkRpl(Decl *D, Attr *A, llvm::StringRef RplStr);
  /// \brief Wrapper calling checkRpl.
  /// AttrType must implement getRpl (i.e., RegionArgAttr, & Effect Attributes).
  template<typename AttrType>
  bool checkRpls(Decl* D) {
    bool Success = true;
    const RplVector *RV = 0;
    for (specific_attr_iterator<AttrType>
         I = D->specific_attr_begin<AttrType>(),
         E = D->specific_attr_end<AttrType>();
         I != E; ++I) {
      Success &= checkRpls(D, *I, (*I)->getRpl());
    }
    if (!Success) {
      delete RV;
      RV = 0;
      FatalError = true;
    }
    return Success;
  }

  /// Map AttrType to Effect Kind
  /// FIXME: we should move this elsewhere, probably in Effect.h
  inline Effect::EffectKind getEffectKind(const NoEffectAttr* Attr) {
    return Effect::EK_NoEffect;
  }
  inline Effect::EffectKind getEffectKind(const ReadsEffectAttr* Attr) {
    return Effect::EK_ReadsEffect;
  }
  inline Effect::EffectKind getEffectKind(const WritesEffectAttr* Attr) {
    return Effect::EK_WritesEffect;
  }
  inline Effect::EffectKind getEffectKind(const AtomicReadsEffectAttr* Attr) {
    return Effect::EK_AtomicReadsEffect;
  }
  inline Effect::EffectKind getEffectKind(const AtomicWritesEffectAttr* Attr) {
    return Effect::EK_AtomicWritesEffect;
  }

  /// Called with AttrType being one of ReadsEffectAttr, WritesEffectAttr,
  /// ΑtomicReadsEffectAttr, or AtomicWritesEffectAttr.
  template<typename AttrType>
  void buildPartialEffectSummary(FunctionDecl *D, EffectSummary &ES) {
    for (specific_attr_iterator<AttrType>
         I = D->specific_attr_begin<AttrType>(),
         E = D->specific_attr_end<AttrType>();
         I != E; ++I) {
      Effect::EffectKind EK = getEffectKind(*I);
      RplVector *Tmp = RplVecAttrMap[*I];

      if (Tmp) { /// Tmp may be NULL if the RPL was ill formed (e.g., contained
                 /// undeclared RPL elements).
        for (size_t Idx = 0; Idx < Tmp->size(); ++Idx) {
          const Effect E(EK, Tmp->getRplAt(Idx), *I);
          bool Success = ES.insert(&E);
          assert(Success);
        }
      }
    }
  }

  void buildEffectSummary(FunctionDecl* D, EffectSummary &ES);

  void checkBaseSpecifierArgs(CXXRecordDecl *D);

  //FIXME make this function static
  const RegionBaseArgAttr *findBaseArg(const CXXRecordDecl *D, StringRef S);
  //FIXME make this function static
  const CXXBaseSpecifier *findBaseDecl(const CXXRecordDecl *D, StringRef S);

public:
  typedef RecursiveASTVisitor<ASaPSemanticCheckerTraverser> BaseClass;

  explicit ASaPSemanticCheckerTraverser (
    ento::BugReporter &BR, ASTContext &Ctx,
    AnalysisDeclContext *AC, raw_ostream &OS,
    SymbolTable &SymT);
  virtual ~ASaPSemanticCheckerTraverser ();
  inline bool encounteredFatalError() { return FatalError; }

  bool shouldVisitTemplateInstantiations() const { return true; }
  bool shouldVisitImplicitCode() const { return true; }

  bool VisitValueDecl(ValueDecl *D);
  bool VisitParmVarDecl(ParmVarDecl *D);
  bool VisitFunctionDecl(FunctionDecl *D);
  bool VisitRecordDecl (RecordDecl *D);
  bool VisitEmptyDecl(EmptyDecl *D);
  bool VisitNamespaceDecl (NamespaceDecl *D);
  bool VisitFieldDecl(FieldDecl *D);
  bool VisitVarDecl(VarDecl *D);
  bool VisitCXXMethodDecl(clang::CXXMethodDecl *D);
  bool VisitCXXConstructorDecl(CXXConstructorDecl *D);
  bool VisitCXXDestructorDecl(CXXDestructorDecl *D);
  bool VisitCXXConversionDecl(CXXConversionDecl *D);
  bool VisitFunctionTemplateDecl(FunctionTemplateDecl *D);

}; // End class ASaPSemanticCheckerTraverser.
} // End namespace asap.
} // End namespace clang.

#endif
