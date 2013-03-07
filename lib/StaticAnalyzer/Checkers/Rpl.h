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

// FIXME
static llvm::raw_ostream& OSv2 = llvm::errs();
static llvm::raw_ostream& os = llvm::errs();

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
  virtual bool operator == (const RplElement& That) const {
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

extern const StarRplElement *STARRplElmt;
extern const SpecialRplElement *ROOTRplElmt;
extern const SpecialRplElement *LOCALRplElmt;
extern const Effect *WritesLocal;

/// \brief returns a special RPL element (Root, Local, *, ...) or NULL
const RplElement* getSpecialRplElement(const llvm::StringRef& s);

/// Return true when the input string is a special RPL element
/// (e.g., '*', '?', 'Root'.
// TODO (?): '?', 'Root'
bool isSpecialRplElement(const llvm::StringRef& s);

/**
*  Return true when the input string is a valid region
*  name or region parameter declaration
*/
bool isValidRegionName(const llvm::StringRef& s);

class Rpl {
  friend class Rpl;
public:
  /// Static Constants
  static const char RPL_SPLIT_CHARACTER = ':';

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
      OSv2  << "DEBUG:: ~~~~~~~~isUnder[RplRef]("
          << this->toString() << ", " << rhs.toString() << ")\n";
      /// R <= Root
      if (rhs.isEmpty())
        return true;
      if (isEmpty()) /// and rhs is not Empty
        return false;
      /// R <= R' <== R c= R'
      if (isIncludedIn(rhs)) return true;
      /// R:* <= R' <== R <= R'
      if (getLastElement() == STARRplElmt)
        return stripLast().isUnder(rhs);
      /// R:r <= R' <==  R <= R'
      /// R:[i] <= R' <==  R <= R'
      return stripLast().isUnder(rhs);
      // TODO z-regions
    }

    /// Inclusion: this c= rhs
    bool isIncludedIn(RplRef& rhs) {
      OSv2  << "DEBUG:: ~~~~~~~~isIncludedIn[RplRef]("
          << this->toString() << ", " << rhs.toString() << ")\n";
      if (rhs.isEmpty()) {
        /// Root c= Root
        if (isEmpty()) return true;
        /// RPL c=? Root and RPL!=Root ==> not included
        else /*!isEmpty()*/ return false;
      } else { /// rhs is not empty
        /// R c= R':* <==  R <= R'
        if (*rhs.getLastElement() == *STARRplElmt) {
          OSv2 <<"DEBUG:: isIncludedIn[RplRef] last elmt of RHS is '*'\n";
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
  inline const RplElement* getFirstElement() {
    return RplElements.front();
  }

  /// \brief Returns the number of RPL elements.
  inline size_t length() {
    return RplElements.size();
  }
  // Setters
  /// \brief Appends an RPL element to this RPL.
  inline void appendElement(const RplElement* RplElm) {
    RplElements.push_back(RplElm);
    if (RplElm->isFullySpecified() == false)
      FullySpecified = false;
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
};

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

// TODO support substitution over multiple parameters
class Substitution {
private:
  // Fields
  const RplElement *FromEl; // RplElement not owned by class
  const Rpl *ToRpl;         // Rpl not owned by class
public:
  // Constructor
  Substitution() : FromEl(0),  ToRpl(0) {}

  Substitution(const RplElement *FromEl, const Rpl *ToRpl) :
    FromEl(FromEl), ToRpl(ToRpl) {
    assert(FromEl);
    assert(ToRpl);
  }
  // Getters
  inline const RplElement *getFrom() const { return FromEl; }
  inline const Rpl *getTo() const { return ToRpl; }
  // Setters
  void set(const RplElement *FromEl, const Rpl *ToRpl) {
    this->FromEl = FromEl;
    this->ToRpl = ToRpl;
  }
  // Apply
  /// \brief Apply substitution to RPL
  inline void applyTo(Rpl *R) const {
    if (FromEl && ToRpl) {
      assert(R);
      R->substitute(*FromEl, *ToRpl);
    }
  }
  // print
  /// \brief Print Substitution: [From<-To]
  void print(raw_ostream &OS) const {
    llvm::StringRef FromName = "<MISSING>";
    llvm::StringRef ToName = "<MISSING>";
    if (FromEl)
      FromName = FromEl->getName();
    if (ToRpl)
      ToName = ToRpl->toString();
    OS << "[" << FromName << "<-" << ToName << "]";
  }
  /// \brief Return a string for the Substitution.
  inline std::string toString() const {
    std::string SBuf;
    llvm::raw_string_ostream OS(SBuf);
    print(OS);
    return std::string(OS.str());
  }
}; // end class Substitution

// An ordered sequence of substitutions
class SubstitutionVector {
public:
  // Type
#ifndef SUBSTITUTION_VECTOR_SIZE
  #define SUBSTITUTION_VECTOR_SIZE 4
#endif
  typedef llvm::SmallVector<Substitution*, SUBSTITUTION_VECTOR_SIZE>
    SubstitutionVecT;
private:
  SubstitutionVecT SubV;
public:
  // Constructor
  SubstitutionVector() {}
  SubstitutionVector(Substitution *S) { SubV.push_back(S); }
  // Methods
  /// \brief Return an iterator at the first RPL of the vector.
  inline SubstitutionVecT::iterator begin () { return SubV.begin(); }
  /// \brief Return an iterator past the last RPL of the vector.
  inline SubstitutionVecT::iterator end () { return SubV.end(); }
  /// \brief Return a const_iterator at the first RPL of the vector.
  inline SubstitutionVecT::const_iterator begin () const { return SubV.begin(); }
  /// \brief Return a const_iterator past the last RPL of the vector.
  inline SubstitutionVecT::const_iterator end () const { return SubV.end(); }
  /// \brief Return the size of the RPL vector.
  inline size_t size () const { return SubV.size(); }
  /// \brief Append the argument Substitution to the Substitution vector.
  inline void push_back(Substitution *S) {
    assert(S);
    SubV.push_back(S);
  }

  // Apply
  void applyTo(Rpl *R) const {
    assert(R);
    for(SubstitutionVecT::const_iterator I = SubV.begin(), E = SubV.end();
        I != E; ++I) {
      assert(*I);
      (*I)->applyTo(R);
    }
  }
  // Print
  /// \brief Print Substitution vector.
  void print(raw_ostream &OS) const {
    SubstitutionVecT::const_iterator
      I = SubV.begin(),
      E = SubV.end();
    for(; I != E; ++I) {
      assert(*I);
      (*I)->print(OS);
    }
  }

  /// \brief Return a string for the Substitution vector.
  inline std::string toString() const {
    std::string SBuf;
    llvm::raw_string_ostream OS(SBuf);
    print(OS);
    return std::string(OS.str());
  }
}; // End class Substituion.

class EffectSummary;

class Effect {
public:
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
  bool isSubEffectKindOf(const Effect &E) const;

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

  /// \brief Print Effect Kind to raw output stream.
  bool printEffectKind(raw_ostream &OS) const;

  /// \brief Print Effect to raw output stream.
  void print(raw_ostream &OS) const;

  /// \brief Return a string for this Effect.
  std::string toString() const;

  // Predicates
  /// \brief Returns true iff this is a no_effect.
  inline bool isNoEffect() const {
    return (Kind == EK_NoEffect) ? true : false;
  }
  /// \brief Returns true iff this effect has RPL argument.
  inline bool hasRplArgument() const { return !isNoEffect(); }
  /// \brief Returns true if this effect is atomic.
  inline bool isAtomic() const {
    return (Kind==EK_AtomicReadsEffect ||
            Kind==EK_AtomicWritesEffect) ? true : false;
  }

  // Getters
  /// \brief Return effect kind.
  inline EffectKind getEffectKind() const { return Kind; }
  /// \brief Return RPL (which may be null for no_effect).
  inline const Rpl *getRpl() const { return R; }
  /// \brief Return corresponding Attribute.
  inline const Attr *getAttr() const { return Attribute; }
  /// \brief Return source location.
  inline SourceLocation getLocation() const {
    return Attribute->getLocation();
  }

  /// \brief substitute (Effect)
  inline void substitute(const Substitution &S) {
    if (R)
      S.applyTo(R);
  }

  /// \brief SubEffect Rule: true if this <= e
  ///
  ///  RPL1 c= RPL2   E1 c= E2
  /// ~~~~~~~~~~~~~~~~~~~~~~~~~
  ///    E1(RPL1) <= E2(RPL2)
  bool isSubEffectOf(const Effect &That) const {
    bool Result = (isNoEffect() ||
            (isSubEffectKindOf(That) && R->isIncludedIn(*(That.R))));
    OSv2  << "DEBUG:: ~~~isSubEffect(" << this->toString() << ", "
        << That.toString() << ")=" << (Result ? "true" : "false") << "\n";
    return Result;
  }
  /// \brief Returns covering effect in effect summary or null.
  const Effect *isCoveredBy(const EffectSummary &ES);

}; // end class Effect

/////////////////////////////////////////////////////////////////////////////
class EffectVector {
public:
  // Types
#ifndef EFFECT_VECTOR_SIZE
  #define EFFECT_VECTOR_SIZE 16
#endif
  typedef llvm::SmallVector<Effect*, EFFECT_VECTOR_SIZE> EffectVectorT;

private:
  // Fields
  EffectVectorT EffV;

public:
  /// Constructor
  EffectVector() {}
  EffectVector(const Effect &E) {
    EffV.push_back(new Effect(E));
  }

  /// Destructor
  ~EffectVector() {
    for (EffectVectorT::const_iterator
            I = EffV.begin(),
            E = EffV.end();
         I != E; ++I) {
      delete (*I);
    }
  }

  // Methods
  /// \brief Return an iterator at the first Effect of the vector.
  inline EffectVectorT::iterator begin() { return EffV.begin(); }
  /// \brief Return an iterator past the last Effect of the vector.
  inline EffectVectorT::iterator end() { return EffV.end(); }
  /// \brief Return a const_iterator at the first Effect of the vector.
  inline EffectVectorT::const_iterator begin () const { return EffV.begin(); }
  /// \brief Return a const_iterator past the last Effect of the vector.
  inline EffectVectorT::const_iterator end () const { return EffV.end(); }
  /// \brief Return the size of the Effect vector.
  inline size_t size () const { return EffV.size(); }
  /// \brief Append the argument Effect to the Effect vector.
  inline void push_back (const Effect *E) {
    assert(E);
    EffV.push_back(new Effect(*E));
  }
  /// \brief Return the last Effect in the vector after removing it.
  inline Effect *pop_back_val() { return EffV.pop_back_val(); }

  void substitute(const Substitution &S) {
    for (EffectVectorT::const_iterator
            I = EffV.begin(),
            E = EffV.end();
         I != E; ++I) {
      Effect *Eff = *I;
      Eff->substitute(S);
    }
  }

}; // end class EffectVector

/////////////////////////////////////////////////////////////////////////////
/// \brief Implements a Set of Effects
class EffectSummary {
private:
  // Fields
#ifndef EFFECT_SUMMARY_SIZE
#define EFFECT_SUMMARY_SIZE 8
#endif
  typedef llvm::SmallPtrSet<const Effect*, EFFECT_SUMMARY_SIZE>
    EffectSummarySetT;
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
  /// \brief Returns the size of the EffectSummary
  inline size_t size() const { return EffectSum.size(); }
  /// \brief Returns a const_iterator at the first element of the summary.
  inline const_iterator begin() const { return EffectSum.begin(); }
  /// \brief Returns a const_iterator past the last element of the summary.
  inline const_iterator end() const { return EffectSum.end(); }

  /// \brief Returns the effect that covers Eff or null otherwise.
  const Effect *covers(const Effect *Eff) const;

  /// \brief Returns true iff insertion was successful.
  inline bool insert(const Effect *Eff) {
    return EffectSum.insert(Eff);
  }

  typedef llvm::SmallVector<std::pair<const Effect*, const Effect*> *, 8>
    EffectCoverageVector;

  /// \brief Makes effect summary minimal by removing covered effects.
  /// The caller is responsible for deallocating the EffectCoverageVector.
  void makeMinimal(EffectCoverageVector &ECV);

  /// \brief Prints effect summary to raw output stream.
  void print(raw_ostream &OS, char Separator='\n') const;

  /// \brief Returns a string with the effect summary.
  std::string toString() const;
}; // end class EffectSummary
} // End namespace asap.

} // End namespace clang.

#endif

