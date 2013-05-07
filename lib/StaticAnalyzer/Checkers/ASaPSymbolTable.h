//=== ASaPSymbolTable.h - Safe Parallelism checker -----*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This files defines the SymbolTable and SymbolTableEntry classes used by
// the Safe Parallelism checker, which tries to prove the safety of
// parallelism given region and effect annotations.
//
//===----------------------------------------------------------------===//

/// Maps AST Decl* nodes to ASaP info that appertains to the node
/// such information includes ASaPType*, ParamVector, RegionNameSet,
/// and EffectSummary

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_SYMBOL_TABLE_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_SYMBOL_TABLE_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringRef.h"
#include "clang/AST/Type.h"
#include "ASaPFwdDecl.h"

namespace clang {
namespace asap {

enum ResultKind {
  RK_OK,
  RK_ERROR,
  RK_NOT_VISITED,
  RK_VAR
};

StringRef stringOf(ResultKind R);

class SymbolTable {
  class SymbolTableEntry;
  typedef llvm::DenseMap<const Decl*, SymbolTableEntry*>  SymbolTableMapT;
  // Symbol Table Map
  SymbolTableMapT SymTable;

  static int Initialized;

  /// \brief The implicit region parameter "P" of implicitly boxed types
  /// such as int or pointers.
  const ParameterVector *BuiltinDefaultRegionParameterVec;

public:
  SymbolTable();
  virtual ~SymbolTable();

  // Static Constants
  static const StarRplElement *STAR_RplElmt;
  static const SpecialRplElement *ROOT_RplElmt;
  static const SpecialRplElement *LOCAL_RplElmt;
  static const SpecialRplElement *GLOBAL_RplElmt;
  static const SpecialRplElement *IMMUTABLE_RplElmt;
  static const Effect *WritesLocal;

  // Static Functions
  static void Initialize();
  static void Destroy();
  static inline bool isNonPointerScalarType(QualType QT) {
    return (QT->isScalarType() && !QT->isPointerType());
  }
  /// \brief a special RPL element (Root, Local, *, ...) or NULL
  static const RplElement *getSpecialRplElement(const llvm::StringRef& S);
  /// \brief Return true when the input string is a special RPL element
  /// (e.g., '*', '?', 'Root', 'Local', 'Globa', ...)
  static bool isSpecialRplElement(const llvm::StringRef& S);

  // Types
  class ResultTriplet {
  public:
    ResultKind ResKin;
    long NumArgs;
    RecordDecl *DeclNotVis;
    ResultTriplet(ResultKind ReKi, long NumA, RecordDecl *D) :
      ResKin(ReKi), NumArgs(NumA), DeclNotVis(D) {}
  };

  // Functions
  /// \brief return the number of In/Arg annotations needed for type or -1
  /// if unknown.
  ResultTriplet getRegionParamCount(QualType QT);

  bool hasDecl(const Decl *D) const;
  bool hasType(const Decl *D) const;
  bool hasParameterVector(const Decl *D) const;
  bool hasRegionNameSet(const Decl *D) const;
  bool hasEffectSummary(const Decl *D) const;

  /// \brief Returns the ASaP Type for D or null.
  const ASaPType *getType(const Decl *D) const;
  /// \brief Retuns the parameter vector for D or null.
  const ParameterVector *getParameterVector(const Decl *D) const;

  const SubstitutionVector *getInheritanceSubVec(const Decl *D) const;

  /// \brief Returns the region names declarations for D or null.
  const RegionNameSet *getRegionNameSet(const Decl *D) const;
  /// \brief Returns the effect summmary for D or null.
  const EffectSummary *getEffectSummary(const Decl *D) const;
  /// \brief Returns true iff the type for D was not already set.
  bool setType(const Decl* D, ASaPType *T);

