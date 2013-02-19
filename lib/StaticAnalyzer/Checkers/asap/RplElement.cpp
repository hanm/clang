  /// Return true when the input string is a special RPL element
  /// (e.g., '*', '?', 'Root'.
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
  virtual bool operator == (const RplElement& That) const {
    return (this == &That) ? true : false;
  }
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

  static bool classof(const RplElement *R) {
    return R->getKind() == RK_Special;
  }
};


static const SpecialRplElement *ROOTRplElmt = new SpecialRplElement("Root");
static const SpecialRplElement *LOCALRplElmt = new SpecialRplElement("Local");


///-////////////////////////////////////////
class StarRplElement : public RplElement {

public:
  /// Constructor/Destructor
  StarRplElement () : RplElement(RK_Star) {}
  virtual ~StarRplElement() {}

  /// Methods
  virtual bool isFullySpecified() const { return false; }
  virtual StringRef getName() const { return "*"; }

  static bool classof(const RplElement *R) {
    return R->getKind() == RK_Star;
  }
}; // end class StarRplElement

static const StarRplElement *STARRplElmt = new StarRplElement();

/// \brief returns a special RPL element (Root, Local, *, ...) or NULL
const RplElement* getSpecialRplElement(const StringRef& s) {
  if (!s.compare(STARRplElmt->getName()))
    return STARRplElmt;
  else if (!s.compare(ROOTRplElmt->getName()))
    return ROOTRplElmt;
  else if (!s.compare(LOCALRplElmt->getName()))
    return LOCALRplElmt;
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
                          RPL_ELEMENT_VECTOR_SIZE> RplElementVectorTy;

class ParameterVector {
private:
  // Fields
#ifndef PARAM_VECTOR_SIZE
  #define PARAM_VECTOR_SIZE 8
#endif
  typedef llvm::SmallVector<const ParamRplElement*, PARAM_VECTOR_SIZE> ParamVecT;
  ParamVecT ParamVec;
public:
  // Constructor
  ParameterVector() {}

  ParameterVector(const ParamRplElement *ParamEl) {
    ParamVec.push_back(ParamEl);
  }

  // Destructor
  ~ParameterVector() {
    for(ParamVecT::iterator
          I = ParamVec.begin(),
          E = ParamVec.end();
        I != E; ++I) {
      delete (*I);
    }
  }

  // Types
  typedef ParamVecT::const_iterator const_iterator;
  // Methods
  /// \brief Appends a ParamRplElement to the tail of the vector.
  inline void push_back(ParamRplElement *Elm) { ParamVec.push_back(Elm); }
  /// \brief Returns the size of the vector.
  inline size_t size() const { return ParamVec.size(); }
  /// \brief Returns a const_iterator to the begining of the vector.
  inline const_iterator begin() const { return ParamVec.begin(); }
  /// \brief Returns a const_iterator to the end of the vector.
  inline const_iterator end() const { return ParamVec.end(); }

  /// \brief Returns the Parameter at position Idx
  const ParamRplElement *getParamAt(size_t Idx) const {
    assert(Idx < ParamVec.size());
    return ParamVec[Idx];
  }
  /// \brief Returns the ParamRplElement with name=Name or null.
  const ParamRplElement *lookup(StringRef Name) const {
    for(ParamVecT::const_iterator
          I = ParamVec.begin(),
          E = ParamVec.end();
        I != E; ++I) {
      const ParamRplElement *El = *I;
      if (El->getName().compare(Name) == 0)
        return El;
    }
    return 0;
  }
}; // end class ParameterVector

class RegionNameSet {
private:
  // Fields
#ifndef REGION_NAME_SET_SIZE
  #define REGION_NAME_SET_SIZE 8
#endif
  typedef llvm::SmallPtrSet<const NamedRplElement*, REGION_NAME_SET_SIZE> RegnNameSetTy;
  RegnNameSetTy RegnNameSet;
public:
  // Destructor
  ~RegionNameSet() {
    for (RegnNameSetTy::iterator
           I = RegnNameSet.begin(),
           E = RegnNameSet.end();
         I != E; ++I) {
      delete (*I);
    }
  }
  // Methods
  /// \brief Inserts an element to the set and returns true upon success.
  inline bool insert(NamedRplElement *E) { return RegnNameSet.insert(E); }
  /// \brief Returns the number of elements in the set.
  inline size_t size() const { return RegnNameSet.size(); }

  /// \brief Returns the NamedRplElement with name=Name or null.
  const NamedRplElement *lookup (StringRef Name) {
    for (RegnNameSetTy::iterator
           I = RegnNameSet.begin(),
           E = RegnNameSet.end();
         I != E; ++I) {
      const NamedRplElement *El = *I;
      if (El->getName().compare(Name) == 0)
        return El;
    }
    return 0;
  }
}; // end class RegionNameSet

