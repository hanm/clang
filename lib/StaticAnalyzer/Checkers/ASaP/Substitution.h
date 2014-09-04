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

#include <SWI-Prolog.h>

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

  /// \brief Build a Prolog term_t for the Substitution object
  term_t getPLTerm() const;

  // print
  /// \brief Print Substitution: [From<-To]
  void print(llvm::raw_ostream &OS) const;

  /// \brief Return a string for the Substitution.
  std::string toString() const;

  bool hasBase(const RplElement &Base) const;

}; // end class Substitution

//////////////////////////////////////////////////////////////////////////
// An unordered sequence of substitutions -- only one substitution will
// succeed per RPL
#ifndef SUBSTITUTION_SET_SIZE
  #define SUBSTITUTION_SET_SIZE 4
#endif
class SubstitutionSet :
  public OwningPtrSet<Substitution, SUBSTITUTION_SET_SIZE> {

public:
  typedef OwningPtrSet<Substitution, SUBSTITUTION_SET_SIZE> BaseClass;

  void buildSubstitutionSet(const ParameterVector *ParV,
                            const RplVector *RplVec);

  /// \brief Print Substitution vector.
  void print(llvm::raw_ostream &OS) const;

  /// \brief Return a string for the Substitution vector.
  std::string toString() const;

  void applyTo(ConcreteRpl &R) const;

  term_t getPLTerm() const;

  bool hasBase(const RplElement &Base) const;
}; // end class SubstitutionSet

//////////////////////////////////////////////////////////////////////////
// An ordered sequence of substitution sets
#ifndef SUBSTITUTION_VECTOR_SIZE
  #define SUBSTITUTION_VECTOR_SIZE 4
#endif

class SubstitutionVector :
  public OwningVector<SubstitutionSet, SUBSTITUTION_VECTOR_SIZE> {

public:
  // Methods
  /// \brief Apply substitution vector to type T, which must implement
  /// T->substitute(Subtitution *)
  template<typename T>
  void applyTo(T *X) const {
    if (X) {
      for(VectorT::const_iterator I = begin(), E = end();
          I != E; ++I) {
        assert(*I);
        X->substitute(*I);
      }
    }
  }

  template<typename T>
  void reverseApplyTo(T *X) const {
    if (X) {
      for(VectorT::const_reverse_iterator I = rbegin(), E = rend();
          I != E; ++I) {
        assert(*I);
        X->substitute(*I);
      }
    }
  }

  void push_back_vec(const SubstitutionVector *SubV);
  void push_back_vec(std::unique_ptr<SubstitutionVector> &SubV);

  term_t getPLTerm() const;

}; // End class SubstituionVector.

} // end namespace clang
} // end namespace asap
#endif
