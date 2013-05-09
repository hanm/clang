//=== Substitution.h - Safe Parallelism checker ---*- C++ -*---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This file defines the Substitution and SubstitutionVector classes used
// by the Safe Parallelism checker, which tries to prove the safety of
// parallelism given region and effect annotations.
//
//===----------------------------------------------------------------===//

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_SUBSTITUTION_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_SUBSTITUTION_H

#include "llvm/ADT/SmallVector.h"
#include "ASaPFwdDecl.h"
#include "OwningVector.h"

namespace clang {
namespace asap {

// TODO support substitution over multiple parameters
class Substitution {
private:
  // Fields
  const RplElement *FromEl; // RplElement *not* owned by class
  const Rpl *ToRpl;         // Rpl owned by class
public:
  // Constructors
  Substitution() : FromEl(0),  ToRpl(0) {}

  Substitution(const RplElement *FromEl, const Rpl *ToRpl);

  Substitution(const Substitution &Sub);

  virtual ~Substitution();

  // Getters
  inline const RplElement *getFrom() const { return FromEl; }
  inline const Rpl *getTo() const { return ToRpl; }
  // Setters
  void set(const RplElement *FromEl, const Rpl *ToRpl);

  // Apply
  /// \brief Apply substitution to RPL
  void applyTo(Rpl *R) const;
  void applyTo(Effect *E) const;
  void applyTo(ASaPType *T) const;

  // print
  /// \brief Print Substitution: [From<-To]
  void print(llvm::raw_ostream &OS) const;

  /// \brief Return a string for the Substitution.
  std::string toString() const;
}; // end class Substitution

//////////////////////////////////////////////////////////////////////////
// An ordered sequence of substitutions
#ifndef SUBSTITUTION_VECTOR_SIZE
  #define SUBSTITUTION_VECTOR_SIZE 4
#endif

class SubstitutionVector :
  public OwningVector<Substitution, SUBSTITUTION_VECTOR_SIZE> {

public:
  // Methods
  void buildSubstitutionVector(const ParameterVector *ParV,
                                              RplVector *RplVec);

  // Apply
  void applyTo(Rpl *R) const;

  void applyTo(Effect *Eff) const;

  void applyTo(ASaPType *T) const;
  // Print
  /// \brief Print Substitution vector.
  void print(llvm::raw_ostream &OS) const;

  /// \brief Return a string for the Substitution vector.
  std::string toString() const;

  void push_back_vec(const SubstitutionVector *SubV);

}; // End class SubstituionVector.

} // end namespace clang
} // end namespace asap
#endif
