
  /**
   *  Return true when the input string is a special RPL element
   *  (e.g., '*', '?', 'Root'.
   */
  // TODO (?): '?', 'Root'
  bool isSpecialRplElement(const StringRef& s) {
    if (!s.compare("*"))
      return true;
    else
      return false;
  }

  /**
   *  Return true when the input string is a valid region
   *  name or region parameter declaration
   */
  bool isValidRegionName(const StringRef& s) {
    // false if it is one of the Special Rpl Elements
    // => it is not allowed to redeclare them
    if (isSpecialRplElement(s)) return false;

    // must start with [_a-zA-Z]
    const char c = s.front();
    if (c != '_' &&
        !( c >= 'a' && c <= 'z') &&
        !( c >= 'A' && c <= 'Z'))
      return false;
    // all remaining characters must be in [_a-zA-Z0-9]
    for (size_t i=0; i < s.size(); i++) {
      const char c = s[i];
      if (c != '_' &&
          !( c >= 'a' && c <= 'z') &&
          !( c >= 'A' && c <= 'Z') &&
          !( c >= '0' && c <= '9'))
        return false;
    }

    return true;
  }

///-///////////////////////////////////////////////////////////////////////////
/// RplElement Class Hierarchy

class Rpl;

class RplElement {
public:
  /// Type
  enum RplElementKind {
      RK_Special,
      RK_Star,
      RK_Named,
      RK_Parameter,
      RK_Capture
  };
private:
  const RplElementKind Kind;

public:
  /// API
  RplElementKind getKind() const { return Kind; }
  RplElement(RplElementKind K) : Kind(K) {}

  virtual bool isFullySpecified() const { return true; }

  virtual StringRef getName() const = 0;
  virtual bool operator == (const RplElement& that) const = 0;
  //virtual RplElement* clone() const = 0;
  virtual ~RplElement() {}

};

///-////////////////////////////////////////
// Root & Local
class SpecialRplElement : public RplElement {
  /// Fields
  const StringRef name;

public:
  /// Constructor
  SpecialRplElement(StringRef name) : RplElement(RK_Special), name(name) {}
  virtual ~SpecialRplElement() {}

  /// Methods
  virtual StringRef getName() const { return name; }
  //virtual SpecialRplElement* clone const { return new SpecialRplElement(this);}
  virtual bool operator == (const RplElement& that) const {
    return (isa<const SpecialRplElement>(that)
            && this->getName().compare(that.getName()) == 0) ? true : false;
  }

  static bool classof(const RplElement *R) {
    return R->getKind() == RK_Special;
  }
};


static const SpecialRplElement *ROOT_RplElmt = new SpecialRplElement("Root");
static const SpecialRplElement *LOCAL_RplElmt = new SpecialRplElement("Local");


///-////////////////////////////////////////
class StarRplElement : public RplElement {

public:
  /// Constructor/Destructor
  StarRplElement () : RplElement(RK_Star) {}
  virtual ~StarRplElement() {}

  /// Methods
  virtual bool isFullySpecified() const { return false; }
  virtual StringRef getName() const { return "*"; }
  virtual bool operator == (const RplElement &that) const {
    return (isa<const StarRplElement>(that)
            && this==&that) ? true : false;
  }
  static bool classof(const RplElement *R) {
    return R->getKind() == RK_Star;
  }
};

static const StarRplElement *STAR_RplElmt = new StarRplElement();

/// \brief returns a special RPL element (Root, Local, *, ...) or NULL
const RplElement* getSpecialRplElement(const StringRef& s) {
  if (!s.compare(STAR_RplElmt->getName()))
    return STAR_RplElmt;
  else if (!s.compare(ROOT_RplElmt->getName()))
    return ROOT_RplElmt;
  else if (!s.compare(LOCAL_RplElmt->getName()))
    return LOCAL_RplElmt;
  else
    return 0;
}

///-////////////////////////////////////////
class NamedRplElement : public RplElement {
  /// Fields
  const StringRef name;

public:
  /// Constructor
  NamedRplElement(StringRef name) : RplElement(RK_Named), name(name) {}
  virtual ~NamedRplElement() {}
  /// Methods
  virtual StringRef getName() const { return name; }
  virtual bool operator == (const RplElement &that) const {
    return (isa<const NamedRplElement>(that)
            && this->getName().compare(that.getName()) == 0) ? true: false;
  }

  static bool classof(const RplElement *R) {
    return R->getKind() == RK_Named;
  }

};

