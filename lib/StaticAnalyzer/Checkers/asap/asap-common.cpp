  
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


static const SpecialRplElement* ROOT_RplElmt = new SpecialRplElement("Root");
static const SpecialRplElement* LOCAL_RplElmt = new SpecialRplElement("Local");


///-////////////////////////////////////////
class StarRplElement : public RplElement { 

public:
  /// Constructor/Destructor
  StarRplElement () : RplElement(RK_Star) {}
  virtual ~StarRplElement() {}
  
  /// Methods
  virtual bool isFullySpecified() const { return false; }
  virtual StringRef getName() const { return "*"; }
  virtual bool operator == (const RplElement& that) const {
    return (isa<const StarRplElement>(that) 
            && this==&that) ? true : false;
  }
  static bool classof(const RplElement *R) {
    return R->getKind() == RK_Star;
  }
};

static const StarRplElement* STAR_RplElmt = new StarRplElement();

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
  virtual bool operator == (const RplElement& that) const {
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

void destroyRplElementVector(RplElementVector& rev) {
  for (RplElementVector::const_iterator
           it = rev.begin(),
           end = rev.end();
       it != end; it++) {
    delete(*it);
  }
}

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
  StringRef rpl_str;
  RplElementVector rplElements;
  bool fullySpecified;

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
      lastIdx = rpl.rplElements.size()-1;
    }
    /// Printing
    void printElements(raw_ostream& os) {
      int i = firstIdx;
      for (; i<lastIdx; i++) {
        os << rpl.rplElements[i]->getName() << ":";
      } 
      // print last element
      if (i==lastIdx)
        os << rpl.rplElements[i]->getName();
    }
  
    std::string toString() {
      std::string sbuf;
      llvm::raw_string_ostream os(sbuf);
      printElements(os);
      return std::string(os.str());
    }

    /// Getters
    const RplElement* getFirstElement() {
      return rpl.rplElements[firstIdx];
    }
    const RplElement* getLastElement() {
      return rpl.rplElements[lastIdx];
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
    fullySpecified = true;
    while(rpl.size() > 0) { /// for all RPL elements of the RPL
      // FIXME: '::' can appear as part of an RPL element. Splitting must
      // be done differently to account for that.
      std::pair<StringRef,StringRef> pair = rpl.split(RPL_SPLIT_CHARACTER);
      
      rplElements.push_back(new NamedRplElement(pair.first));
      if (isSpecialRplElement(pair.first))
        fullySpecified = false;
      rpl = pair.second;
    }
  }*/
  
  Rpl() : fullySpecified(true) {}
  
  Rpl(RplElement *elm) {
    fullySpecified = elm->isFullySpecified();
    rpl_str = elm->getName();
    rplElements.push_back(elm);
  }
  
  /// Copy Constructor -- deep clone
  Rpl(Rpl* that) :
      rpl_str(that->rpl_str),
      fullySpecified(that->fullySpecified)  
  {
    rplElements = that->rplElements;
    /*for(RplElementVector::const_iterator
            it = that->rplElements.begin(),
            end = that->rplElements.end();
         it != end; it++) {
      rplElements.append((*it));
    }*/
  }
  /// Destructors
  static void destroyRplVector(RplVector& ev) {
    for (RplVector::const_iterator
            it = ev.begin(),
            end = ev.end();
          it != end; it++) {
      delete (*it);
    }
  }

  /*
  virtual ~Rpl() {
    for (RplElementVector::const_iterator
             it = rplElements.begin(),
             end = rplElements.end();
          it != end; it++) {
      delete(*it);
    }
  }*/
  
  /// Printing
  void printElements(raw_ostream& os) const {
    RplElementVector::const_iterator I = rplElements.begin();
    RplElementVector::const_iterator E = rplElements.end();
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
    return rplElements.back(); 
  }
  
  inline size_t length() {
    return rplElements.size();
  }
  /// Setters
  inline void appendElement(const RplElement* rplElm) {
    rplElements.push_back(rplElm);
    if (rplElm->isFullySpecified() == false) 
      fullySpecified = false;
  }
  /// Predicates
  inline bool isFullySpecified() { return fullySpecified; }
  inline bool isEmpty() { return rplElements.empty(); }
  
  /// Nesting (Under)
  bool isUnder(const Rpl& rhsRpl) const {
    const CaptureRplElement *cap = dyn_cast<CaptureRplElement>(rplElements.front());
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
    const CaptureRplElement *cap = dyn_cast<CaptureRplElement>(rplElements.front());
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
  
  /// Substitution
  bool substitute(RplElement& from, Rpl& to) {
    os << "DEBUG:: before substitution(" << from.getName() << "<-";
    to.printElements(os);
    os <<"): ";
    printElements(os);
    os << "\n";
    /// 1. find all occurences of 'from'
    for (RplElementVector::iterator
            it = rplElements.begin();
         it != rplElements.end(); it++) {
      if (*(*it) == from) { 
        osv2 << "DEBUG:: found '" << from.getName() 
          << "' replaced with '" ;
        to.printElements(osv2);
        it = rplElements.erase(it);
        it = rplElements.insert(it, to.rplElements.begin(), to.rplElements.end());
        osv2 << "' == '";
        printElements(osv2);
        osv2 << "'\n";
      }
    }
    os << "DEBUG:: after substitution(" << from.getName() << "<-";
    to.printElements(os);
    os << "): ";
    printElements(os);
    os << "\n";
    return false;
  }  
  
  Rpl* upperBound() { 
    if (isEmpty() || isa<CaptureRplElement>(rplElements.front()))
      return this;
    /// TODO 
    // else
    //Rpl* upperBound = & dyn_cast<CaptureRplElement>(rplElements.front())->upperBound();
    //upperBound->appendRpl()
    return 0;
  }
  /// Capture
  inline static Rpl* capture(Rpl* r) {
    return new Rpl(new CaptureRplElement(*r));
  }
  /// Iterator
}; // end class Rpl

