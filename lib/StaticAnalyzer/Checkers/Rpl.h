//=== Rpl.h - Safe Parallelism checker -----*- C++ -*--------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This files defines the Rpl and RplVector classes used by the Safe
// Parallelism checker, which tries to prove the safety of parallelism
// given region and effect annotations.
//
//===----------------------------------------------------------------===//

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_RPL_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_RPL_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/AST/Attr.h"

namespace clang {
namespace asap {

#define ASAP_DEBUG
#define ASAP_DEBUG_VERBOSE2
#ifdef ASAP_DEBUG
  static raw_ostream& os = llvm::errs();
#else
  static raw_ostream &os = llvm::nulls();
#endif

#ifdef ASAP_DEBUG_VERBOSE2
  static raw_ostream &OSv2 = llvm::errs();
#else
  static raw_ostream &OSv2 = llvm::nulls();
#endif

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

  virtual llvm::StringRef getName() const = 0;
  virtual bool operator == (const RplElement &That) const {
    return (this == &That) ? true : false;
  }
  virtual ~RplElement() {}
};

class SpecialRplElement : public RplElement {
  /// Fields
  const llvm::StringRef name;

public:
  /// Constructor
  SpecialRplElement(llvm::StringRef name) : RplElement(RK_Special), name(name) {}
  virtual ~SpecialRplElement() {}

  /// Methods
  virtual llvm::StringRef getName() const { return name; }

  static bool classof(const RplElement *R) {
    return R->getKind() == RK_Special;
  }
};

class StarRplElement : public RplElement {
public:
  /// Constructor/Destructor
  StarRplElement () : RplElement(RK_Star) {}
  virtual ~StarRplElement() {}

  /// Methods
  virtual bool isFullySpecified() const { return false; }
  virtual llvm::StringRef getName() const { return "*"; }

  static bool classof(const RplElement *R) {
    return R->getKind() == RK_Star;
  }
}; // end class StarRplElement

///-////////////////////////////////////////
class NamedRplElement : public RplElement {
  /// Fields
  const llvm::StringRef name;

public:
  /// Constructor
  NamedRplElement(llvm::StringRef name) : RplElement(RK_Named), name(name) {}
  virtual ~NamedRplElement() {}
  /// Methods
  virtual llvm::StringRef getName() const { return name; }

  static bool classof(const RplElement *R) {
    return R->getKind() == RK_Named;
  }

};

///-////////////////////////////////////////
class ParamRplElement : public RplElement {
  ///Fields
  llvm::StringRef name;

public:
  /// Constructor
  ParamRplElement(llvm::StringRef name) : RplElement(RK_Parameter), name(name) {}
  virtual ~ParamRplElement() {}

  /// Methods
  virtual llvm::StringRef getName() const { return name; }

  static bool classof(const RplElement *R) {
    return R->getKind() == RK_Parameter;
  }
};

class Effect;


class Rpl {
  friend class Rpl;
public:
  /// Static Constants
  static const char RPL_SPLIT_CHARACTER = ':';
  static const StringRef RPL_LIST_SEPARATOR;
  static const StringRef RPL_NAME_SPEC;

  /// Static Functions
  /// \brief Return true when input is a valid region name or param declaration
  static bool isValidRegionName(const llvm::StringRef& s);


  /// Types
#ifndef RPL_VECTOR_SIZE
  #define RPL_VECTOR_SIZE 4
#endif
  typedef llvm::SmallVector<Rpl*, RPL_VECTOR_SIZE> RplVectorT;

private:
#ifndef RPL_ELEMENT_VECTOR_SIZE
#define RPL_ELEMENT_VECTOR_SIZE 8
#endif
typedef llvm::SmallVector<const RplElement*,
                          RPL_ELEMENT_VECTOR_SIZE> RplElementVectorTy;
  /// Fields
  /// Note: RplElements are not owned by Rpl class.
  /// They are *NOT* destroyed with the Rpl.
  RplElementVectorTy RplElements;
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
    RplRef(const Rpl& R) : rpl(R) {
      firstIdx = 0;
      lastIdx = rpl.RplElements.size()-1;
    }
    /// Printing (Rpl Ref)
    void print(llvm::raw_ostream& OS) const {
      int I = firstIdx;
      for (; I < lastIdx; ++I) {
        OS << rpl.RplElements[I]->getName() << RPL_SPLIT_CHARACTER;
      }
      // print last element
      if (I==lastIdx)
        OS << rpl.RplElements[I]->getName();
    }