///-////////////////////////////////////////
class ParamRplElement : public RplElement {
  ///Fields
  StringRef name;

public:
  /// Constructor
  ParamRplElement(StringRef name) : RplElement(RK_Parameter), name(name) {}
  virtual ~ParamRplElement() {}

  /// Methods
  virtual StringRef getName() const { return name; }
  virtual bool operator == (const RplElement& that) const {
    return (isa<const ParamRplElement>(that)
            && this->getName().compare(that.getName()) == 0) ? true : false;
  }
  static bool classof(const RplElement *R) {
    return R->getKind() == RK_Parameter;
  }
};

///-////////////////////////////////////////
class CaptureRplElement : public RplElement {
  /// Fields
  Rpl& includedIn;

public:
  /// Constructor
  CaptureRplElement(Rpl& includedIn) : RplElement(RK_Capture),
                                       includedIn(includedIn)
  { /*assert(includedIn.isFullySpecified() == false);*/ }
  virtual ~CaptureRplElement() {}
  /// Methods
  virtual StringRef getName() const { return "rho"; }

  virtual bool operator == (const RplElement &that) const {
    return (isa<const CaptureRplElement>(that) && this==&that) ? true : false;
  }
  virtual bool isFullySpecified() const { return false; }

  Rpl& upperBound() const { return includedIn; }

  static bool classof(const RplElement *R) {
    return R->getKind() == RK_Capture;
  }

};

#ifndef RPL_ELEMENT_VECTOR_SIZE
  #define RPL_ELEMENT_VECTOR_SIZE 8
#endif
typedef llvm::SmallVector<const RplElement*,
                          RPL_ELEMENT_VECTOR_SIZE> RplElementVector;

///-///////////////////////////////////////////////////////////////////////////
/// Rpl Class
class Rpl {
  friend class Rpl;
public:
  /// Types
#ifndef RPL_VECTOR_SIZE
  #define RPL_VECTOR_SIZE 4
#endif
  typedef llvm::SmallVector<Rpl*, RPL_VECTOR_SIZE> RplVector;

private:
  /// Fields
  StringRef RplString;
  /// Note: RplElements are not owned by Rpl class.
  /// They are *NOT* destroyed with the Rpl.
  RplElementVector RplElements;
  bool FullySpecified;

  /// RplRef class
  // We use the RplRef class, friends with Rpl to efficiently perform
  // isIncluded and isUnder tests
  class RplRef {
    long firstIdx;
    long lastIdx;
    const Rpl& rpl;

  public:
    /// Constructor
    RplRef(const Rpl& r) : rpl(r) {
      firstIdx = 0;
      lastIdx = rpl.RplElements.size()-1;
    }
    /// Printing
    void printElements(raw_ostream& os) {
      int i = firstIdx;
      for (; i<lastIdx; i++) {
        os << rpl.RplElements[i]->getName() << ":";
      }
      // print last element
      if (i==lastIdx)
        os << rpl.RplElements[i]->getName();
    }

    std::string toString() {
      std::string sbuf;
      llvm::raw_string_ostream os(sbuf);
      printElements(os);
      return std::string(os.str());
    }

    /// Getters
    const RplElement* getFirstElement() {
      return rpl.RplElements[firstIdx];
    }
    const RplElement* getLastElement() {
      return rpl.RplElements[lastIdx];
    }

    RplRef& stripLast() {
      lastIdx--;
      return *this;
    }
    inline bool isEmpty() {
      //os  << "DEBUG:: isEmpty[RplRef] returned "
      //    << (lastIdx<firstIdx ? "true" : "false") << "\n";
      return (lastIdx<firstIdx) ? true : false;
    }

    bool isUnder(RplRef& rhs) {
      osv2  << "DEBUG:: ~~~~~~~~isUnder[RplRef]("
          << this->toString() << ", " << rhs.toString() << ")\n";
      /// R <= Root
      if (rhs.isEmpty())
        return true;
      if (isEmpty()) /// and rhs is not Empty
        return false;
      /// R <= R' <== R c= R'
      if (isIncludedIn(rhs)) return true;
      /// R:* <= R' <== R <= R'
      if (getLastElement() == STAR_RplElmt)
        return stripLast().isUnder(rhs);
      /// R:r <= R' <==  R <= R'
      /// R:[i] <= R' <==  R <= R'
      return stripLast().isUnder(rhs);
      // TODO z-regions
    }

