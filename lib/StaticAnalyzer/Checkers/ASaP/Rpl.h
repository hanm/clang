//=== Rpl.h - Safe Parallelism checker -----*- C++ -*--------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This file defines the Rpl and RplVector classes used by the Safe
// Parallelism checker, which tries to prove the safety of parallelism
// given region and effect annotations.
//
//===----------------------------------------------------------------===//

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_RPL_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_RPL_H

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include "ASaPUtil.h"  // Included for the 2 uses of OSv2
#include "OwningPtrSet.h"
#include "OwningVector.h"

namespace clang {
namespace asap {

class RplElement {
public:
  /// Discriminator for LLVM-style RTTI (dyn_cast<> et al.)
  enum RplElementKind {
      RK_Special,
      RK_Star,
      RK_Named,
      RK_Parameter,
      RK_Capture,
      RK_Var //Rpl variable
  };

private:
  /// \brief Stores the LLVM-style RTTI discriminator
  const RplElementKind Kind;
  /// FIXME: protect copy constructor
  //RplElement(RplElement &Elmt);

public:
  /// API
  RplElementKind getKind() const { return Kind; }
  RplElement(RplElementKind K) : Kind(K) {}

  virtual bool isFullySpecified() const { return true; }

  virtual StringRef getName() const = 0;
  virtual bool operator == (const RplElement &That) const {
    return (this == &That) ? true : false;
  }
  virtual ~RplElement() {}
}; // end class RplElement

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
class VarRplElement : public RplElement {
  ///Fields
  StringRef name;

public:
  /// Constructor
  VarRplElement(StringRef name) : RplElement(RK_Var), name(name) {}
  virtual ~VarRplElement() {}

  /// Methods
  virtual StringRef getName() const { return name; }

  static bool classof(const RplElement *R) {
    return R->getKind() == RK_Var;
  }
};


class Effect;


class Rpl {
public:
  /// Static Constants
  static const char RPL_SPLIT_CHARACTER = ':';
  static const StringRef RPL_LIST_SEPARATOR;
  static const StringRef RPL_NAME_SPEC;

  /// Static Functions
  /// \brief Return true when input is a valid region name or param declaration
  static bool isValidRegionName(const StringRef& s);


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

    RplRef& stripFirst() {
      firstIdx++;
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

    bool isDisjointLeft(RplRef &That);
    bool isDisjointRight(RplRef &That);

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

  static std::pair<StringRef, StringRef> splitRpl(StringRef &String);
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
  bool isUnder(const Rpl &That) const;

  // Inclusion
  /// \brief Returns true iff this RPL is included in That
  bool isIncludedIn(const Rpl &That) const;

  /// \brief Returns true iff this RPL is disjoint from That
  bool isDisjoint(const Rpl &That) const;

  // Substitution (Rpl)
  /// \brief this[FromEl <- ToRpl]
  //void substitute(const RplElement& FromEl, const Rpl& ToRpl);
  void substitute(const Substitution *S);

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

//////////////////////////////////////////////////////////////////////////

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
}; // end class CaptureRplElement


//////////////////////////////////////////////////////////////////////////
#ifndef PARAM_VECTOR_SIZE
  #define PARAM_VECTOR_SIZE 8
#endif

class ParameterSet:
  public llvm::SmallPtrSet<const ParamRplElement*, PARAM_VECTOR_SIZE> {
public:
  //typedef llvm::SmallPtrSet<const ParamRplElement*, PARAM_VECTOR_SIZE> BaseClass;

  bool hasElement(const RplElement *Elmt) const {
    for(const_iterator I = begin(), E = end();
        I != E; ++I) {
      if (Elmt == *I)
        return true;
    }
    return false;
  }
}; // end class TempParamVector

class ParameterVector :
  public OwningVector<ParamRplElement, PARAM_VECTOR_SIZE> {
private:
  // protect copy constructor by making it private.
  ParameterVector(const ParameterVector &From);
public:
  typedef OwningVector<ParamRplElement, PARAM_VECTOR_SIZE> BaseClass;
  // Constructor
  ParameterVector() : BaseClass() {}

  ParameterVector(ParamRplElement &P) : BaseClass(P) {}

  void addToParamSet(ParameterSet *PSet) const {
    for(VectorT::const_iterator I = begin(), E = end();
        I != E; ++I) {
      const ParamRplElement *El = *I;
      PSet->insert(El);
    }
  }

  /// \brief Returns the Parameter at position Idx
  const ParamRplElement *getParamAt(size_t Idx) const {
    assert(Idx < size());
    return (*this)[Idx];
  }

  /// \brief Returns the ParamRplElement with name=Name or null.
  const ParamRplElement *lookup(StringRef Name) const {
    for(VectorT::const_iterator I = begin(), E = end();
        I != E; ++I) {
      const ParamRplElement *El = *I;
      if (El->getName().compare(Name) == 0)
        return El;
    }
    return 0;
  }

  /// \brief Returns true if the argument RplElement is contained.
  bool hasElement(const RplElement *Elmt) const {
    for(VectorT::const_iterator I = begin(), E = end();
        I != E; ++I) {
      if (Elmt == *I)
        return true;
    }
    return false;
  }

  void take(ParameterVector *&PV) {
    if (!PV)
      return;

    BaseClass::take(PV);
    assert(PV->size()==0);
    delete PV;
    PV = 0;
  }

}; // end class ParameterVector

//////////////////////////////////////////////////////////////////////////
#ifndef RPL_VECTOR_SIZE
  #define RPL_VECTOR_SIZE 4
#endif

class RplVector : public OwningVector<Rpl, RPL_VECTOR_SIZE> {
  //friend class RplVector;
  typedef OwningVector<Rpl, RPL_VECTOR_SIZE> BaseClass;
  public:
  /// Constructor
  RplVector() : BaseClass() {}
  RplVector(const Rpl &R) : BaseClass(R) {}

