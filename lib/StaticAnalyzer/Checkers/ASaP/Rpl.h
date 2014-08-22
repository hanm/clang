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

#include <SWI-Prolog.h>

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include "ASaPFwdDecl.h"
#include "ASaPUtil.h"
#include "OwningPtrSet.h"
#include "OwningVector.h"
#include "Substitution.h"

namespace clang {
namespace asap {

class RegionNameVector;

class RplDomain {
  const StringRef ID;          // unique Prolog Name
  RegionNameVector *Regions;     // owned
  const ParameterVector *Params; // not owned
  RplDomain *Parent;             // not owned
  bool Used;

public:
  RplDomain(StringRef ID,
            const RegionNameVector *RV,
            const ParameterVector *PV,
            RplDomain *Parent);
  RplDomain(const StringRef ID, const RplDomain &Dom);
  virtual ~RplDomain();

  const StringRef getID() { return ID; }
  void addRegion(const NamedRplElement &R);
  bool isUsed() const { return Used; }
  void markUsed();
  void print (llvm::raw_ostream &OS) const;
  term_t getPLTerm() const;
  void assertzProlog() const;
};

class RplElement {
public:
  /// Discriminator for LLVM-style RTTI (dyn_cast<> et al.)
  enum RplElementKind {
      RK_Special,
      RK_Star,
      RK_Named,
      RK_Parameter,
      RK_Capture
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

  void print (llvm::raw_ostream& OS) const {
    OS << getName();
  }

  virtual bool isFullySpecified() const { return true; }

  virtual StringRef getName() const = 0;
  virtual bool operator == (const RplElement &That) const {
    return (this == &That) ? true : false;
  }
  virtual term_t getPLTerm() const = 0;

  virtual ~RplElement() {}
}; // end class RplElement

class SpecialRplElement : public RplElement {
public:
  enum SpecialRplElementKind {
      SRK_Root,
      SRK_Global,
      SRK_Local,
      SRK_Immutable
  };
private:
  /// Fields
  SpecialRplElementKind Kind;

public:
  /// Constructor
  SpecialRplElement(SpecialRplElementKind Kind) : RplElement(RK_Special), Kind(Kind) {}
  virtual ~SpecialRplElement() {}

  /// Methods
  virtual StringRef getName() const {
    switch(Kind) {
    case SRK_Root: return "Root";
    case SRK_Global: return "Global";
    case SRK_Local: return "Local";
    case SRK_Immutable: return "Immutable";
    }
  }

  virtual term_t getPLTerm() const {
    term_t Result = PL_new_term_ref();
    switch(Kind) {
    case SRK_Root: PL_put_atom_chars(Result,"rROOT"); break;
    case SRK_Global: PL_put_atom_chars(Result,"rGLOBAL"); break;
    case SRK_Local: PL_put_atom_chars(Result,"rLOCAL"); break;
    case SRK_Immutable: PL_put_atom_chars(Result,"rIMMUTABLE"); break;
    }
    return Result;
  }

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
  virtual term_t getPLTerm() const {
    term_t Result = PL_new_term_ref();
    PL_put_atom_chars(Result,"rSTAR");
    return Result;
  }

  static bool classof(const RplElement *R) {
    return R->getKind() == RK_Star;
  }
}; // end class StarRplElement

///-////////////////////////////////////////
class NamedRplElement : public RplElement {
  /// Fields
  const StringRef Name;
  const StringRef PrologName;

public:
  /// Constructor
  NamedRplElement(StringRef Name, StringRef PrologName)
                 : RplElement(RK_Named), Name(Name), PrologName(PrologName) {}
  virtual ~NamedRplElement() {}
  /// Methods
  virtual StringRef getName() const { return Name; }
  virtual term_t getPLTerm() const {
    term_t Result = PL_new_term_ref();
    PL_put_atom_chars(Result, PrologName.data());
    return Result;
  }

