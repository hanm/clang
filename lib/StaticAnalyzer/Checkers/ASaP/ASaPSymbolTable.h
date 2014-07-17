//=== ASaPSymbolTable.h - Safe Parallelism checker -----*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This file defines the SymbolTable and SymbolTableEntry classes used by
// the Safe Parallelism checker, which tries to prove the safety of
// parallelism given region and effect annotations.
//
//===----------------------------------------------------------------===//

/// Maps AST Decl* nodes to ASaP info that appertains to the node
/// such information includes ASaPType*, ParamVector, RegionNameSet,
/// and EffectSummary

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_SYMBOL_TABLE_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_SYMBOL_TABLE_H

#include <sstream>

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringRef.h"

#include "clang/AST/Type.h"

#include "ASaPAnnotationScheme.h"
#include "ASaPUtil.h"
#include "ASaPFwdDecl.h"
#include "ASaPInheritanceMap.h"
#include "Constraints.h"
#include "OwningPtrSet.h"
#include "Rpl.h"

namespace clang {
namespace asap {

enum ResultKind {
  RK_OK,          // We know the result
  RK_ERROR,       // An error occured
  RK_NOT_VISITED, // We don't know the result because decl not yet visited
  RK_VAR          // We don't know the result because it's a template type var
};

StringRef stringOf(ResultKind R);

class ResultTriplet {
public:
  // Fields
  ResultKind ResKin;
  long NumArgs;
  RecordDecl *DeclNotVis;
  // Constructor
  ResultTriplet(ResultKind ReKi, long NumA, RecordDecl *D) :
    ResKin(ReKi), NumArgs(NumA), DeclNotVis(D) {}
};
#ifndef NUM_OF_CONSTRAINTS
  #define NUM_OF_CONSTRAINTS 100
#endif
class SymbolTable {
  typedef llvm::DenseMap<const Decl*, SymbolTableEntry*>  SymbolTableMapT;
  typedef llvm::DenseMap<const FunctionDecl*,
                          const SpecificNIChecker*> ParallelismMapT;
  typedef OwningPtrSet<std::string, 1024> FreshNamesSetT;
  //typedef OwningPtrSet<clang::asap::Constraint, NUM_OF_CONSTRAINTS> ConstraintsSetT;
  typedef llvm::SmallPtrSet<Constraint*, NUM_OF_CONSTRAINTS> ConstraintsSetT;

  //typedef OwningPtrSet<clang::asap::EffectNIConstraint, NUM_OF_CONSTRAINTS> NIConstraintsSetT;

  /// \brief Symbol Table Map
  SymbolTableMapT SymTable;

  /// \brief Parallelism Checker Map: maps specific function declarations
  /// to objects that 'know' which NI constraints need checking.
  ///
  /// These objects have types derived from SpecificNIChecker that 'know'
  /// what NI constraints each function needs (e.g., parallel_for w. range,
  /// parallel_for with iterators, parallel_reduce, parallel_invoke, etc).
  ParallelismMapT ParTable;

  /// \brief A set keeping track of fresh names created (strings)
  /// so that they are properly deallocated by the symbol table destructor
  FreshNamesSetT FreshNames;

  /// \brief Set of all effect inclusion constraints generated
  ConstraintsSetT ConstraintSet;

  /// \brief Set of all non-interference constraints generated
  //NIConstraintsSetT NIConstraints;

  AnnotationScheme *AnnotScheme;
  static int Initialized;

  // Private Constructors & Destructors
  SymbolTable();
  virtual ~SymbolTable();

  /// \brief The implicit region parameter "P" of implicitly boxed types
  /// such as int or pointers.
  const ParameterVector *BuiltinDefaultRegionParameterVec;

  unsigned long ParamIDNumber;
  unsigned long RegionIDNumber;
  unsigned long DeclIDNumber;
  unsigned long RVIDNumber;
  unsigned long ConstraintIDNumber;