    /// Inclusion: this c= rhs
    bool isIncludedIn(RplRef& rhs) {
      osv2  << "DEBUG:: ~~~~~~~~isIncludedIn[RplRef]("
          << this->toString() << ", " << rhs.toString() << ")\n";
      if (rhs.isEmpty()) {
        /// Root c= Root
        if (isEmpty()) return true;
        /// RPL c=? Root and RPL!=Root ==> not included
        else /*!isEmpty()*/ return false;
      } else { /// rhs is not empty
        /// R c= R':* <==  R <= R'
        if (*rhs.getLastElement() == *STAR_RplElmt) {
          osv2 <<"DEBUG:: isIncludedIn[RplRef] last elmt of RHS is '*'\n";
          return isUnder(rhs.stripLast());
        }
        ///   R:r c= R':r    <==  R <= R'
        /// R:[i] c= R':[i]  <==  R <= R'
        if (!isEmpty()) {
          if ( *getLastElement() == *rhs.getLastElement() )
            return stripLast().isIncludedIn(rhs.stripLast());
        }
        return false;
      }
    }
  }; /// end class RplRef
  ///////////////////////////////////////////////////////////////////////////

public:
  static const char RPL_SPLIT_CHARACTER = ':';

  /// Constructors
  /*Rpl(StringRef rpl):rpl(rpl) {
    FullySpecified = true;
    while(rpl.size() > 0) { /// for all RPL elements of the RPL
      // FIXME: '::' can appear as part of an RPL element. Splitting must
      // be done differently to account for that.
      std::pair<StringRef,StringRef> pair = rpl.split(RPL_SPLIT_CHARACTER);

      RplElements.push_back(new NamedRplElement(pair.first));
      if (isSpecialRplElement(pair.first))
        FullySpecified = false;
      rpl = pair.second;
    }
  }*/

  Rpl() : FullySpecified(true) {}

  Rpl(RplElement *Elm) :
    RplString(Elm->getName()),
    FullySpecified(Elm->isFullySpecified())
  {
    RplElements.push_back(Elm);
  }

  /// Copy Constructor
  Rpl(Rpl* That) :
      RplString(That->RplString),
      RplElements(That->RplElements),
      FullySpecified(That->FullySpecified)
  {}

  /// Destructors
  //~Rpl() {}

  /// Printing
  void printElements(raw_ostream& os) const {
    RplElementVector::const_iterator I = RplElements.begin();
    RplElementVector::const_iterator E = RplElements.end();
    for (; I < E-1; I++) {
      os << (*I)->getName() << Rpl::RPL_SPLIT_CHARACTER;
    }
    // print last element
    if (I==E-1)
      os << (*I)->getName();
  }

  std::string toString() const {
    std::string sbuf;
    llvm::raw_string_ostream os(sbuf);
    printElements(os);
    return std::string(os.str());
  }

  /// Getters
  inline const RplElement* getLastElement() {
    return RplElements.back();
  }

  inline const RplElement* getFirstElement() {
    return RplElements.front();
  }

  /** returns the number of RPL elements */
  inline size_t length() {
    return RplElements.size();
  }
  /// Setters
  inline void appendElement(const RplElement* rplElm) {
    RplElements.push_back(rplElm);
    if (rplElm->isFullySpecified() == false)
      FullySpecified = false;
  }
  /// Predicates
  inline bool isFullySpecified() { return FullySpecified; }
  inline bool isEmpty() { return RplElements.empty(); }

  /// Nesting (Under)
  bool isUnder(const Rpl& rhsRpl) const {
    const CaptureRplElement *cap = dyn_cast<CaptureRplElement>(RplElements.front());
    if (cap) {
      cap->upperBound().isUnder(rhsRpl);
    }

    RplRef* lhs = new RplRef(*this);
    RplRef* rhs = new RplRef(rhsRpl);
    bool result = lhs->isIncludedIn(*rhs);
    delete lhs; delete rhs;
    return result;
  }
  /// Inclusion
  bool isIncludedIn(const Rpl& rhsRpl) const {
    const CaptureRplElement *cap = dyn_cast<CaptureRplElement>(RplElements.front());
    if (cap) {
      cap->upperBound().isIncludedIn(rhsRpl);
    }

    RplRef* lhs = new RplRef(*this);
    RplRef* rhs = new RplRef(rhsRpl);
    bool result = lhs->isIncludedIn(*rhs);
    delete lhs; delete rhs;
    osv2 << "DEBUG:: ~~~~~ isIncludedIn[RPL](" << this->toString() << ", "
        << rhsRpl.toString() << ")=" << (result ? "true" : "false") << "\n";
    return result;
  }