  static bool classof(const RplElement *R) {
    return R->getKind() == RK_Named;
  }

};

///-////////////////////////////////////////
class ParamRplElement : public RplElement {
  ///Fields
  const StringRef Name;
  const StringRef PrologName;

public:
  /// Constructor
  ParamRplElement(StringRef Name, StringRef PrologName)
                 : RplElement(RK_Parameter), Name(Name), PrologName(PrologName) {}
  virtual ~ParamRplElement() {}

  /// Methods
  virtual StringRef getName() const { return Name; }
  virtual term_t getPLTerm() const {
    term_t Result = PL_new_term_ref();
    PL_put_atom_chars(Result, PrologName.data());
    return Result;
  }

  static bool classof(const RplElement *R) {
    return R->getKind() == RK_Parameter;
  }
};

///-////////////////////////////////////////

class Rpl {
public:

  enum RplKind {
    RPLK_Concrete,
    RPLK_Var
  };

private:
  const RplKind Kind;
  Trivalent FullySpecified;
  SubstitutionVector SubV;

public:
  Rpl(RplKind K, Trivalent FullySpecified)
     : Kind(K), FullySpecified(FullySpecified) {}
  Rpl(const Rpl &That);
  virtual ~Rpl() {}
  virtual Rpl *clone() const = 0;


  RplKind getKind() const { return Kind; }
  Trivalent isFullySpecified() const { return FullySpecified; }
  void setFullySpecified(Trivalent V) { FullySpecified = V; }

  bool operator != (const RplElement &That) const {
    return !(*this==That);
  }

  void addSubstitution(const Substitution &S);
  void addSubstitution(const SubstitutionSet &SubS);

  term_t getSubVPLTerm() const;
  bool hasSubs() const { return (SubV.size() > 0); }

  // Nesting (Under)
  /// \brief Returns true iff this is under That
  virtual Trivalent isUnder(const Rpl &That) const = 0;

  // Inclusion
  /// \brief Returns true iff this RPL is included in That
  virtual Trivalent isIncludedIn(const Rpl &That,
                                 bool GenConstraint = false) const = 0;

  /// \brief Returns true iff this RPL is disjoint from That
  virtual Trivalent isDisjoint(const Rpl &That) const = 0;

  std::string toString() const;
  virtual void print(llvm::raw_ostream &OS) const;
  virtual void printSolution(llvm::raw_ostream &OS) const;
  virtual void join(Rpl *That) = 0;
  virtual Trivalent substitute(const Substitution *S) = 0;
  virtual void substitute(const SubstitutionSet *SubS) = 0;
  virtual bool operator == (const RplElement &That) const = 0;
  virtual term_t getPLTerm() const = 0;
  virtual term_t getRplElementsPLTerm() const = 0;

  /// Static Constants
  static const char RPL_SPLIT_CHARACTER = ':';
  static const StringRef RPL_LIST_SEPARATOR;
  static const StringRef RPL_NAME_SPEC;

  /// Static Functions
  /// \brief Return true when input is a valid region name or param declaration
  static bool isValidRegionName(const StringRef& s);

  /// \brief Returns two StringRefs delimited by the first occurrence
  /// of the RPL_SPLIT_CHARACTER.
  static std::pair<StringRef, StringRef> splitRpl(StringRef &String);

}; // end class Rpl

class ConcreteRpl : public Rpl {

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

  /// RplRef class
  // We use the RplRef class, friends with Rpl to efficiently perform
  // isIncluded and isUnder tests
  class RplRef {
    long FirstIdx;
    long LastIdx;
    const ConcreteRpl &Rpl;

  public:
    /// Constructor
    RplRef(const ConcreteRpl &R);

    /// Printing (Rpl Ref)
    void print(llvm::raw_ostream &OS) const;
    std::string toString() const;