  // Private Methods
  /// \brief Return the next unused parameter ID number.
  /// Used to encode names into Prolog.
  inline unsigned long getNextUniqueParamID() { return ParamIDNumber++; }

  /// \brief Return the next unused region ID number.
  /// Used to encode names into Prolog.
  inline unsigned long getNextUniqueRegionID() { return RegionIDNumber++; }

  /// \brief Return the next unused declaration ID number.
  /// Used to encode names into Prolog.
  inline unsigned long getNextUniqueDeclID() { return DeclIDNumber++; }

  /// \brief Return the next unused VarRpl ID number.
  /// Used to encode VarRpl names into Prolog.
  inline unsigned long getNextUniqueRVID() { return RVIDNumber++; }

  /// \brief Return the next unused Constraint ID number.
  /// Used to encode Constraints into Prolog.
  inline unsigned long getNextUniqueConstraintID() {
    return ConstraintIDNumber++;
  }

  inline StringRef addFreshName(StringRef SRef) {
    std::string *S = new std::string(SRef.str());
    StringRef Result(*S);
    FreshNames.insert(S);
    return Result;
  }

  bool addInclusionConstraint(const FunctionDecl *FunD,
                              EffectInclusionConstraint *EIC);

  void assertzHasEffectSummary(const NamedDecl *NDec,
                               const ConcreteEffectSummary *EffSum) const;
public:
  // Static Constants
  static const StarRplElement *STAR_RplElmt;
  static const SpecialRplElement *ROOT_RplElmt;
  static const SpecialRplElement *LOCAL_RplElmt;
  static const SpecialRplElement *GLOBAL_RplElmt;
  static const SpecialRplElement *IMMUTABLE_RplElmt;
  static const ConcreteEffectSummary *PURE_EffSum;
  static const Effect *WritesLocal;

  // Unique globally accessible symbol table
  static SymbolTable *Table;
  static VisitorBundle VB;

  // Static Functions
  static void Initialize(VisitorBundle &VB);
  static void Destroy();
  static inline bool isNonPointerScalarType(QualType QT) {
    return (QT->isScalarType() && !QT->isPointerType());
  }
  /// \brief a special RPL element (Root, Local, *, ...) or NULL
  static const RplElement *getSpecialRplElement(const llvm::StringRef& S);
  /// \brief Return true when the input string is a special RPL element
  /// (e.g., '*', '?', 'Root', 'Local', 'Globa', ...)
  static bool isSpecialRplElement(const llvm::StringRef& S);

  // Functions
  /// \brief set the pointer to the annotation scheme.
  inline void setAnnotationScheme(AnnotationScheme *AnS) {
    AnnotScheme = AnS;
  }

  /// \brief return the number of In/Arg annotations needed for type or -1
  /// if unknown.
  ResultTriplet getRegionParamCount(QualType QT);

  // Predicates
  bool hasDecl(const Decl *D) const;
  bool hasType(const Decl *D) const;
  bool hasParameterVector(const Decl *D) const;
  bool hasRegionNameSet(const Decl *D) const;
  bool hasEffectSummary(const Decl *D) const;
  bool hasInheritanceMap(const Decl *D) const;

  // Getters
  /// \brief Returns the ASaP Type for D or null.
  const ASaPType *getType(const Decl *D) const;
  /// \brief Retuns the parameter vector for D or null.
  const ParameterVector *getParameterVector(const Decl *D) const;

  /// \brief Returns the region names declarations for D or null.
  const RegionNameSet *getRegionNameSet(const Decl *D) const;
  RegionNameVector *getRegionNameVector(const Decl *D) const;
  RplDomain *buildDomain(const Decl *D) ;

  /// \brief Returns the effect summmary for D or null.
  const EffectSummary *getEffectSummary(const Decl *D) const;
  const InheritanceMapT *getInheritanceMap(const ValueDecl *D) const;
  const InheritanceMapT *getInheritanceMap(const CXXRecordDecl *D) const;
  const InheritanceMapT *getInheritanceMap(QualType QT) const;
  const SubstitutionVector *getInheritanceSubVec(const Decl *D) const;
  const StringRef getPrologName(const Decl *D) const;