    std::string toString() {
      std::string SBuf;
      llvm::raw_string_ostream OS(SBuf);
      print(OS);
      return std::string(OS.str());
    }

    /// Getters
    const RplElement* getFirstElement() const {
      return rpl.RplElements[firstIdx];
    }
    const RplElement* getLastElement() const {
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

    /// \brief Under: true  iff this <= RHS
    bool isUnder(RplRef& RHS);

    /// \brief Inclusion: true  iff  this c= rhs
    bool isIncludedIn(RplRef& RHS);

  }; /// end class RplRef
  ///////////////////////////////////////////////////////////////////////////
  public:
  /// Constructors
  Rpl() : FullySpecified(true) {}

  Rpl(const RplElement &Elm) :
    FullySpecified(Elm.isFullySpecified()) {
    RplElements.push_back(&Elm);
  }

  /// Copy Constructor
  Rpl(const Rpl &That) :
      RplElements(That.RplElements),
      FullySpecified(That.FullySpecified)
  {}

  static std::pair<llvm::StringRef, llvm::StringRef> splitRpl(llvm::StringRef &String);
  void print(llvm::raw_ostream &OS) const;
  std::string toString() const;

  // Getters
  /// \brief Returns the last of the RPL elements of this RPL.
  inline const RplElement* getLastElement() const {
    return RplElements.back();
  }
  /// \brief Returns the first of the RPL elements of this RPL.
  inline const RplElement* getFirstElement() const {
    return RplElements.front();
  }

  /// \brief Returns the number of RPL elements.
  inline size_t length() const {
    return RplElements.size();
  }
  // Setters
  /// \brief Appends an RPL element to this RPL.
  inline void appendElement(const RplElement* RplElm) {
    if (RplElm) {
      RplElements.push_back(RplElm);
      if (RplElm->isFullySpecified() == false)
        FullySpecified = false;
    }
  }
  // Predicates
  inline bool isFullySpecified() { return FullySpecified; }
  inline bool isEmpty() { return RplElements.empty(); }

  // Nesting (Under)
  /// \brief Returns true iff this is under That
  bool isUnder(const Rpl& That) const;

  // Inclusion
  /// \brief Returns true iff this RPL is included in That
  bool isIncludedIn(const Rpl& That) const;

  // Substitution (Rpl)
  /// \brief this[FromEl <- ToRpl]
  void substitute(const RplElement& FromEl, const Rpl& ToRpl);

  /// \brief Append to this RPL the argument Rpl but without its head element.
  void appendRplTail(Rpl* That);

  /// \brief Return the upper bound of an RPL (different when RPL is captured).
  Rpl *upperBound();

  /// \brief Join this to That (by modifying this).
  void join(Rpl* That);

  bool operator == (const RplElement &That) const {
    if (RplElements.size() == 1 &&
        *RplElements[0] == That)
      return true;
    else
      return false;
  }

  bool operator != (const RplElement &That) const {
    return !(*this==That);
  }
  // Capture
  // TODO: caller must deallocate Rpl and its element
  Rpl* capture();
}; // end class Rpl

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
  virtual llvm::StringRef getName() const { return "rho"; }

  virtual bool isFullySpecified() const { return false; }

  Rpl& upperBound() const { return includedIn; }

  static bool classof(const RplElement *R) {
    return R->getKind() == RK_Capture;
  }
}; // end class CaptureRplElement

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
  const ParamRplElement *lookup(llvm::StringRef Name) const {
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
  /// \brief Returns true if the argument RplElement is contained.
  bool hasElement(const RplElement *Elmt) const {
    for(ParamVecT::const_iterator
          I = ParamVec.begin(),
          E = ParamVec.end();
        I != E; ++I) {
      if (Elmt == *I)
        return true;
    }
    return false;
  }
}; // end class ParameterVector

