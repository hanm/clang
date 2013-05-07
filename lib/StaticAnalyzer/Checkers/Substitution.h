//=== Substitution.h - Safe Parallelism checker ---*- C++ -*---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This files defines the Substitution and SubstitutionVector classes used
// by the Safe Parallelism checker, which tries to prove the safety of
// parallelism given region and effect annotations.
//
//===----------------------------------------------------------------===//

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_SUBSTITUTION_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_SUBSTITUTION_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"

namespace clang {
namespace asap {

class Rpl;
class RplElement;
class RplVector;
class ParameterVector;
class Effect;
class EffectSummary;

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

  // print
  /// \brief Print Substitution: [From<-To]
  void print(llvm::raw_ostream &OS) const;

  /// \brief Return a string for the Substitution.
  std::string toString() const;
}; // end class Substitution

// An ordered sequence of substitutions
class SubstitutionVector {
public:
  // Type
#ifndef SUBSTITUTION_VECTOR_SIZE
  #define SUBSTITUTION_VECTOR_SIZE 4
#endif
  typedef llvm::SmallVector<Substitution*, SUBSTITUTION_VECTOR_SIZE>
    SubstitutionVecT;
private:
  SubstitutionVecT SubV;
public:
  // Constructors & Destructor
  SubstitutionVector() {}

  SubstitutionVector(Substitution *S) {
    if (S)
      SubV.push_back(new Substitution(*S));
  }

  virtual ~SubstitutionVector() {
    for (SubstitutionVecT::const_iterator I = SubV.begin(), E = SubV.end();
         I != E; ++I) {
      delete(*I);
    }
  }
  // Methods
  void buildSubstitutionVector(const ParameterVector *ParV,
                                              RplVector *RplVec);

  /// \brief Return an iterator at the first RPL of the vector.
  inline SubstitutionVecT::iterator begin () { return SubV.begin(); }
  /// \brief Return an iterator past the last RPL of the vector.
  inline SubstitutionVecT::iterator end () { return SubV.end(); }
  /// \brief Return a const_iterator at the first RPL of the vector.
  inline SubstitutionVecT::const_iterator begin () const { return SubV.begin();}
  /// \brief Return a const_iterator past the last RPL of the vector.
  inline SubstitutionVecT::const_iterator end () const { return SubV.end(); }
  /// \brief Return the size of the RPL vector.
  inline size_t size () const { return SubV.size(); }
  /// \brief Append the argument Substitution to the Substitution vector.
  inline void push_back(Substitution *Sub) {
    if (Sub)
      SubV.push_back(new Substitution(*Sub));
  }

  // Apply
  void applyTo(Rpl *R) const {
    if (R) {
      for(SubstitutionVecT::const_iterator I = SubV.begin(), E = SubV.end();
          I != E; ++I) {
        assert(*I);
        (*I)->applyTo(R);
      }
    }
  }

  void applyTo(Effect *Eff) const {
    if (Eff) {
      for(SubstitutionVecT::const_iterator I = SubV.begin(), E = SubV.end();
          I != E; ++I) {
        assert(*I);
        (*I)->applyTo(Eff);
      }
    }
  }
  // Print
  /// \brief Print Substitution vector.
  void print(llvm::raw_ostream &OS) const {
    SubstitutionVecT::const_iterator
      I = SubV.begin(),
      E = SubV.end();
    for(; I != E; ++I) {
      assert(*I);
      (*I)->print(OS);
    }
  }

  /// \brief Return a string for the Substitution vector.
  inline std::string toString() const {
    std::string SBuf;
    llvm::raw_string_ostream OS(SBuf);
    print(OS);
    return std::string(OS.str());
  }

  void push_back(const SubstitutionVector *SubV);

}; // End class SubstituionVector.

} // end namespace clang
} // end namespace asap
#endif