  bool initParameterVector(const Decl *D);
  /// \brief Returns true iff the parameter vector for D was not already set.
  bool setParameterVector(const Decl *D, ParameterVector *PV);
  /// \brief Returns true iff the region names for D were not already set.
  bool setRegionNameSet(const Decl *D, RegionNameSet *RNS);
  /// \brief Returns true iff the effect summary for D was not already set.
  bool setEffectSummary(const Decl *D, EffectSummary *ES);
  /// \brief Sets the effect summary of D to that of Dfrom if it was set
  bool setEffectSummary(const Decl *D, const Decl *Dfrom);
  // Lookup
  /// \brief Returns a named RPL element of the same name or null.
  const NamedRplElement *lookupRegionName(const Decl *D,
                                          llvm::StringRef Name) const;
  /// \brief Returns a parameter RPL element of the same name or null.
  const ParamRplElement *lookupParameterName(const Decl *D,
                                             llvm::StringRef Name) const;
  // Others
  /// \brief Returns true iff D has a declared region-name Name.
  bool hasRegionName(const Decl *D, llvm::StringRef Name) const;
  /// \brief Returns true iff D has a declared region parameter Name.
  bool hasParameterName(const Decl *D, llvm::StringRef Name) const;
  /// \brief Returns true iff D has a declared region-name or parameter Name.
  bool hasRegionOrParameterName(const Decl *D, llvm::StringRef Name) const;

  bool hasBase(const Decl *D, const Decl *Base) const;
  /// \brief Adds a region name to declaration D. Return false if name exists.
  bool addRegionName(const Decl *D, llvm::StringRef Name);
  /// \brief Adds a region parameter to D. Return false if name exists.
  bool addParameterName(const Decl *D, llvm::StringRef Name);

  bool addBaseTypeAndSub(const Decl *D, const Decl *Base,
                         SubstitutionVector *&SubV);



  /// \brief Get parameter vector from Clang::Type
  const ParameterVector *getParameterVectorFromQualType(QualType QT);

  const SubstitutionVector *getInheritanceSubVec(QualType QT);

private:
  class SymbolTableEntry {
    friend class SymbolTable;
    // Fields
    ASaPType *Typ;
    ParameterVector *ParamVec;
    RegionNameSet *RegnNameSet;
    EffectSummary *EffSum;

    // Instead of Map(Decl -> SubVec) we use the SymbolTableEntry that
    // corresponds to the Decl so we won't have to store a pointer to
    // the SymbolTable from the SymbolTableEntries, in order to resolve
    // the Decls
    typedef llvm::DenseMap<SymbolTableEntry *, const SubstitutionVector *>
        InheritanceMapT;
    InheritanceMapT *InheritanceMap;
    bool ComputedInheritanceSubVec;
    SubstitutionVector *InheritanceSubVec;

    // Private Methods
    void computeInheritanceSubVec();

  public:
    SymbolTableEntry();
    ~SymbolTableEntry();

    // Predicates.
    inline bool hasType() const { return (Typ) ? true : false; }
    inline bool hasParameterVector() const { return (ParamVec) ? true : false; }
    inline bool hasRegionNameSet() const { return (RegnNameSet) ? true :false; }
    inline bool hasEffectSummary() const { return (EffSum) ? true : false; }
    inline bool hasInheritanceMap() const { return (InheritanceMap) ? true : false; }

    // Getters.
    inline const ASaPType *getType() const { return Typ; }
    inline const ParameterVector *getParameterVector() const { return ParamVec; }
    inline const RegionNameSet *getRegionNameSet() const { return RegnNameSet; }
    inline const EffectSummary *getEffectSummary() const { return EffSum; }
    inline const InheritanceMapT *getInheritanceMap() const { return InheritanceMap; }

    // Setters.
    inline void setType(ASaPType *T) { Typ = T; }
    inline void setParameterVector(ParameterVector *PV) { ParamVec = PV; }
    inline void setRegionNameSet(RegionNameSet *RNS) { RegnNameSet = RNS; }
    inline void setEffectSummary(EffectSummary *ES) { EffSum = ES; }
    inline void setInheritanceMap(InheritanceMapT *Map) { InheritanceMap = Map; }

    // Lookup.
    const NamedRplElement *lookupRegionName(llvm::StringRef Name);
    const ParamRplElement *lookupParameterName(llvm::StringRef Name);

    // Adders.
    void addRegionName(llvm::StringRef Name);
    void addParameterName(llvm::StringRef Name);
    bool addBaseTypeAndSub(SymbolTableEntry *Base,
                           SubstitutionVector *&SubV);

    const SubstitutionVector *getSubVec(SymbolTableEntry *Base) const;

    const SubstitutionVector *getInheritanceSubVec();


  protected:
    inline EffectSummary *getNonConstEffectSummary() const { return EffSum; }

  }; // End class SymbolTableEntry.
}; // End class SymbolTable.
} // End namespace asap.
} // End namespace clang.


#endif


