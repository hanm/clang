/// Maps AST Decl* nodes to ASaP info that appertains to the node
//  such information includes ASaPType*, ParamVector, RegionNameSet,
//  and EffectSummary

namespace ASaP {
class SymbolTable {
private:
  // Fields
  class SymbolTableEntry;
  typedef llvm::DenseMap<const Decl*, SymbolTableEntry*>  SymbolTableMapT;
  SymbolTableMapT SymTable;
public:
  // Constructor
  // Destructor
  virtual ~SymbolTable() {
    // TODO destroy map
    for(SymbolTableMapT::iterator I = SymTable.begin(), E = SymTable.end();
        I != E; ++I) {
      // for this key, delete the value
      delete (*I).second;
    }
  }
  // Getters
  const ASaPType *getType(const Decl* D) const {
    if (!SymTable.lookup(D))
      return 0;
    else
      return SymTable.lookup(D)->getType();
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
    // if !map[D] add SymbolTableEntry
    // if map[D].hasType() return false
    // else map[D].setType(T)
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
  }; // end class SymbolTableEntry
}; // end class SymbolTable
} // end namespace ASaP