  RplVector(const ParameterVector &ParamVec) {
    for(ParameterVector::const_iterator
          I = ParamVec.begin(),
          E = ParamVec.end();
        I != E; ++I) {
      if (*I) {
        Rpl Param(**I);
        push_back(Param);
      }
    }
  }

  /// \brief Add the argument RPL to the front of the RPL vector.
  inline void push_front (const Rpl *R) {
    assert(R);
    VectorT::insert(begin(), new Rpl(*R));
  }
  /// \brief Remove and return the first RPL in the vector.
  inline std::auto_ptr<Rpl> deref() { return pop_front(); }

  /// \brief Same as performing deref() DerefNum times.
  std::auto_ptr<Rpl> deref(size_t DerefNum) {
    std::auto_ptr<Rpl> Result;
    assert(DerefNum < size());
    for (VectorT::iterator I = begin();
         DerefNum > 0 && I != end(); ++I, --DerefNum) {
      Result.reset(*I);
      I = VectorT::erase(I);
    }
    return Result;
  }

  /// \brief Return a pointer to the RPL at position Idx in the vector.
  inline const Rpl *getRplAt(size_t Idx) const {
    assert(Idx < size() && "attempted to access beyond last RPL element");
    return (*this)[Idx];
  }

  /// \brief Joins this to That.
  void join(RplVector *That) {
    if (!That)
      return;
    assert(That->size() == this->size());

    VectorT::iterator
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
    assert(That.size() == this->size());

    VectorT::const_iterator
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
  void substitute(const Substitution *S) {
    for(VectorT::const_iterator I = begin(), E = end();
         I != E; ++I) {
      if (*I)
        (*I)->substitute(S);
    }
  }
  /// \brief Print RPL vector
  void print(llvm::raw_ostream &OS) const {
    VectorT::const_iterator I = begin(), E = end();
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
    (A->size() >= B->size()) ? ( LHS = new RplVector(*A), RHS = B)
                             : ( LHS = new RplVector(*B), RHS = A);
    // fold RHS into LHS
    VectorT::const_iterator RHSI = RHS->begin(), RHSE = RHS->end();
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
    VectorT::iterator RHSI = RHS->begin();
    while (RHSI != RHS->end()) {
      Rpl *R = *RHSI;
      LHS->VectorT::push_back(R);
      RHSI = RHS->VectorT::erase(RHSI);
    }
    delete RHS;
    A = B = 0;
    return LHS;
  }
}; // End class RplVector.

//////////////////////////////////////////////////////////////////////////
#ifndef REGION_NAME_SET_SIZE
  #define REGION_NAME_SET_SIZE 8
#endif
class RegionNameSet
: public OwningPtrSet<NamedRplElement, REGION_NAME_SET_SIZE> {

public:
  /// \brief Returns the NamedRplElement with name=Name or null.
  const NamedRplElement *lookup (StringRef Name) {
    for (SetT::iterator I = begin(), E = end();
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