    /// Getters
    inline const RplElement *getFirstElement() const {
      return Rpl.RplElements[FirstIdx];
    }
    inline const RplElement *getLastElement() const {
      return Rpl.RplElements[LastIdx];
    }

    inline RplRef &stripLast() {
      LastIdx--;
      return *this;
    }

    inline RplRef &stripFirst() {
      FirstIdx++;
      return *this;
    }

    inline bool isEmpty() {
      return (LastIdx < FirstIdx) ? true : false;
    }

    /// \brief Under: true  iff this <= RHS
    bool isUnder(RplRef &RHS);

    /// \brief Inclusion: true  iff  this c= rhs
    bool isIncludedIn(RplRef &RHS);

    bool isDisjointLeft(RplRef &That);
    bool isDisjointRight(RplRef &That);

  }; /// end class RplRef
  ///////////////////////////////////////////////////////////////////////////
  public:
  /// Constructors
  ConcreteRpl() :  Rpl(RPLK_Concrete, RK_TRUE) {}

  ConcreteRpl(const RplElement &Elm)
             : Rpl(RPLK_Concrete, boolToTrivalent(Elm.isFullySpecified())) {
    RplElements.push_back(&Elm);
  }

  /// Copy Constructor
  ConcreteRpl(const ConcreteRpl &That);

  /// \brief Print the Rpl to an output stream.
  virtual void print(llvm::raw_ostream &OS) const;

  /// \brief Return a Prolog term for the Rpl.
  virtual term_t getPLTerm() const;
  virtual term_t getRplElementsPLTerm() const;

  // Getters
  /// \brief Returns the last of the RPL elements of this RPL.
  inline const RplElement *getLastElement() const {
    return RplElements.back();
  }
  /// \brief Returns the first of the RPL elements of this RPL.
  inline const RplElement *getFirstElement() const {
    return RplElements.front();
  }

  /// \brief Returns the number of RPL elements.
  inline size_t length() const {
    return RplElements.size();
  }
  // Setters
  /// \brief Appends an RPL element to this RPL.
  inline void appendElement(const RplElement *RplElm) {
    if (RplElm) {
      RplElements.push_back(RplElm);
      if (!RplElm->isFullySpecified()) {
        setFullySpecified(RK_FALSE);
      }
    }
  }
  // Predicates
  //inline bool isFullySpecified() { return FullySpecified; }
  inline bool isEmpty() { return RplElements.empty(); }
  bool isPrivate() const;

  virtual Rpl *clone() const {
    return new ConcreteRpl(*this);
  }
  // Nesting (Under)
  /// \brief Returns true iff this is under That
  virtual Trivalent isUnder(const Rpl &That) const;

  // Inclusion
  /// \brief Returns true iff this RPL is included in That
  virtual Trivalent isIncludedIn(const Rpl &That,
                                 bool GenConstraint = false) const;

  /// \brief Returns true iff this RPL is disjoint from That
  virtual Trivalent isDisjoint(const Rpl &That) const;

  // Substitution (Rpl)
  /// \brief this[FromEl <- ToRpl]
  virtual Trivalent substitute(const Substitution *S);
  virtual void substitute(const SubstitutionSet *SubS);

  /// \brief Append to this RPL the argument Rpl but without its head element.
  void appendRplTail(ConcreteRpl *That);

  /// \brief Return the upper bound of an RPL (different when RPL is captured).
  //ConcreteRpl *upperBound();

  /// \brief Join this to That (by modifying this).
  virtual void join(Rpl *That);

  virtual bool operator == (const RplElement &That) const {
    if (RplElements.size() == 1 &&
        *RplElements[0] == That)
      return true;
    else
      return false;
  }

  // Capture
  // TODO: caller must deallocate Rpl and its element
  //Rpl *capture();