  inline const SpecificNIChecker *getNIChecker(const FunctionDecl *FD) const {
    return ParTable.lookup(FD);
  }
  // Setters
  /// \brief Returns true iff the type for D was not already set.
  bool setType(const Decl* D, ASaPType *T);

  bool initParameterVector(const Decl *D);
  /// \brief Returns true iff the parameter vector for D was not already set.
  bool setParameterVector(const Decl *D, ParameterVector *PV);
  /// \brief Destructively adds parameters to the parameters of the declaration
  bool addToParameterVector(const Decl *D, ParameterVector *&PV);
  /// \brief Returns true iff the region names for D were not already set.
  bool setRegionNameSet(const Decl *D, RegionNameSet *RNS);
  /// \brief Returns true iff the effect summary for D was not already set.
  bool setEffectSummary(const Decl *D, EffectSummary *ES);
  /// \brief Sets the effect summary of D to that of Dfrom if it was set
  bool setEffectSummary(const Decl *D, const Decl *Dfrom);
  /// \brief Sets the effect summary of D to a copy of ES after deleting
  /// any effect summary that D may have had.
  void resetEffectSummary(const Decl *D, const EffectSummary *ES);

  // Lookup
  /// \brief Returns a region-name RPL element of the requested Name or null.
  const NamedRplElement *lookupRegionName(const Decl *D,
                                          llvm::StringRef Name) const;
  /// \brief Returns a parameter RPL element of the requested Name or null.
  const ParamRplElement *lookupParameterName(const Decl *D,
                                             llvm::StringRef Name) const;
  /// \brief Returns an RPL element (region-name or parameter) of the
  /// requested Name or null.
  const RplElement *lookupRegionOrParameterName(const Decl *D,
                                                StringRef Name) const;
  // Others
  /// \brief Returns true iff D has a declared region-name Name.
  bool hasRegionName(const Decl *D, llvm::StringRef Name) const;
  /// \brief Returns true iff D has a declared region parameter Name.
  bool hasParameterName(const Decl *D, llvm::StringRef Name) const;
  /// \brief Returns true iff D has a declared region-name or parameter Name.
  bool hasRegionOrParameterName(const Decl *D, llvm::StringRef Name) const;

  bool hasBase(const Decl *D, const RecordDecl *Base) const;
  /// \brief Adds a region name to declaration D. Return false if name exists.
  bool addRegionName(const Decl *D, llvm::StringRef Name);
  /// \brief Adds a region parameter to D. Return false if name exists.
  bool addParameterName(const Decl *D, llvm::StringRef Name);

  bool addBaseTypeAndSub(const Decl *D, const RecordDecl *Base,
                         SubstitutionVector *&SubV);

  bool addParallelFun(const FunctionDecl *D, const SpecificNIChecker *NIC);

  /// \brief Get parameter vector from Clang::Type
  const ParameterVector *getParameterVectorFromQualType(QualType QT);

  const SubstitutionVector *getInheritanceSubVec(QualType QT);

  inline StringRef makeFreshParamName(const std::string &Name) {
    std::stringstream ss;
    ss << "p" << getNextUniqueParamID()
       << "_" << Name;
    return addFreshName(ss.str());
  }

  inline StringRef makeFreshRegionName(const std::string &Name) {
    std::stringstream ss;
    ss << "r" << getNextUniqueRegionID()
       << "_" << Name;
    return addFreshName(ss.str());
  }

  inline StringRef makeFreshDeclName(const std::string &Name) {
    std::stringstream ss;
    ss << "d" << getNextUniqueDeclID()
       << "_" << Name;
    return addFreshName(ss.str());
  }

