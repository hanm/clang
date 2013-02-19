/// Maps AST Decl* nodes to ASaP info that appertains to the node
/// such information includes ASaPType*, ParamVector, RegionNameSet,
/// and EffectSummary

namespace ASaP {
class SymbolTable {
private:
  // Forward Declaration
  class SymbolTableEntry;
  // Type
  typedef llvm::DenseMap<const Decl*, SymbolTableEntry*>  SymbolTableMapT;
  // Fields
  SymbolTableMapT SymTable;
  /// \brief The implicit region parameter "P" of implicitly boxed types
  /// such as int or pointers.
  const ParameterVector *BuiltinDefaultRegionParameterVec;

public:
  // Constructor
  SymbolTable() {
    BuiltinDefaultRegionParameterVec =
      new ParameterVector(new ParamRplElement("P"));
  }
  // Destructor
  virtual ~SymbolTable() {
    delete BuiltinDefaultRegionParameterVec;

    for(SymbolTableMapT::iterator I = SymTable.begin(), E = SymTable.end();
        I != E; ++I) {
      // for this key, delete the value
      delete (*I).second;
    }
  }
  // Predicates
  inline bool hasType(const Decl* D) const {
    if (!SymTable.lookup(D))
      return false;
    return SymTable.lookup(D)->hasType();
  }

  inline bool hasParameterVector(const Decl* D) const {
    if (!SymTable.lookup(D))
      return false;
    return SymTable.lookup(D)->hasParameterVector();
  }

  inline bool hasRegionNameSet(const Decl* D) const {
    if (!SymTable.lookup(D))
      return false;
    return SymTable.lookup(D)->hasParameterVector();
  }

  inline bool hasEffectSummary(const Decl* D) const {
    if (!SymTable.lookup(D))
      return false;
    return SymTable.lookup(D)->hasEffectSummary();
  }
  // Getters
  const ASaPType *getType(const Decl* D) const {
    if (!SymTable.lookup(D))
      return 0;
    else
      return SymTable.lookup(D)->getType();
  }

  const ParameterVector *getParameterVector(const Decl *D) const {
    if (!SymTable.lookup(D))
      return 0;
    else
      return SymTable.lookup(D)->getParameterVector();
  }

  const RegionNameSet *getRegionNameSet(const Decl *D) const {
    if (!SymTable.lookup(D))
      return 0;
    else
      return SymTable.lookup(D)->getRegionNameSet();
  }

  const EffectSummary *getEffectSummary(const Decl *D) const {
    if (!SymTable.lookup(D))
      return 0;
    else
      return SymTable.lookup(D)->getEffectSummary();
  }
  // Setters
  /// \brief returns true iff the type for D was not already set
  bool setType(const Decl* D, ASaPType *T) {
    if (!SymTable[D])
      SymTable[D] = new SymbolTableEntry();
    // invariant: SymTable[D] not null
    if (SymTable[D]->hasType())
      return false;
    else {
      SymTable[D]->setType(T);
      return true;
    }
  }
  /// \brief returns true iff the parameter vectr for D was not already set
  bool setParameterVector(const Decl *D, ParameterVector *PV) {
    if (!SymTable[D])
      SymTable[D] = new SymbolTableEntry();
    // invariant: SymTable[D] not null
    if (SymTable[D]->hasParameterVector())
      return false;
    else {
      SymTable[D]->setParameterVector(PV);
      return true;
    }
  }
  /// \brief returns true iff the parameter vectr for D was not already set
  bool setRegionNameSet(const Decl *D, RegionNameSet *RNS) {
    if (!SymTable[D])
      SymTable[D] = new SymbolTableEntry();
    // invariant: SymTable[D] not null
    if (SymTable[D]->hasRegionNameSet())
      return false;
    else {
      SymTable[D]->setRegionNameSet(RNS);
      return true;
    }
  }
  /// \brief returns true iff the parameter vectr for D was not already set
  bool setEffectSummary(const Decl *D, EffectSummary *ES) {
    if (!SymTable[D])
      SymTable[D] = new SymbolTableEntry();
    // invariant: SymTable[D] not null
    if (SymTable[D]->hasEffectSummary())
      return false;
    else {
      SymTable[D]->setEffectSummary(ES);
      return true;
    }
  }
  // Lookup
  const NamedRplElement *lookupRegionName(const Decl* D, StringRef Name) {
    if (!SymTable.lookup(D))
      return 0;
    return SymTable.lookup(D)->lookupRegionName(Name);
  }

