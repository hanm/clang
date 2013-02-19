///-///////////////////////////////////////////////////////////////////////////
/// Effect Class
class EffectSummary;

class Effect {
public:
  /// Types
#ifndef EFFECT_VECTOR_SIZE
  #define EFFECT_VECTOR_SIZE 16
#endif
  typedef llvm::SmallVector<Effect*, EFFECT_VECTOR_SIZE> EffectVector;

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

  /// Printing (Effect)
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

  void print(raw_ostream &OS) const {
    bool HasRpl = printEffectKind(OS);
    if (HasRpl) {
      OS << " on ";
      assert(R && "NULL RPL in non-pure effect");
      R->print(OS);
    }
  }

  inline std::string toString() const {
    std::string SBuf;
    llvm::raw_string_ostream OS(SBuf);
    print(OS);
    return std::string(OS.str());
  }

  /// Predicates
  inline bool isNoEffect() const {
    return (Kind == EK_NoEffect) ? true : false;
  }

  inline bool hasRplArgument() const { return !isNoEffect(); }

  inline bool isAtomic() const {
    return (Kind==EK_AtomicReadsEffect ||
            Kind==EK_AtomicWritesEffect) ? true : false;
  }

  /// Getters
  inline EffectKind getEffectKind() const { return Kind; }

  inline const Rpl *getRpl() const { return R; }

  inline const Attr *getAttr() const { return Attribute; }

  inline SourceLocation getLocation() const {
    return Attribute->getLocation();
  }

  /// Substitution (Effect)
  inline void substitute(const RplElement &FromElm, const Rpl &ToRpl) {
    if (R)
      R->substitute(FromElm, ToRpl);
  }

  /// SubEffect: true if this <= e
  /**
   *  rpl1 c= rpl2   E1 c= E2
   * ~~~~~~~~~~~~~~~~~~~~~~~~~
   *    E1(rpl1) <= E2(rpl2)
   */
  bool isSubEffectOf(const Effect &E) const {
    bool Result = (isNoEffect() ||
            (isSubEffectKindOf(E) && R->isIncludedIn(*(E.R))));
    OSv2  << "DEBUG:: ~~~isSubEffect(" << this->toString() << ", "
        << E.toString() << ")=" << (Result ? "true" : "false") << "\n";
    return Result;
  }
  /// isCoveredBy
  inline const Effect *isCoveredBy(const EffectSummary &ES);

}; // end class Effect

/// Implements a Set of Effects
class EffectSummary {
private:
  // Fields
#ifndef EFFECT_SUMMARY_SIZE
#define EFFECT_SUMMARY_SIZE 8
  typedef llvm::SmallPtrSet<const Effect*, EFFECT_SUMMARY_SIZE> EffectSummarySetT;
#undef EFFECT_SUMMARY_SIZE
#endif
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
  /// \brief covering effect E' if E is covered by the effect summary
  inline size_t size() const { return EffectSum.size(); }
  inline const_iterator begin() const { return EffectSum.begin(); }
  inline const_iterator end() const { return EffectSum.end(); }

  /// \brief returns the effect that covers Eff or Null otherwise
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

  inline bool insert(const Effect *Eff) {
    return EffectSum.insert(Eff);
  }

  typedef llvm::SmallVector<std::pair<const Effect*, const Effect*> *, 8>
    EffectCoverageVector;
  /// \brief removes covered effects from summary and adds them to EffCovVec
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
      else       ++I;
    } // end while loop
  }

  void print(raw_ostream &OS, char Separator='\n') const {
    EffectSummarySetT::const_iterator
      I = EffectSum.begin(),
      E = EffectSum.end();
    for(; I != E; ++I) {
      (*I)->print(OS);
      OS << Separator;
    }
  }

  inline std::string toString() const {
    std::string SBuf;
    llvm::raw_string_ostream OS(SBuf);
    print(OS);
    return std::string(OS.str());
  }

}; // end class EffectSummary

inline const Effect *Effect::isCoveredBy(const EffectSummary &ES) {
  return ES.covers(this);
}
