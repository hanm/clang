//=== Effect.h - Safe Parallelism checker -----*- C++ -*--------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This file defines the Effect and EffectSummary classes used by the Safe
// Parallelism checker, which tries to prove the safety of parallelism
// given region and effect annotations.
//
//===----------------------------------------------------------------===//

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_EFFECT_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_EFFECT_H

#include "llvm/Support/raw_ostream.h"
#include "clang/AST/Attr.h"

#include "ASaPFwdDecl.h"
#include "OwningPtrSet.h"
#include "OwningVector.h"

namespace clang {
namespace asap {

   
class Effect{
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
    EK_AtomicWritesEffect,
    /// invocation effect
    EK_InvocEffect
  };

private:
  
  /// Fields
  EffectKind Kind;
  Rpl* R;
  const Attr* Attribute; // used to get SourceLocation information
  const Expr* Exp;

  //Fiels needed by invocation effects
  SubstitutionVector* SubV;
  FunctionDecl* decl;


  /// \brief returns true if this is a subeffect kind of E.
  /// This method only looks at effect kinds, not their Rpls.
  /// E.g. NoEffect is a subeffect kind of all other effects,
  /// Read is a subeffect of Write. Atomic-X is a subeffect of X.
  /// The Subeffect relation is transitive.
  bool isSubEffectKindOf(const Effect &E) const;

public:
  /// Constructors
  Effect(EffectKind EK, const Rpl* R, const Attr* A = 0);
  Effect(EffectKind EK, const Rpl* R, const Expr* e);
  Effect(const Effect &E);
  Effect(EffectKind EK, const Expr* e, FunctionDecl* FunD, const SubstitutionVector* sv);

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
  inline const Expr* getExp() const {return Exp; }

  
  inline SubstitutionVector* getSubV() const { return SubV; }
  inline FunctionDecl* getDecl() const { return decl; }


  /// \brief substitute (Effect)
  void substitute(const Substitution *S);
  void substitute(const SubstitutionVector *SubV);


  /// \brief SubEffect Rule: true if this <= that
  ///
  ///  RPL1 c= RPL2   E1 c= E2
  /// ~~~~~~~~~~~~~~~~~~~~~~~~~
  ///    E1(RPL1) <= E2(RPL2)
  bool isSubEffectOf(const Effect &That) const;

  /// \brief true iff this # That
  bool isNonInterfering(const Effect &That) const;

  /// \brief Returns covering effect in effect summary or null.
  const Effect *isCoveredBy(const EffectSummary &ES);
}; // end class Effect


 

/////////////////////////////////////////////////////////////////////////////

#ifndef EFFECT_VECTOR_SIZE
#define EFFECT_VECTOR_SIZE 8
#endif
class EffectVector : public OwningVector<Effect, EFFECT_VECTOR_SIZE> {

public:
  void substitute(const Substitution *S) {
    if (!S)
      return; // Nothing to do.
    for (VectorT::const_iterator
            I = begin(),
            E = end();
         I != E; ++I) {
      Effect *Eff = *I;
      Eff->substitute(S);
    }
  }

  void substitute(const SubstitutionVector *SubV) {
    if (!SubV)
      return; // Nothing to do.
    for (VectorT::const_iterator
            I = begin(),
            E = end();
         I != E; ++I) {
      Effect *Eff = *I;
      Eff->substitute(SubV);
      }
  }

  // The following two methods apply the substitutions to the last N
  // elements of the effect vector
  void substitute(const Substitution *S, int N) {
    if (!S)
      return; // Nothing to do.
    unsigned long i=0;
    for (VectorT::const_iterator
	   I = begin(),
	   E = end();
         I != E; ++I) {
      i++;
      if(i>(this->size()-N)){
	Effect *Eff = *I;
	Eff->substitute(S);
      }
      
    }
  }
  
  void substitute(const SubstitutionVector *SubV, int N) {
    if (!SubV)
      return; // Nothing to do.
    unsigned long i=0;
    for (VectorT::const_iterator
	   I = begin(),
	   E = end();
         I != E; ++I) {
      i++;
      if(i>(this->size()-N)){
	Effect *Eff = *I;
	Eff->substitute(SubV);
      }
    }
  }
  

}; // end class EffectVector



/////////////////////////////////////////////////////////////////////////////
/// \brief Implements a Set of Effects
#ifndef EFFECT_SUMMARY_SIZE
  #define EFFECT_SUMMARY_SIZE 8
#endif
class EffectSummary : public OwningPtrSet<Effect, EFFECT_SUMMARY_SIZE> {
public:
  typedef OwningPtrSet<Effect, EFFECT_SUMMARY_SIZE> BaseClass;
  EffectSummary() : BaseClass() {}
  EffectSummary(Effect &E) : BaseClass(E) {}

  /// \brief Returns the effect that covers Eff or null otherwise.
  const Effect *covers(const Effect *Eff) const;
  /// \brief Returns true iff 'this' covers 'Sum'
  bool covers(const EffectSummary *Sum) const;
  /// \brief Returns true iff 'this' is non-interfering with 'Eff'
  bool isNonInterfering(const Effect *Eff) const;
  /// \brief Returns true iff 'this' is non-interfering with 'Sum'
  bool isNonInterfering(const EffectSummary *Sum) const;


  // contains effect pairs (E1, E2) such that E1 is covered by E2.
  typedef llvm::SmallVector<std::pair<const Effect*, const Effect*> *, 8>
    EffectCoverageVector;

  /// \brief Makes effect summary minimal by removing covered effects.
  /// The caller is responsible for deallocating the EffectCoverageVector.
  void makeMinimal(EffectCoverageVector &ECV);

  /// \brief Prints effect summary to raw output stream.
  void print(raw_ostream &OS, std::string Separator="\n",
                              bool PrintLastSeparator=true) const;

  /// \brief Returns a string with the effect summary.
  std::string toString(std::string Separator=", ",
                       bool PrintLastSeparator=false) const;

  void substitute(const Substitution *Sub);
  void substitute(const SubstitutionVector *SubV);

}; // end class EffectSummary


} // end namespace asap
} // end namespace clang

#endif