  static bool classof(const Rpl *R) {
    return R->getKind() == RPLK_Concrete;
  }
}; // end class ConcreteRpl


class VarRpl : public Rpl {
private:
  StringRef Name;
  RplDomain *Domain;

public:
  VarRpl(StringRef ID, RplDomain *Dom)
        : Rpl(RPLK_Var, RK_DUNNO), Name(ID), Domain(Dom) {
    assert(Dom && "Internal Error: making VarRpl without an RplDomain");
    Dom->markUsed();
  }

  VarRpl(const VarRpl &That)
        : Rpl(That), Name(That.Name), Domain(That.Domain) {}

  virtual Rpl *clone() const {
    return new VarRpl(*this);
  }

  //inline void setDomain(RplDomain *D) { Domain = D;}
  inline const RplDomain *getDomain() { return Domain;}

  /// \brief Print the Rpl to an output stream.
  virtual void print(llvm::raw_ostream &OS) const;

  virtual void printSolution(llvm::raw_ostream &OS) const;

  // Nesting (Under)
  /// \brief Returns true iff this is under That
  virtual Trivalent isUnder(const Rpl &That) const;

  // Inclusion
  /// \brief Returns true iff this RPL is included in That
  virtual Trivalent isIncludedIn(const Rpl &That, bool GenConstraints = false) const;

  /// \brief Returns true iff this RPL is disjoint from That
  virtual Trivalent isDisjoint(const Rpl &That) const;

  virtual void join(Rpl *That);
  virtual Trivalent substitute(const Substitution *S);
  virtual void substitute(const SubstitutionSet *SubS);

  /// \brief builds a rpl([ID], Subs) Prolog term
  virtual term_t getPLTerm() const;
  /// \brief build a [ID] Prolog list term
  virtual term_t getRplElementsPLTerm() const;
  /// \brief builds a atom out of the ID (Name)
  term_t getIDPLTerm() const;
  /// \brief builds and assertz a head_rpl_var(ID, Domain) Prolog predicate
  void assertzProlog() const;

  /// \brief Query Prolog to retrieve the infered value for the VarRpl.
  char *readPLValue() const;

  virtual bool operator == (const RplElement &That) const {
      return false;
  }

  static bool classof(const Rpl *R) {
    return R->getKind() == RPLK_Var;
  }
}; // end class VarRpl
//////////////////////////////////////////////////////////////////////////

/*class CaptureRplElement : public RplElement {
  /// Fields
  Rpl &includedIn;

public:
  /// Constructor
  CaptureRplElement(Rpl& includedIn) : RplElement(RK_Capture),
                                       includedIn(includedIn)
  { //assert(includedIn.isFullySpecified() == false);
  }
  virtual ~CaptureRplElement() {}
  /// Methods
  virtual StringRef getName() const { return "rho"; }

  virtual bool isFullySpecified() const { return false; }

  ConcreteRpl& upperBound() const { return includedIn; }

  virtual term_t getPLTerm() const {
    term_t Result = PL_new_term_ref();
    // TODO FIXME: implement this
    return Result;
  }

  static bool classof(const RplElement *R) {
    return R->getKind() == RK_Capture;
  }
}; // end class CaptureRplElement
*/

//////////////////////////////////////////////////////////////////////////
#ifndef PARAM_VECTOR_SIZE
  #define PARAM_VECTOR_SIZE 8
#endif

class ParameterSet:
  public llvm::SmallPtrSet<const ParamRplElement*, PARAM_VECTOR_SIZE> {
public:
  //typedef llvm::SmallPtrSet<const ParamRplElement*, PARAM_VECTOR_SIZE> BaseClass;

  bool hasElement(const RplElement *Elmt) const;
  void print(llvm::raw_ostream &OS) const;
  std::string toString() const;

}; // end class ParameterSet

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

  void addToParamSet(ParameterSet *PSet) const;

  /// \brief Returns the Parameter at position Idx
  inline const ParamRplElement *getParamAt(size_t Idx) const {
    assert(Idx < size());
    return (*this)[Idx];
  }