  const ParamRplElement *lookupParameterName(const Decl *D, StringRef Name) {
    if (!SymTable.lookup(D))
      return 0;
    return SymTable.lookup(D)->lookupParameterName(Name);
  }
  // Others
  inline bool hasRegionName(const Decl *D, StringRef Name) {
    return lookupRegionName(D, Name) ? true : false;
  }
  inline bool hasParameterName(const Decl *D, StringRef Name) {
    return lookupParameterName(D, Name) ? true : false;
  }

  inline bool hasRegionOrParameterName(const Decl *D, StringRef Name) {
    return hasRegionName(D, Name) || hasParameterName(D, Name);
  }

  /// \brief Add a region name to declaration D. Return false if name exists.
  bool addRegionName(const Decl *D, StringRef Name) {
    if (hasRegionOrParameterName(D, Name))
      return false;
    // else
    if (!SymTable.lookup(D))
      SymTable[D] = new SymbolTableEntry();
    SymTable[D]->addRegionName(Name);
    return true;
  }
  /// \brief
  bool addParameterName(const Decl *D, StringRef Name) {
    if (hasRegionOrParameterName(D, Name))
      return false;
    // else
    if (!SymTable.lookup(D))
      SymTable[D] = new SymbolTableEntry();
    SymTable[D]->addParameterName(Name);
    return true;
  }

  /// \brief Get parameter vector from Clang::Type
  const ParameterVector *getParameterVectorFromQualType(QualType QT) {
    const ParameterVector *ParamVec = 0;
    if (const TagType* TT = dyn_cast<TagType>(QT.getTypePtr())) {
      const TagDecl* TD = TT->getDecl();
      ParamVec = getParameterVector(TD);
    } else if (QT->isBuiltinType() || QT->isPointerType()) {
      // TODO check the number of parameters of the arg attr to be 1
      ParamVec = BuiltinDefaultRegionParameterVec;
    } /// else result = NULL;

    return ParamVec;
  }


private:
  class SymbolTableEntry {
    friend class ASaP::SymbolTable;
  private:
    // Fields
    ASaPType *Typ;
    ParameterVector *ParamVec;
    RegionNameSet *RegnNameSet;
    EffectSummary *EffSum;
  public:
    // Constructor
    SymbolTableEntry() : Typ(0), ParamVec(0), RegnNameSet(0), EffSum(0) {}

    // Predicates
    bool hasType() const { return (Typ) ? true : false; }
    bool hasParameterVector() const { return (ParamVec) ? true : false; }
    bool hasRegionNameSet() const { return (RegnNameSet) ? true :false; }
    bool hasEffectSummary() const { return (EffSum) ? true : false; }

    // Getters
    const ASaPType *getType() const { return Typ; };
    const ParameterVector *getParameterVector() const { return ParamVec; }
    const RegionNameSet *getRegionNameSet() const { return RegnNameSet; }
    const EffectSummary *getEffectSummary() const { return EffSum; }

    // Setters
    inline void setType(ASaPType *T) { Typ = T; }
    inline void setParameterVector(ParameterVector *PV) { ParamVec = PV; }
    inline void setRegionNameSet(RegionNameSet *RNS) { RegnNameSet = RNS; }
    inline void setEffectSummary(EffectSummary *ES) { EffSum = ES; }

    // Lookup
    const NamedRplElement *lookupRegionName(StringRef Name) {
      if (!RegnNameSet)
        return 0;
      return RegnNameSet->lookup(Name);
    }

    const ParamRplElement *lookupParameterName(StringRef Name) {
      if (!ParamVec)
        return 0;
      return ParamVec->lookup(Name);
    }

    // Adders
    void addRegionName(StringRef Name) {
      if (!RegnNameSet)
        RegnNameSet = new RegionNameSet();
      RegnNameSet->insert(new NamedRplElement(Name));
    }

    void addParameterName(StringRef Name) {
      if (!ParamVec)
        ParamVec = new ParameterVector();
      ParamVec->push_back(new ParamRplElement(Name));
    }

  }; // end class SymbolTableEntry
}; // end class SymbolTable
} // end namespace ASaP
