//=== ASaPType.h - Safe Parallelism checker -------*- C++ -*---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This file defines the ASaPType class used by the Safe Parallelism
// checker, which tries to prove the safety of parallelism given region
// and effect annotations.
//
//===----------------------------------------------------------------===//

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_TYPE_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_TYPE_H

#include "clang/AST/Type.h"

#include "ASaPFwdDecl.h"
#include "ASaPUtil.h"
#include "ASaPInheritanceMap.h"

namespace clang {
namespace asap {

class ASaPType {
  /// \brief C++ Qualified Type of ASaPType
  QualType QT;
  /// \brief this map is owned by the SymbolTableEntry and should be immutable.
  const InheritanceMapT *InheritanceMap;

  /// \brief Region Argument Vector
  RplVector *ArgV;
  /// \brief In RPL (can be null)
  Rpl *InRpl;

  // Private Functions
  /// \brief Depending on QT, removes the head of ArgV to set InRpl
  void adjust();
  //
  inline static bool areUnqualQTsEqual(const QualType QT1, const QualType QT2){
    return QT1.getUnqualifiedType().getCanonicalType()
              == QT2.getUnqualifiedType().getCanonicalType();
  }

  inline static bool areQTsEqual(const QualType QT1, const QualType QT2) {
    return QT1.getCanonicalType() == QT2.getCanonicalType();
  }

  /// \brief Determine whether the type \p Derived is a C++ class that is
  /// derived from the type \p Base. (adapted from SemaDeclCXX.cpp)
  static bool isDerivedFrom(QualType Derived, QualType Base);

public:
  ASaPType(QualType QT, const InheritanceMapT *InheritanceMap,
           RplVector *ArgV, Rpl *InRpl = 0, bool Simple = false);
  ASaPType(const ASaPType &T);
  ~ASaPType();

  inline static bool typeExpectsInRpl(QualType QT) {
    if (QT->isScalarType() && !QT->isReferenceType())
      return true;
    else
      return false;
  }

  static QualType deref(QualType QT, int DerefNum, ASTContext &Ctx);

  /// \brief Returns true iff this is of FunctionType.
  inline bool isFunctionType() const { return QT->isFunctionType(); }
  /// \brief Returns true iff this is a Reference.
  inline bool isReferenceType() const { return QT->isReferenceType(); }
  inline bool hasInheritanceMap() const {
    return InheritanceMap ? true : false;
  }

  bool hasRplVar() const;

  /// \brief set the QualType part without touching the Region info.
  /// useful for implicit casts (e.g., cast from int to float)
  inline void setQT(QualType QT) { this->QT = QT; }

  /// \brief Return the size of the ArgsVector.
  int getArgVSize() const;
  /// \brief Return the In RPL which can be null.
  const Rpl *getInRpl() const;
  /// \brief Return the In RPL of this after DerefNum dereferences.
  const Rpl *getInRpl(int DerefNum) const;
  /// \brief Return the Region Arguments Vector
  const RplVector *getArgV() const;
  /// \brief Return the Argument for substitution after DerefNum dereferences.
  /// FIXME: support multiple region parameters per class type.
  const Rpl *getSubstArg(int DerefNum = 0) const;
  size_t getSubstSize() const;
  /// \brief Return the QualType of this ASapType.
  inline QualType getQT() const { return QT; }
  /// \brief Return the QualType of this after DerefNum dereferences.
  QualType getQT(int DerefNum) const;
  /// \brief If this is a function type, return its return type; else null.
  ASaPType *getReturnType();
  /// \brief For ArrayType, modify type by applying one level of sub-scripting
  void arraySubscript();
  /// \brief Dereferences this type DerefNum times.
  void deref(int DerefNum = 1);
  /// \brief Modifies this type as if its address were taken.
  void addrOf(QualType RefQT);
  /// \brief Set the In annotation to NULL (and free the Rpl).
  void dropInRpl();
  /// \brief free the ArgV RPLs and allocate a new empty ArgV
  void dropArgV();
  /// \brief Returns a string describing this ASaPType.
  std::string toString(ASTContext &Ctx) const;
  /// \brief Returns a string describing this ASaPType.
  std::string toString() const;
  /// \brief Queries Prolog for RplVars and prints solution to @param OS
  void printSolution(llvm::raw_ostream &OS) const;
  /// \brief Returns true when 'this' can be assigned to That
  // Note: That=LHS and this=RHS
  Trivalent isAssignableTo(const ASaPType &That, SymbolTable &SymT,
                           ASTContext &Ctx, bool IsInit = false,
                           bool GenConstraints = true) const;
  /// \brief true when when cast is successful
  bool implicitCastToBase(QualType BaseQT, SymbolTable &SymT);
  /// \brief  true when 'this' is a subtype (derived type) of 'that'.
  Trivalent isSubtypeOf(const ASaPType &That,
                        SymbolTable &SymT, bool GenConstraints) const;
  /// \brief Joins this to That (by modifying this).
  /// Join returns the smallest common supertype (Base Type).
  void join(ASaPType *That);
  /// \brief Substitution (ASaPType).
  void substitute(const SubstitutionVector *SubV);
  /// \brief Substitution (ASaPType).
  void substitute(const SubstitutionSet *SubV);
  /// \brief Performs substitution on type: this[FromEl <- ToRpl].
  void substitute(const Substitution *S);
}; // end class ASaPType

} // End namespace asap.
} // End namespace clang.

#endif



