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
#include "ASaPUtil.h"
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
  Rpl *R;
  const Attr *Attribute; // used to get SourceLocation information
  const Expr *Exp;

  //Fields needed by invocation effects
  SubstitutionVector *SubV;
  const FunctionDecl *FunD;


  /// \brief returns true if this is a subeffect kind of E.
  /// This method only looks at effect kinds, not their Rpls.
  /// E.g. NoEffect is a subeffect kind of all other effects,
  /// Read is a subeffect of Write. Atomic-X is a subeffect of X.
  /// The Subeffect relation is transitive.
  bool isSubEffectKindOf(const Effect &E) const;

public:
  /// Constructors
  Effect(EffectKind EK, const Rpl *R, const Attr *A = 0);
  Effect(EffectKind EK, const Rpl *R, const Expr *E);
  Effect(const Effect &E);
  Effect(EffectKind EK, const Expr *E, const FunctionDecl *FunD,
         const SubstitutionVector *SV);

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
  inline const Expr *getExp() const {return Exp; }


  inline SubstitutionVector *getSubV() const { return SubV; }
  inline const FunctionDecl *getDecl() const { return FunD; }


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
  //const Effect *isCoveredBy(const EffectSummary &ES);
  //  EffectSummary::Trivalent isCoveredBy(const EffectSummary &ES);
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
    int i=0;
    for (VectorT::const_reverse_iterator
	   I = rbegin(),
	   E = rend();
         I != E && i<N; ++I, ++i) {
      Effect *Eff = *I;
      Eff->substitute(S);
    }

  }

  void substitute(const SubstitutionVector *SubV, int N) {
    if (!SubV)
      return; // Nothing to do.
    int i=0;
    for (VectorT::const_reverse_iterator
	   I = rbegin(),
	   E = rend();
         I != E && i<N; ++I, ++i) {
      Effect *Eff = *I;
      Eff->substitute(SubV);
    }
  }


}; // end class EffectVector



/////////////////////////////////////////////////////////////////////////////
/// \brief Implements a Set of Effects
class EffectSummary {
public:

  /// Discriminator for LLVM-style RTTI (dyn_cast<> et al.)
  enum SummaryKind {
    ESK_Concrete,
    ESK_Var
  };

private:
  /// \brief Stores the LLVM-style RTTI discriminator
  SummaryKind Kind;

public:
  void setSummaryKind(SummaryKind SK) {Kind = SK;}
  SummaryKind getSummaryKind() const {return Kind;}

  virtual  Trivalent covers(const Effect *Eff) const {return RK_DUNNO;}
  /// \brief Returns true iff 'this' covers 'Sum'
  virtual  Trivalent covers(const EffectSummary *Sum) const {return RK_DUNNO;}
  /// \brief Returns true iff 'this' is non-interfering with 'Eff'
  virtual  Trivalent isNonInterfering(const Effect *Eff) const {return RK_DUNNO;}
  /// \brief Returns true iff 'this' is non-interfering with 'Sum'
  virtual  Trivalent isNonInterfering(const EffectSummary *Sum) const {return RK_DUNNO;}
  typedef llvm::SmallVector<std::pair<const Effect*, const Effect*> *, 8>
      EffectCoverageVector;

  /// \brief Makes effect summary minimal by removing covered effects.
  /// The caller is responsible for deallocating the EffectCoverageVector.
  virtual void makeMinimal(EffectCoverageVector &ECV) = 0;

  virtual void substitute(const Substitution *Sub) = 0;
  virtual void substitute(const SubstitutionVector *SubV) = 0;

  virtual void print(raw_ostream &OS, std::string Separator="\n",
                       bool PrintLastSeparator=true) const = 0;

  std::string toString(std::string Separator=", ",
                       bool PrintLastSeparator=false) const;

  EffectSummary(SummaryKind SK) : Kind(SK) {}
  virtual EffectSummary *clone() const = 0;
  virtual ~EffectSummary() {};
};

#ifndef EFFECT_SUMMARY_SIZE
  #define EFFECT_SUMMARY_SIZE 8
#endif
class ConcreteEffectSummary : public OwningPtrSet<Effect, EFFECT_SUMMARY_SIZE>,
                              public EffectSummary {
public:
  typedef OwningPtrSet<Effect, EFFECT_SUMMARY_SIZE> BaseClass;
  /// Constructors
  ConcreteEffectSummary() : BaseClass(), EffectSummary(ESK_Concrete) {}
  ConcreteEffectSummary(Effect &E)
                       : BaseClass(E), EffectSummary(ESK_Concrete) {}
  ConcreteEffectSummary(const ConcreteEffectSummary &CES)
                       : BaseClass(CES), EffectSummary(ESK_Concrete) {}
  virtual ConcreteEffectSummary *clone() const {
    return new ConcreteEffectSummary(*this);
  }
  ///
  static bool classof(const EffectSummary *ES) {
    return ES->getSummaryKind() == ESK_Concrete;
  }
  /// \brief Returns the effect that covers Eff or null otherwise.
  // const Effect *covers(const Effect *Eff) const;
  virtual Trivalent covers(const Effect *Eff) const;
  /// \brief Returns true iff 'this' covers 'Sum'
  virtual Trivalent covers(const EffectSummary *Sum) const;
  /// \brief Returns true iff 'this' is non-interfering with 'Eff'
  virtual Trivalent isNonInterfering(const Effect *Eff) const;
  /// \brief Returns true iff 'this' is non-interfering with 'Sum'
  virtual Trivalent isNonInterfering(const EffectSummary *Sum) const;

  /// \brief Makes effect summary minimal by removing covered effects.
  /// The caller is responsible for deallocating the EffectCoverageVector.
  virtual void makeMinimal(EffectCoverageVector &ECV);

  /// \brief Prints effect summary to raw output stream.
  virtual void print(raw_ostream &OS, std::string Separator="\n",
	     bool PrintLastSeparator=true) const;

  virtual void substitute(const Substitution *Sub);
  virtual void substitute(const SubstitutionVector *SubV);

  virtual ~ConcreteEffectSummary() {};
}; // end class ConcreteEffectSummary

class VarEffectSummary : public EffectSummary {
  // TODO: add field to store solution of inference
  // ConcreteEffectSummary* Concrete;  // Commented out to remove warning.
public:
  // Constructors
  VarEffectSummary() : EffectSummary(ESK_Var) {}

  VarEffectSummary(const VarEffectSummary &VES)
                  : EffectSummary(ESK_Var) {}

  virtual VarEffectSummary *clone() const {
    return new VarEffectSummary(*this);
  }

  static bool classof(const EffectSummary *ES) {
    return ES->getSummaryKind() == ESK_Var;
  }
  virtual void print(raw_ostream &OS, std::string Separator="\n",
		     bool PrintLastSeparator=true) const;
  virtual void makeMinimal(EffectCoverageVector &ECV) {
    //nothing to do
  }

  virtual void substitute(const Substitution *Sub) {
     //TODO
     //Store these subtitutions?
  }
  virtual void substitute(const SubstitutionVector *SubV) {
     //TODO
     //Store these subtitutions?
  }

  virtual ~VarEffectSummary() {};
}; // end class VarEffectSummary


} // end namespace asap
} // end namespace clang

#endif