  /// \brief Returns the ParamRplElement with name=Name or null.
  const ParamRplElement *lookup(StringRef Name) const;

  /// \brief Returns true if the argument RplElement is contained.
  bool hasElement(const RplElement *Elmt) const;

  void take(ParameterVector *&PV);

  void assertzProlog () const;

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
  RplVector(const Rpl &R) {
    push_back(R);
  }

  RplVector(const RplVector &From) : OwningVector() {
    for (typename VectorT::const_iterator
            I = From.VectorT::begin(),
            E = From.VectorT::end();
         I != E; ++I) {
      push_back(*I); // push_back makes a copy of *I
    }
  }


  bool push_back (const Rpl *E) {
    if (E) {
      VectorT::push_back(E->clone());
      return true;
    } else {
      return false;
    }
  }

  void push_back (const Rpl &E) {
    VectorT::push_back(E.clone());
  }

  RplVector(const ParameterVector &ParamVec);

  /// \brief Add the argument RPL to the front of the RPL vector.
  inline void push_front (const Rpl *R) {
    assert(R);
    VectorT::insert(begin(), R->clone());
  }
  /// \brief Remove and return the first RPL in the vector.
  inline std::unique_ptr<Rpl> deref() { return pop_front(); }

  /// \brief Same as performing deref() DerefNum times.
  std::unique_ptr<Rpl> deref(size_t DerefNum);

  /// \brief Return a pointer to the RPL at position Idx in the vector.
  inline const Rpl *getRplAt(size_t Idx) const {
    assert(Idx < size() && "attempted to access beyond last RPL element");
    return (*this)[Idx];
  }

  /// \brief Joins this to That.
  void join(RplVector *That);

  /// \brief Return true when this is included in That, false otherwise.
  Trivalent isIncludedIn (const RplVector &That, bool GenConstraints = false) const;

  /// \brief Substitution this[FromEl <- ToRpl] (over RPL vector)
  void substitute(const Substitution *S);
  void substitute(const SubstitutionSet *SubS);
  /// \brief Retrun true if at least one of the Rpl is a VarRpl
  bool hasRplVar() const;
  void printSolution(raw_ostream &OS) const;
  /// \brief Returns the union of two RPL Vectors by copying its inputs.
  static RplVector *merge(const RplVector *A, const RplVector *B);
  /// \brief Returns the union of two RPL Vectors but destroys its inputs.
  static RplVector *destructiveMerge(RplVector *&A, RplVector *&B);
}; // End class RplVector.

//////////////////////////////////////////////////////////////////////////
#ifndef REGION_NAME_SET_SIZE
  #define REGION_NAME_SET_SIZE 8
#endif
class RegionNameSet
: public OwningPtrSet<NamedRplElement, REGION_NAME_SET_SIZE> {

public:
  /// \brief Returns the NamedRplElement with name=Name or null.
  const NamedRplElement *lookup (StringRef Name);
  void assertzProlog() const;
}; // End class RegionNameSet.



class RegionNameVector
: public OwningVector<NamedRplElement, REGION_NAME_SET_SIZE> {

public:
  /// \brief Returns the NamedRplElement with name=Name or null.
  const NamedRplElement *lookup (StringRef Name) {
    for (VectorT::iterator I = begin(), E = end();
         I != E; ++I) {
      const NamedRplElement *El = *I;
      if (El->getName().compare(Name) == 0)
        return El;
    }
    return 0;
  }

  void print (llvm::raw_ostream& OS) {
    //    OS << "Printing Regions vector...\n";
    VectorT::iterator I = begin(), E = end();
    if (I != E) {
      const NamedRplElement *El = *I;
      OS << El->getName();
      ++I;
    }
    for(; I != E; ++I) {
       const NamedRplElement *El = *I;
       OS << ", " << El->getName();
     }
  }
}; // End class RegionNameVector.

} // End namespace asap.
} // End namespace clang.

#endif



