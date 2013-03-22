//=== ASaPType.h - Safe Parallelism checker -----*- C++ -*---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This files defines the ASaPType class used by the Safe Parallelism
// checker, which tries to prove the safety of parallelism given region
// and effect annotations.
//
//===----------------------------------------------------------------===//

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_TYPE_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_TYPE_H

#include "clang/AST/Type.h"
#include "Effect.h"

namespace clang {
namespace asap {

class Rpl;
class RplVector;

class ASaPType {
  friend class ASaPType;
  QualType QT;
  RplVector *ArgV;
  // InRpl can be null.
  Rpl *InRpl;
  // Private Functions
  inline static bool areQTsEqual(const QualType QT1, const QualType QT2) {
    return QT1.getUnqualifiedType().getCanonicalType()
              == QT2.getUnqualifiedType().getCanonicalType();
  }

public:
  ASaPType(QualType QT, RplVector *ArgV, Rpl *InRpl = 0, bool Simple = false);
  ASaPType(const ASaPType &T);
  ~ASaPType();

  /// \brief Returns true iff this is of FunctionType.
  bool isFunctionType() const;
  /// \brief Return the size of the ArgsVector.
  int getArgVSize() const;
  /// \brief Return the In RPL which can be null.
  const Rpl *getInRpl() const;
  /// \brief Return the In RPL of this after DerefNum dereferences.
  const Rpl *getInRpl(int DerefNum) const;
  /// \brief Return the Argument for substitution after DerefNum dereferences.
  /// FIXME: support multiple region parameters per class type.
  const Rpl *getSubstArg(int DerefNum = 0) const;
  /// \brief Return the QualType of this ASapType.
  inline QualType getQT() const { return QT; }
  /// \brief Return the QualType of this after DerefNum dereferences.
  QualType getQT(int DerefNum) const;
  /// \brief If this is a function type, return its return type; else null.
  //ASaPType *getReturnType() const;
  /// \brief Dereferences this type DerefNum times.
  void deref(int DerefNum = 1);
  /// \brief Modifies this type as if its address were taken.
  void addrOf(QualType RefQT);
  /// \brief Set the InAnnotation to NULL (and free the Rpl).
  void dropInRpl();
  /// \brief Returns a string describing this ASaPType.
  std::string toString(ASTContext &Ctx) const;
  /// \brief Returns a string describing this ASaPType.
  std::string toString() const;
  /// \brief Returns true when 'this' can be assigned to That
  // Note: That=LHS and this=RHS
  bool isAssignableTo(const ASaPType &That, bool IsInit = false) const;
  /// \brief  true when 'this' is a subtype (derived type) of 'that'.
  bool isSubtypeOf(const ASaPType &That) const;
  /// \brief true when 'this' is a subtype (derived type) of 'that'.
  bool operator <= (const ASaPType &That) const;
  /// \brief Joins this to That (by modifying this).
  /// Join returns the smallest common supertype (Base Type).
  void join(ASaPType *That);
  /// \brief Substitution (ASaPType).
  /// FIXME: use pointer instead of ref type to remove #include "Effect.h".
  void substitute(const SubstitutionVector &SubV);
  /// \brief Performs substitution on type: this[FromEl <- ToRpl].
  void substitute(Substitution &S);
}; // end class ASaPType

} // End namespace asap.
} // End namespace clang.

#endif