///-///////////////////////////////////////////////////////////////////////////
/// Effect Class

enum EffectKind {
  /// pure = no effect
  NoEffect,
  /// reads effect
  ReadsEffect,
  /// atomic reads effect
  AtomicReadsEffect,
  /// writes effect
  WritesEffect,
  /// atomic writes effect
  AtomicWritesEffect
};


class Effect {
private:
  /// Fields
  EffectKind effectKind;
  Rpl* rpl;
  const Attr* attr; // used to get SourceLocation information

  /// Sub-Effect Kind
  inline bool isSubEffectKindOf(const Effect& e) const {
    bool result = false;
    if (effectKind == NoEffect) return true; // optimization
    
    if (!e.isAtomic() || this->isAtomic()) { 
      /// if e.isAtomic ==> this->isAtomic() [[else return false]]
      switch(e.getEffectKind()) {
      case WritesEffect:
        if (effectKind == WritesEffect) result = true;
        // intentional fall through (lack of 'break')
      case AtomicWritesEffect:
        if (effectKind == AtomicWritesEffect) result = true;
        // intentional fall through (lack of 'break')
      case ReadsEffect:
        if (effectKind == ReadsEffect) result = true;
        // intentional fall through (lack of 'break')
      case AtomicReadsEffect:
        if (effectKind == AtomicReadsEffect) result = true;
        // intentional fall through (lack of 'break')
      case NoEffect:
        if (effectKind == NoEffect) result = true;
      }
    }
    return result;
  }

public:
  /// Types
#ifndef EFFECT_VECTOR_SIZE
  #define EFFECT_VECTOR_SIZE 16
#endif
  typedef llvm::SmallVector<Effect*, EFFECT_VECTOR_SIZE> EffectVector;

  /// Constructors
  Effect(EffectKind ec, Rpl* r, const Attr* a) 
        : effectKind(ec), rpl(r), attr(a) {}

  /// Destructors
  virtual ~Effect() { 
    delete rpl;
  }

  static void destroyEffectVector(Effect::EffectVector& ev) {
    for (Effect::EffectVector::const_iterator
            it = ev.begin(),
            end = ev.end();
          it != end; it++) {
      delete (*it);
    }
  }

  /// Printing
  inline bool printEffectKind(raw_ostream& os) const {
    bool hasRpl = true;
    switch(effectKind) {
    case NoEffect: os << "Pure Effect"; hasRpl = false; break;
    case ReadsEffect: os << "Reads Effect"; break;
    case WritesEffect: os << "Writes Effect"; break;
    case AtomicReadsEffect: os << "Atomic Reads Effect"; break;
    case AtomicWritesEffect: os << "Atomic Writes Effect"; break;
    }
    return hasRpl;
  }

  void print(raw_ostream& os) const {
    bool hasRpl = printEffectKind(os);
    if (hasRpl) {
      os << " on ";
      assert(rpl && "NULL RPL in non-pure effect");
      rpl->printElements(os);
    }
  }

  static void printEffectSummary(Effect::EffectVector& ev, raw_ostream& os) {
    for (Effect::EffectVector::const_iterator 
            I = ev.begin(),
            E = ev.end(); 
            I != E; I++) {
      (*I)->print(os);
      os << "\n";
    }    
  }

  std::string toString() const {
    std::string sbuf;
    llvm::raw_string_ostream os(sbuf);
    print(os);
    return std::string(os.str());
  }
  
  /// Predicates
  inline bool isNoEffect() const {
    return (effectKind == NoEffect) ? true : false;
  }
  
  inline bool hasRplArgument() const { return !isNoEffect(); }

  inline bool isAtomic() const { 
    return (effectKind==AtomicReadsEffect ||
            effectKind==AtomicWritesEffect) ? true : false;
  }

  /// Getters
  inline EffectKind getEffectKind() const { return effectKind; }
  
  inline const Rpl* getRpl() { return rpl; }

  inline const Attr* getAttr() { return attr; }
  inline SourceLocation getLocation() { return attr->getLocation();}
  
  /// Substitution
  inline bool substitute(RplElement& from, Rpl& to) {
    if (rpl)
      return rpl->substitute(from, to);
    else 
      return true;
  }  
  
  /// SubEffect: true if this <= e
  /** 
   *  rpl1 c= rpl2   E1 c= E2
   * ~~~~~~~~~~~~~~~~~~~~~~~~~
   *    E1(rpl1) <= E2(rpl2) 
   */
  bool isSubEffectOf(const Effect& e) const {
    bool result = (isNoEffect() ||
            (isSubEffectKindOf(e) && rpl->isIncludedIn(*(e.rpl))));
    osv2  << "DEBUG:: ~~~isSubEffect(" << this->toString() << ", "
        << e.toString() << ")=" << (result ? "true" : "false") << "\n";
    return result;
  }
  /// isCoveredBy
  bool isCoveredBy(EffectVector effectSummary) {
    for (EffectVector::const_iterator
            I = effectSummary.begin(),
            E = effectSummary.end();
            I != E; I++) {
      if (isSubEffectOf(*(*I))) return true;
    }
    return false;
  }

}; // end class Effect