  /// Substitution (Rpl)
  bool substitute(const RplElement& From, const Rpl& To) {
    os << "DEBUG:: before substitution(" << From.getName() << "<-";
    To.printElements(os);
    os <<"): ";
    assert(RplElements.size()>0);
    printElements(os);
    os << "\n";
    /// A parameter is only allowed at the head of an Rpl
    RplElementVector::iterator I = RplElements.begin();
    if (*(*I) == From) {
      osv2 << "DEBUG:: found '" << From.getName()
        << "' replaced with '" ;
      To.printElements(osv2);
      I = RplElements.erase(I);
      I = RplElements.insert(I, To.RplElements.begin(), To.RplElements.end());
      osv2 << "' == '";
      printElements(osv2);
      osv2 << "'\n";
    }
    os << "DEBUG:: after substitution(" << From.getName() << "<-";
    To.printElements(os);
    os << "): ";
    printElements(os);
    os << "\n";
    return false;
  }

  /// Append to this Rpl the argument Rpl without its head element
  inline void appendRplTail(Rpl* That) {
    if (That->length()>1)
      RplElements.append(That->length()-1, (*(That->RplElements.begin() + 1)));
  }

  Rpl* upperBound() {
    if (isEmpty() || !isa<CaptureRplElement>(RplElements.front()))
      return this;
    // else
    Rpl* upperBound = & dyn_cast<CaptureRplElement>(RplElements.front())->upperBound();
    upperBound->appendRplTail(this);
    return upperBound;
  }

  /// Capture
  /// TODO: caller must deallocate Rpl and its element
  inline Rpl* capture() {
    if (this->isFullySpecified()) return this;
    else return new Rpl(new CaptureRplElement(*this));
  }

}; // end class Rpl

///-///////////////////////////////////////////////////////////////////////////
/// Effect Class


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
  const Attr* A; // used to get SourceLocation information

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
  Effect(EffectKind EK, Rpl* R, const Attr* A)
        : Kind(EK), R(R), A(A) {}

  Effect(Effect *E): Kind(E->Kind), A(E->A) {
    R = (E->R) ? new Rpl(E->R) : 0;
  }
  /// Destructors
  ~Effect() {
    delete R;
  }

  /// Printing
  inline bool printEffectKind(raw_ostream& OS) const {
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

  void print(raw_ostream& OS) const {
    bool HasRpl = printEffectKind(OS);
    if (HasRpl) {
      OS << " on ";
      assert(R && "NULL RPL in non-pure effect");
      R->printElements(OS);
    }
  }

  static void printEffectSummary(Effect::EffectVector& EV, raw_ostream& OS) {
    for (Effect::EffectVector::const_iterator
            I = EV.begin(),
            E = EV.end();
            I != E; I++) {
      (*I)->print(OS);
      OS << "\n";
    }
  }

  std::string toString() const {
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

  inline const Rpl* getRpl() { return R; }

  inline const Attr* getAttr() { return A; }

  inline SourceLocation getLocation() { return A->getLocation();}

  /// Substitution (Effect)
  inline bool substitute(const RplElement &FromElm, const Rpl &ToRpl) {
    if (R)
      return R->substitute(FromElm, ToRpl);
    else
      return true;
  }

  /// SubEffect: true if this <= e
  /**
   *  rpl1 c= rpl2   E1 c= E2
   * ~~~~~~~~~~~~~~~~~~~~~~~~~
   *    E1(rpl1) <= E2(rpl2)
   */
  bool isSubEffectOf(const Effect& E) const {
    bool Result = (isNoEffect() ||
            (isSubEffectKindOf(E) && R->isIncludedIn(*(E.R))));
    osv2  << "DEBUG:: ~~~isSubEffect(" << this->toString() << ", "
        << E.toString() << ")=" << (Result ? "true" : "false") << "\n";
    return Result;
  }
  /// isCoveredBy
  bool isCoveredBy(EffectVector EffectSummary) {
    for (EffectVector::const_iterator
            I = EffectSummary.begin(),
            E = EffectSummary.end();
            I != E; I++) {
      if (isSubEffectOf(*(*I))) return true;
    }
    return false;
  }

}; // end class Effect