  inline StringRef makeFreshRVName(const std::string &Name) {
    std::stringstream ss;
    ss << "rv" << getNextUniqueRVID()
       << "_" << Name;
    return addFreshName(ss.str());
  }

  inline StringRef makeFreshConstraintName() {
    std::stringstream ss;
    ss << PL_ConstraintPrefix << "_" << getNextUniqueConstraintID();
    return addFreshName(ss.str());
  }

  inline void addConstraint(Constraint *Cons) {
    assert(Cons && "Internal Error: unexpected null-pointer");
    ConstraintSet.insert(Cons);
    if (EffectInclusionConstraint *EIC =
          dyn_cast<EffectInclusionConstraint>(Cons)) {
      addInclusionConstraint(EIC->getDef(), EIC);
    }
  }

  /*inline void addInclusionConstraint(EffectInclusionConstraint *EIC) {
    addConstraint(EIC);
    addInclusionConstraint(EIC->getDef(), EIC);
  }*/

  /*inline void addNIConstraint(EffectNIConstraint *NIC) {
    NIConstraints.insert(NIC);
  }*/

  void solveInclusionConstraints();
  // Default annotations
  AnnotationSet makeDefaultType(ValueDecl *ValD, long ParamCount);
  RplVector *makeDefaultBaseArgs(const RecordDecl *Derived, long NumArgs);

  void createSymbolTableEntry(const Decl *D);

  VarRpl *createFreshRplVar(const ValueDecl *D);

  inline AnnotationSet makeDefaultClassParams(RecordDecl *RecD) {
    return AnnotScheme->makeClassParams(RecD);
  }

  inline AnnotationSet makeDefaultEffectSummary(const FunctionDecl *F) {
    return AnnotScheme->makeEffectSummary(F);
  }

}; // End class SymbolTable.

class SymbolTableEntry {
  friend class SymbolTable;
  // Fields
  StringRef PrologName;
  ASaPType *Typ;
  ParameterVector *ParamVec;
  RegionNameSet *RegnNameSet;
  EffectSummary *EffSum;

  /// \brief Inheritance map for declaration
  InheritanceMapT *InheritanceMap;

  /// \brief True when the Inheritance Substitution Vector map has been
  /// computed (& cached)
  bool ComputedInheritanceSubVec;
  /// \brief Inheritance Substitution Vector for declaration (null until
  /// computed & cached)
  SubstitutionVector *InheritanceSubVec;

  // Private Methods
  void computeInheritanceSubVec();

public:
  SymbolTableEntry(StringRef PrologName);
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
  inline const StringRef getPrologName() const { return PrologName; }

  // Setters.
  inline void setType(ASaPType *T) { Typ = T; }
  inline void setParameterVector(ParameterVector *PV) { ParamVec = PV; }
  void addToParameterVector(ParameterVector *&PV);
  inline void setRegionNameSet(RegionNameSet *RNS) { RegnNameSet = RNS; }
  inline void setEffectSummary(EffectSummary *ES) { EffSum = ES; }

  // Lookup.
  const NamedRplElement *lookupRegionName(llvm::StringRef Name);
  const ParamRplElement *lookupParameterName(llvm::StringRef Name);

  // Adders.
  void addRegionName(llvm::StringRef Name, llvm::StringRef PrologName);
  void addParameterName(llvm::StringRef Name, llvm::StringRef PrologName);
  bool addInclusionConstraint(EffectInclusionConstraint *EIC);

  // Deleters
  inline void deleteEffectSummary();

  /// \brief Add an entry to the Inheritance Map
  bool addBaseTypeAndSub(const RecordDecl *BaseRD,
                         SymbolTableEntry *Base,
                         SubstitutionVector *&SubV);

  const SubstitutionVector *getSubVec(const RecordDecl *Base) const;
  const SubstitutionVector *getInheritanceSubVec();

protected:
  inline EffectSummary *getNonConstEffectSummary() const { return EffSum; }

}; // End class SymbolTableEntry.
} // End namespace asap.
} // End namespace clang.


#endif