class RplVector {
  friend class RplVector;
  private:
  /// Fields
  Rpl::RplVectorT RplV;

  public:
  /// Constructor
  RplVector() {}
  RplVector(const Rpl &R) {
    RplV.push_back(new Rpl(R));
  }

  RplVector(const ParameterVector &ParamVec) {
    for(ParameterVector::const_iterator
          I = ParamVec.begin(),
          E = ParamVec.end();
        I != E; ++I) {
      RplV.push_back(new Rpl(*(*I)));
    }
  }

  RplVector(const RplVector &RV) {
    for (Rpl::RplVectorT::const_iterator
            I = RV.RplV.begin(),
            E = RV.RplV.end();
         I != E; ++I) {
      RplV.push_back(new Rpl(*(*I))); // copy Rpls
    }
  }

  /// Destructor
  ~RplVector() {
    for (Rpl::RplVectorT::const_iterator
            I = RplV.begin(),
            E = RplV.end();
         I != E; ++I) {
      delete (*I);
    }
  }
  // Types
  typedef Rpl::RplVectorT::iterator iterator;
  typedef Rpl::RplVectorT::const_iterator const_iterator;

  // Methods
  /// \brief Return an iterator at the first RPL of the vector.
  inline Rpl::RplVectorT::iterator begin () { return RplV.begin(); }
  /// \brief Return an iterator past the last RPL of the vector.
  inline Rpl::RplVectorT::iterator end () { return RplV.end(); }
  /// \brief Return a const_iterator at the first RPL of the vector.
  inline Rpl::RplVectorT::const_iterator begin () const { return RplV.begin(); }
  /// \brief Return a const_iterator past the last RPL of the vector.
  inline Rpl::RplVectorT::const_iterator end () const { return RplV.end(); }
  /// \brief Return the size of the RPL vector.
  inline size_t size () const { return RplV.size(); }
  /// \brief Append the argument RPL to the RPL vector.
  inline void push_back (const Rpl *R) {
    assert(R);
    RplV.push_back(new Rpl(*R));
  }
  /// \brief Add the argument RPL to the front of the RPL vector.
  inline void push_front (const Rpl *R) {
    assert(R);
    RplV.insert(RplV.begin(), new Rpl(*R));
  }
  /// \brief Remove and return the first RPL in the vector.
  Rpl *pop_front() {
    assert(RplV.size() > 0);
    Rpl *Result = RplV.front();
    RplV.erase(RplV.begin());
    return Result;
  }
  /// \brief Remove and return the first RPL in the vector.
  inline Rpl *deref() { return pop_front(); }
  /// \brief Same as performing deref() DerefNum times.
  Rpl *deref(size_t DerefNum) {
    Rpl *Result = 0;
    //assert(DerefNum >=0);
    assert(DerefNum < RplV.size());
    for (Rpl::RplVectorT::iterator
            I = RplV.begin(),
            E = RplV.end();
         DerefNum > 0 && I != E; ++I, --DerefNum) {
      if (Result)
        delete Result;
      Result = *I;
      I = RplV.erase(I);
    }
    return Result;
  }

  /// \brief Return a pointer to the RPL at position Idx in the vector.
  inline const Rpl *getRplAt(size_t Idx) const {
    //assert(Idx>=0);
    if (Idx >= RplV.size()) {
      OSv2 << "DEBUG:: getRplAt(" << Idx << "), but size = " << RplV.size()
           << "\n";
    }
    assert(Idx < RplV.size());
    return RplV[Idx];
  }

  /// \brief Joins this to That.
  void join(RplVector *That) {
    if (!That)
      return;
    assert(That->size() == this->size());

    Rpl::RplVectorT::iterator
            ThatI = That->begin(),
            ThisI = this->begin(),
            ThatE = That->end(),
            ThisE = this->end();
    for ( ;
          ThatI != ThatE && ThisI != ThisE;
          ++ThatI, ++ThisI) {
      Rpl *LHS = *ThisI;
      Rpl *RHS = *ThatI;
      assert(LHS);
      LHS->join(RHS); // modify *ThisI in place
      //ThisI = this->RplV.erase(ThisI);
      //this->RplV.assign(); // can we use assign instead of erase and insert?
      //ThisI = this->RplV.insert(ThisI, Join);
    }
    return;
  }

