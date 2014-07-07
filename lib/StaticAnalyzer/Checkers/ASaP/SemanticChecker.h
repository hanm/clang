//=== SemanticChecker.h - Safe Parallelism checker -----*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This file defines the Semantic Checker pass of the Safe Parallelism
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

#include "llvm/ADT/DenseMap.h"

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Type.h"

#include "ASaPFwdDecl.h"
#include "Effect.h"


namespace clang {
namespace asap {

class ASaPSemanticCheckerTraverser :
  public RecursiveASTVisitor<ASaPSemanticCheckerTraverser> {

  typedef llvm::DenseMap<const Attr*, RplVector*> RplVecAttrMapT;

  const CheckerBase *Checker;
  BugReporter &BR;
  ASTContext &Ctx;
  raw_ostream &OS;
  SymbolTable &SymT;

  RplVecAttrMapT RplVecAttrMap;
  bool FatalError;

  void addToMap(Decl *D, RplVector *RplVec, QualType QT);
  void addASaPTypeToMap(ValueDecl *ValD, ASaPType *T);
  void addASaPTypeToMap(ValueDecl *D, RplVector *RV, Rpl *InRpl);
  void addASaPBaseTypeToMap(CXXRecordDecl *Derived,
                            QualType BaseQT, RplVector *RplVec);

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
  void emitTemporaryObjectNeedsAnnotation(const CXXTemporaryObjectExpr *Exp,
                                          const CXXRecordDecl *Class);

  /// \brief Print to the debug output stream (os) the attribute.
  template<typename AttrType> void helperPrintAttributes(Decl *D);

  const RplElement *findRegionOrParamName(const Decl *D, llvm::StringRef Name);
  /// \brief Looks for 'Name' in the declaration 'D' and its parent scopes.
  const RplElement *recursiveFindRegionOrParamName(const Decl *D,
                                                   llvm::StringRef Name);
  /// \brief check that the region arguments of the type of D are compatible
  /// with its region parameters
  void checkTypeRegionArgs(ValueDecl *D, const Rpl *DefaultInRpl);

  void checkBaseTypeRegionArgs(NamedDecl *D, const RegionBaseArgAttr *Att,
                        QualType BaseQT, const Rpl *DefaultInRpl);
  void checkParamAndArgCounts(NamedDecl *D, const Attr* Att, QualType QT,
                              const ResultTriplet &ResTriplet,
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
  template<typename AttrType> bool checkRpls(Decl* D);

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
  void buildPartialEffectSummary(FunctionDecl *D, ConcreteEffectSummary &ES);

  void buildEffectSummary(FunctionDecl* D, ConcreteEffectSummary &ES);

  void checkBaseSpecifierArgs(CXXRecordDecl *D);

  //FIXME make this function static
  const RegionBaseArgAttr *findBaseArg(const CXXRecordDecl *D, StringRef S);
  //FIXME make this function static
  const CXXBaseSpecifier *findBaseDecl(const CXXRecordDecl *D, StringRef S);

  void helperMissingRegionArgs(NamedDecl *D, const Attr* Att,
                               RplVector *RplVec, long ParamCount);

public:
  typedef RecursiveASTVisitor<ASaPSemanticCheckerTraverser> BaseClass;

  explicit ASaPSemanticCheckerTraverser ();
  virtual ~ASaPSemanticCheckerTraverser ();
  inline bool encounteredFatalError() { return FatalError; }

  bool shouldVisitTemplateInstantiations() const { return true; }
  bool shouldVisitImplicitCode() const { return true; }
  bool shouldWalkTypesOfTypeLocs() const { return true; }

  // Visitors
  bool VisitValueDecl(ValueDecl *D);
  bool VisitParmVarDecl(ParmVarDecl *D);
  bool VisitFunctionDecl(FunctionDecl *D);
  bool VisitRecordDecl (RecordDecl *D);
  bool VisitFieldDecl(FieldDecl *D);
  bool VisitVarDecl(VarDecl *D);
  bool VisitCXXMethodDecl(clang::CXXMethodDecl *D);
  bool VisitCXXConstructorDecl(CXXConstructorDecl *D);
  bool VisitCXXDestructorDecl(CXXDestructorDecl *D);
  bool VisitCXXConversionDecl(CXXConversionDecl *D);
  bool VisitFunctionTemplateDecl(FunctionTemplateDecl *D);
  bool VisitCXXTemporaryObjectExpr(CXXTemporaryObjectExpr *Exp);
  // Traversers
  bool TraverseTypedefDecl(TypedefDecl *D);
}; // End class ASaPSemanticCheckerTraverser.
} // End namespace asap.
} // End namespace clang.

#endif
