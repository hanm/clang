//=== Effect.h - Safe Parallelism checker -----*- C++ -*--------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This files defines the Effect and EffectSummary classes used by the Safe
// Parallelism checker, which tries to prove the safety of parallelism
// given region and effect annotations.
//
//===----------------------------------------------------------------===//

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_EFFECT_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_EFFECT_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/AST/Attr.h"

namespace clang {
namespace asap {

class Rpl;
class RplElement;
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
  // Constructor
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
    assert(R);
    for(SubstitutionVecT::const_iterator I = SubV.begin(), E = SubV.end();
        I != E; ++I) {
      assert(*I);
      (*I)->applyTo(R);
    }
  }
  // Print
  /// \brief Print Substitution vector.
  void print(raw_ostream &OS) const {
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
}; // End class Substituion.

class Effect {
public:
  enum EffectKind {
    /// pure = no effect
    EK_NoEffect,
    /// reads effect
    EK_ReadsEffect,
    /// atomic reads effect
    EK_AtomicReadsEffect,
    /// writes effect
    EK_WritesEffect,
    /// atomic writes effect
    EK_AtomicWritesEffect
  };

private:
  /// Fields
  EffectKind Kind;
  Rpl* R;
  const Attr* Attribute; // used to get SourceLocation information

  /// \brief returns true if this is a subeffect kind of E.
  /// This method only looks at effect kinds, not their Rpls.
  /// E.g. NoEffect is a subeffect kind of all other effects,
  /// Read is a subeffect of Write. Atomic-X is a subeffect of X.
  /// The Subeffect relation is transitive.
  bool isSubEffectKindOf(const Effect &E) const;

public:
  /// Constructors
  Effect(EffectKind EK, const Rpl* R, const Attr* A = 0);
  Effect(const Effect &E);

  /// Destructors
  ~Effect();

  /// \brief Print Effect Kind to raw output stream.
  bool printEffectKind(llvm::raw_ostream &OS) const;

  /// \brief Print Effect to raw output stream.
  void print(llvm::raw_ostream &OS) const;

  /// \brief Return a string for this Effect.
  std::string toString() const;

  // Predicates
  /// \brief Returns true iff this is a no_effect.
  inline bool isNoEffect() const {
    return (Kind == EK_NoEffect) ? true : false;
  }

  /// \brief Returns true iff this effect has RPL argument.
  inline bool hasRplArgument() const { return !isNoEffect(); }
  /// \brief Returns true if this effect is atomic.

  inline bool isAtomic() const {
    return (Kind==EK_AtomicReadsEffect ||
            Kind==EK_AtomicWritesEffect) ? true : false;
  }

  // Getters
  /// \brief Return effect kind.
  inline EffectKind getEffectKind() const { return Kind; }
  /// \brief Return RPL (which may be null for no_effect).
  inline const Rpl *getRpl() const { return R; }
  /// \brief Return corresponding Attribute.
  inline const Attr *getAttr() const { return Attribute; }
  /// \brief Return source location.
  inline SourceLocation getLocation() const {
    return Attribute->getLocation();
  }

  /// \brief substitute (Effect)
  inline void substitute(const Substitution &S) {
    if (R)
      S.applyTo(R);
  }


  /// \brief SubEffect Rule: true if this <= e
  ///
  ///  RPL1 c= RPL2   E1 c= E2
  /// ~~~~~~~~~~~~~~~~~~~~~~~~~
  ///    E1(RPL1) <= E2(RPL2)
  bool isSubEffectOf(const Effect &That) const;

  /// \brief Returns covering effect in effect summary or null.
  const Effect *isCoveredBy(const EffectSummary &ES);
}; // end class Effect

/////////////////////////////////////////////////////////////////////////////
class EffectVector {
public:
  // Types
#ifndef EFFECT_VECTOR_SIZE
  #define EFFECT_VECTOR_SIZE 16
#endif
  typedef llvm::SmallVector<Effect*, EFFECT_VECTOR_SIZE> EffectVectorT;

private:
  // Fields
  EffectVectorT EffV;

public:
  /// Constructor
  EffectVector() {}
  EffectVector(const Effect &E) {
    EffV.push_back(new Effect(E));
  }

  /// Destructor
  ~EffectVector() {
    for (EffectVectorT::const_iterator
            I = EffV.begin(),
            E = EffV.end();
         I != E; ++I) {
      delete (*I);
    }
  }

  // Methods
  /// \brief Return an iterator at the first Effect of the vector.
  inline EffectVectorT::iterator begin() { return EffV.begin(); }
  /// \brief Return an iterator past the last Effect of the vector.
  inline EffectVectorT::iterator end() { return EffV.end(); }
  /// \brief Return a const_iterator at the first Effect of the vector.
  inline EffectVectorT::const_iterator begin () const { return EffV.begin(); }
  /// \brief Return a const_iterator past the last Effect of the vector.
  inline EffectVectorT::const_iterator end () const { return EffV.end(); }
  /// \brief Return the size of the Effect vector.
  inline size_t size () const { return EffV.size(); }
  /// \brief Append the argument Effect to the Effect vector.
  inline void push_back (const Effect *E) {
    assert(E);
    EffV.push_back(new Effect(*E));
  }
  /// \brief Return the last Effect in the vector after removing it.
  inline Effect *pop_back_val() { return EffV.pop_back_val(); }

  void substitute(const Substitution &S) {
    for (EffectVectorT::const_iterator
            I = EffV.begin(),
            E = EffV.end();
         I != E; ++I) {
      Effect *Eff = *I;
      Eff->substitute(S);
    }
  }

}; // end class EffectVector

/////////////////////////////////////////////////////////////////////////////
/// \brief Implements a Set of Effects
class EffectSummary {
private:
  // Fields
#ifndef EFFECT_SUMMARY_SIZE
#define EFFECT_SUMMARY_SIZE 8
#endif
  typedef llvm::SmallPtrSet<const Effect*, EFFECT_SUMMARY_SIZE>
    EffectSummarySetT;
  EffectSummarySetT EffectSum;
public:
  // Constructor
  // Destructor
  ~EffectSummary() {
    EffectSummarySetT::const_iterator
      I = EffectSum.begin(),
      E = EffectSum.end();
    for(; I != E; ++I) {
      delete *I;
    }
  }

  // Types
  typedef EffectSummarySetT::const_iterator const_iterator;

  // Methods
  /// \brief Returns the size of the EffectSummary
  inline size_t size() const { return EffectSum.size(); }
  /// \brief Returns a const_iterator at the first element of the summary.
  inline const_iterator begin() const { return EffectSum.begin(); }
  /// \brief Returns a const_iterator past the last element of the summary.
  inline const_iterator end() const { return EffectSum.end(); }

  /// \brief Returns the effect that covers Eff or null otherwise.
  const Effect *covers(const Effect *Eff) const;

  /// \brief Returns true iff insertion was successful.
  inline bool insert(const Effect *Eff) {
    return EffectSum.insert(Eff);
  }

  typedef llvm::SmallVector<std::pair<const Effect*, const Effect*> *, 8>
    EffectCoverageVector;

  /// \brief Makes effect summary minimal by removing covered effects.
  /// The caller is responsible for deallocating the EffectCoverageVector.
  void makeMinimal(EffectCoverageVector &ECV);

  /// \brief Prints effect summary to raw output stream.
  void print(raw_ostream &OS, char Separator='\n') const;

  /// \brief Returns a string with the effect summary.
  std::string toString() const;
}; // end class EffectSummary

}
}

#endif