  /// \brief Return true when this is included in That, false otherwise.
  bool isIncludedIn (const RplVector &That) const {
    bool Result = true;
    assert(That.RplV.size() == this->RplV.size());

    Rpl::RplVectorT::const_iterator
            ThatI = That.begin(),
            ThisI = this->begin(),
            ThatE = That.end(),
            ThisE = this->end();
    for ( ;
          ThatI != ThatE && ThisI != ThisE;
          ++ThatI, ++ThisI) {
      Rpl *LHS = *ThisI;
      Rpl *RHS = *ThatI;
      if (!LHS->isIncludedIn(*RHS)) {
        Result = false;
        break;
      }
    }
    OSv2 << "DEBUG:: [" << this->toString() << "] is " << (Result?"":"not ")
        << "included in [" << That.toString() << "]\n";
    return Result;
  }

  /// \brief Substitution this[FromEl <- ToRpl] (over RPL vector)
  void substitute(const RplElement &FromEl, const Rpl &ToRpl) {
    for(Rpl::RplVectorT::const_iterator
            I = RplV.begin(),
            E = RplV.end();
         I != E; ++I) {
      if (*I)
        (*I)->substitute(FromEl, ToRpl);
    }
  }
  /// \brief Print RPL vector
  void print(llvm::raw_ostream &OS) const {
    Rpl::RplVectorT::const_iterator
      I = RplV.begin(),
      E = RplV.end();
    for(; I < E-1; ++I) {
      (*I)->print(OS);
      OS << ", ";
    }
    // print last one
    if (I != E)
      (*I)->print(OS);
  }
  /// \brief Return a string for the RPL vector.
  inline std::string toString() const {
    std::string SBuf;
    llvm::raw_string_ostream OS(SBuf);
    print(OS);
    return std::string(OS.str());
  }

  /// \brief Returns the union of two RPL Vectors by copying its inputs.
  static RplVector *merge(const RplVector *A, const RplVector *B) {
    if (!A && !B)
      return 0;
    if (!A && B)
      return new RplVector(*B);
    if (A && !B)
      return new RplVector(*A);
    // invariant henceforth A!=null && B!=null
    RplVector *LHS;
    const RplVector *RHS;
    OSv2 << "DEBUG:: RplVector::merge : both Vectors are non-null!\n";
    (A->size() >= B->size()) ? ( LHS = new RplVector(*A), RHS = B)
                             : ( LHS = new RplVector(*B), RHS = A);
    // fold RHS into LHS
    Rpl::RplVectorT::const_iterator RHSI = RHS->begin(), RHSE = RHS->end();
    while (RHSI != RHSE) {
      LHS->push_back(*RHSI);
      ++RHSI;
    }
    return LHS;
  }

  /// \brief Returns the union of two RPL Vectors but destroys its inputs.
  static RplVector *destructiveMerge(RplVector *&A, RplVector *&B) {
    if (!A)
      return B;
    if (!B)
      return A;
    // invariant henceforth A!=null && B!=null
    RplVector *LHS, *RHS;
    (A->size() >= B->size()) ? ( LHS = A, RHS = B)
                             : ( LHS = B, RHS = A);
    // fold RHS into LHS
    Rpl::RplVectorT::iterator RHSI = RHS->begin(), RHSE = RHS->end();
    while (RHSI != RHSE) {
      LHS->RplV.push_back(*RHSI);
      RHSI = RHS->RplV.erase(RHSI);
    }
    delete RHS;
    A = B = 0;
    return LHS;
  }
}; // End class RplVector.

class RegionNameSet {
private:
  // Fields
#ifndef REGION_NAME_SET_SIZE
  #define REGION_NAME_SET_SIZE 8
#endif
  typedef llvm::SmallPtrSet<const NamedRplElement*, REGION_NAME_SET_SIZE>
    RegnNameSetTy;
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
  const NamedRplElement *lookup (llvm::StringRef Name) {
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
}; // End class RegionNameSet.
} // End namespace asap.

} // End namespace clang.

#endif



