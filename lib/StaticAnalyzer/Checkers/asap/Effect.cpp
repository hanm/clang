//=== Effect.cpp - Safe Parallelism checker -----*- C++ -*-----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This files defines the Effect, EffectVector, and EffectSummary classes
// used by the Safe Parallelism checker, which tries to prove the safety
// of parallelism given region and effect annotations.
//
//===----------------------------------------------------------------===//

///-///////////////////////////////////////////////////////////////////////////
/// Effect Class
class EffectSummary;

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
  inline bool isSubEffectKindOf(const Effect &E) const {
    if (Kind == EK_NoEffect) return true; // optimization

    bool Result = false;
    if (!E.isAtomic() || this->isAtomic()) {
      /// if e.isAtomic ==> this->isAtomic [[else return false]]
      switch(E.getEffectKind()) {
      case EK_WritesEffect:
        if (Kind == EK_WritesEffect) Result = true;
        // intentional fall through (lack of 'break')
      case EK_AtomicWritesEffect:
        if (Kind == EK_AtomicWritesEffect) Result = true;
        // intentional fall through (lack of 'break')
      case EK_ReadsEffect:
        if (Kind == EK_ReadsEffect) Result = true;
        // intentional fall through (lack of 'break')
      case EK_AtomicReadsEffect:
        if (Kind == EK_AtomicReadsEffect) Result = true;
        // intentional fall through (lack of 'break')
      case EK_NoEffect:
        if (Kind == EK_NoEffect) Result = true;
      }
    }
    return Result;
  }

public:
  /// Constructors
  Effect(EffectKind EK, const Rpl* R, const Attr* A = 0)
        : Kind(EK), Attribute(A) {
    this->R = (R) ? new Rpl(*R) : 0;
  }

  Effect(const Effect &E): Kind(E.Kind), Attribute(E.Attribute) {
    R = (E.R) ? new Rpl(*E.R) : 0;
  }
  /// Destructors
  ~Effect() {
    delete R;
  }

  /// \brief Print Effect Kind to raw output stream.
  inline bool printEffectKind(raw_ostream &OS) const {
    bool HasRpl = true;
    switch(Kind) {
    case EK_NoEffect: OS << "Pure Effect"; HasRpl = false; break;
    case EK_ReadsEffect: OS << "Reads Effect"; break;
    case EK_WritesEffect: OS << "Writes Effect"; break;
    case EK_AtomicReadsEffect: OS << "Atomic Reads Effect"; break;
    case EK_AtomicWritesEffect: OS << "Atomic Writes Effect"; break;
    }
    return HasRpl;
  }
  /// \brief Print Effect to raw output stream.
  void print(raw_ostream &OS) const {
    bool HasRpl = printEffectKind(OS);
    if (HasRpl) {
      OS << " on ";
      assert(R && "NULL RPL in non-pure effect");
      R->print(OS);
    }
  }
  /// \brief Return a string for this Effect.
  inline std::string toString() const {
    std::string SBuf;
    llvm::raw_string_ostream OS(SBuf);
    print(OS);
    return std::string(OS.str());
  }

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
  bool isSubEffectOf(const Effect &That) const {
    bool Result = (isNoEffect() ||
            (isSubEffectKindOf(That) && R->isIncludedIn(*(That.R))));
    OSv2  << "DEBUG:: ~~~isSubEffect(" << this->toString() << ", "
        << That.toString() << ")=" << (Result ? "true" : "false") << "\n";
    return Result;
  }
  /// \brief Returns covering effect in effect summary or null.
  inline const Effect *isCoveredBy(const EffectSummary &ES);

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
  const Effect *covers(const Effect *Eff) const {
    if (EffectSum.count(Eff))
      return Eff;

    EffectSummarySetT::const_iterator
      I = EffectSum.begin(),
      E = EffectSum.end();
    for(; I != E; ++I) {
      if (Eff->isSubEffectOf(*(*I)))
        return *I;
    }
    return 0;
  }
  /// \brief Returns true iff insertion was successful.
  inline bool insert(const Effect *Eff) {
    return EffectSum.insert(Eff);
  }

  typedef llvm::SmallVector<std::pair<const Effect*, const Effect*> *, 8>
    EffectCoverageVector;
  /// \brief Makes effect summary minimal by removing covered effects.
  /// The caller is responsible for deallocating the EffectCoverageVector.
  void makeMinimal(EffectCoverageVector &ECV) {
    EffectSummarySetT::iterator I = EffectSum.begin(); // not a const iterator
    while (I != EffectSum.end()) { // EffectSum.end() is not loop invariant
      bool found = false;
      for (EffectSummarySetT::iterator
            J = EffectSum.begin(); J != EffectSum.end(); ++J) {
        if (I != J && (*I)->isSubEffectOf(*(*J))) {
          //emitEffectCovered(D, *I, *J);
          ECV.push_back(new std::pair<const Effect*, const Effect*>(*I, *J));
          found = true;
          break;
        } // end if
      } // end inner for loop
      /// optimization: remove e from effect Summary
      if (found) {
        bool Success = EffectSum.erase(*I);
        assert(Success);
        I = EffectSum.begin();
      }
      else
        ++I;
    } // end while loop
  }
  /// \brief Prints effect summary to raw output stream.
  void print(raw_ostream &OS, char Separator='\n') const {
    EffectSummarySetT::const_iterator
      I = EffectSum.begin(),
      E = EffectSum.end();
    for(; I != E; ++I) {
      (*I)->print(OS);
      OS << Separator;
    }
  }
  /// \brief Returns a string with the effect summary.
  inline std::string toString() const {
    std::string SBuf;
    llvm::raw_string_ostream OS(SBuf);
    print(OS);
    return std::string(OS.str());
  }

}; // end class EffectSummary

static const Effect *WritesLocal = new Effect(Effect::EK_WritesEffect,
                                              new Rpl(*LOCALRplElmt));

inline const Effect *Effect::isCoveredBy(const EffectSummary &ES) {
  bool Result = this->isSubEffectOf(*WritesLocal);
  if (Result)
    return WritesLocal;
  else
    return ES.covers(this);
}
