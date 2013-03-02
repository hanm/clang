//=== Rpl.cpp - Safe Parallelism checker -----*- C++ -*--------------===//
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

///-//////////////////////////////////////////////////////////////////////
/// Rpl Class
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
    void print(raw_ostream& OS) const {
      int I = firstIdx;
      for (; I < lastIdx; ++I) {
        OS << rpl.RplElements[I]->getName() << RPL_SPLIT_CHARACTER;
      }
      // print last element
      if (I==lastIdx)
        OS << rpl.RplElements[I]->getName();
    }

    inline std::string toString() {
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

  /// Destructors
  //~Rpl() {}

  /// Static
  static std::pair<StringRef, StringRef> splitRpl(StringRef &String) {
    size_t Idx = 0;
    do {
      Idx = String.find(RPL_SPLIT_CHARACTER, Idx);
      OSv2 << "Idx = " << Idx << ", size = " << String.size() << "\n";
      if (Idx == StringRef::npos)
        break;

    } while (Idx < String.size() - 2
             && String[Idx+1] == RPL_SPLIT_CHARACTER && Idx++ && Idx++);

    if (Idx == StringRef::npos)
      return std::pair<StringRef, StringRef>(String, "");
    else
      return std::pair<StringRef, StringRef>(String.slice(0,Idx),
                                              String.slice(Idx+1, String.size()));
  }

  /// Printing (Rpl)
  void print(raw_ostream &OS) const {
    RplElementVectorTy::const_iterator I = RplElements.begin();
    RplElementVectorTy::const_iterator E = RplElements.end();
    for (; I < E-1; I++) {
      OS << (*I)->getName() << Rpl::RPL_SPLIT_CHARACTER;
    }
    // print last element
    if (I < E)
      OS << (*I)->getName();
  }

  inline std::string toString() const {
    std::string SBuf;
    llvm::raw_string_ostream OS(SBuf);
    print(OS);
    return std::string(OS.str());
  }

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
  bool isUnder(const Rpl& That) const {
    const CaptureRplElement *cap = dyn_cast<CaptureRplElement>(RplElements.front());
    if (cap) {
      cap->upperBound().isUnder(That);
    }

    RplRef* LHS = new RplRef(*this);
    RplRef* RHS = new RplRef(That);
    bool Result = LHS->isIncludedIn(*RHS);
    delete LHS; delete RHS;
    return Result;
  }
  // Inclusion
  /// \brief Returns true iff this RPL is included in That
  bool isIncludedIn(const Rpl& That) const {
    const CaptureRplElement *cap = dyn_cast<CaptureRplElement>(RplElements.front());
    if (cap) {
      cap->upperBound().isIncludedIn(That);
    }

    RplRef* LHS = new RplRef(*this);
    RplRef* RHS = new RplRef(That);
    bool Result = LHS->isIncludedIn(*RHS);
    delete LHS; delete RHS;
    OSv2 << "DEBUG:: ~~~~~ isIncludedIn[RPL](" << this->toString() << ", "
        << That.toString() << ")=" << (Result ? "true" : "false") << "\n";
    return Result;
  }

  // Substitution (Rpl)
  /// \brief this[FromEl <- ToRpl]
  void substitute(const RplElement& FromEl, const Rpl& ToRpl) {
    os << "DEBUG:: before substitution(" << FromEl.getName() << "<-";
    ToRpl.print(os);
    os <<"): ";
    assert(RplElements.size()>0);
    print(os);
    os << "\n";
    /// A parameter is only allowed at the head of an Rpl
    RplElementVectorTy::iterator I = RplElements.begin();
    if (*(*I) == FromEl) {
      OSv2 << "DEBUG:: found '" << FromEl.getName()
        << "' replaced with '" ;
      ToRpl.print(OSv2);
      I = RplElements.erase(I);
      I = RplElements.insert(I, ToRpl.RplElements.begin(),
                                ToRpl.RplElements.end());
      OSv2 << "' == '";
      print(OSv2);
      OSv2 << "'\n";
    }
    os << "DEBUG:: after substitution(" << FromEl.getName() << "<-";
    ToRpl.print(os);
    os << "): ";
    print(os);
    os << "\n";
  }

  /// \brief Append to this RPL the argument Rpl but without its head element.
  inline void appendRplTail(Rpl* That) {
    if (!That)
      return;
    if (That->length()>1)
      RplElements.append(That->length()-1, (*(That->RplElements.begin() + 1)));
  }
  /// \brief Return the upper bound of an RPL (different when RPL is captured).
  Rpl *upperBound() {
    if (isEmpty() || !isa<CaptureRplElement>(RplElements.front()))
      return this;
    // else
    Rpl* upperBound = & dyn_cast<CaptureRplElement>(RplElements.front())->upperBound();
    upperBound->appendRplTail(this);
    return upperBound;
  }

  /// \brief Join this to That.
  Rpl *join(Rpl* That) {
    assert(That);
    Rpl Result;
    // join from the left
    RplElementVectorTy::const_iterator
            ThisI = this->RplElements.begin(),
            ThatI = That->RplElements.begin(),
            ThisE = this->RplElements.end(),
            ThatE = That->RplElements.end();
    for ( ; ThisI != ThisE && ThatI != ThatE && (*ThisI == *ThatI);
            ++ThisI, ++ThatI) {
      Result.appendElement(*ThisI);
    }
    if (ThisI != ThisE) {
      // put a star in the middle and join from the right
      assert(ThatI != ThatE);
      Result.appendElement(STARRplElmt);
      Result.FullySpecified = false;
      RplElementVectorTy::const_reverse_iterator
          ThisI = this->RplElements.rbegin(),
          ThatI = That->RplElements.rbegin(),
          ThisE = this->RplElements.rend(),
          ThatE = That->RplElements.rend();
      int ElNum = 0;
      for(; ThisI != ThisE && ThatI != ThatE && (*ThisI == *ThatI);
          ++ThisI, ++ThatI, ++ElNum);
      if (ElNum > 0) {
        Result.RplElements.append(ElNum,
                      *(RplElements.begin() + RplElements.size() - ElNum));
      }
    }
    // return
    this->RplElements = Result.RplElements;
    return this;
  }

  // Capture
  // TODO: caller must deallocate Rpl and its element
  inline Rpl* capture() {
    if (this->isFullySpecified()) return this;
    else return new Rpl(*new CaptureRplElement(*this));
  }

}; // end class Rpl

//////////////////////////////////////////////////////////////////////////
// Rpl Vector
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
    assert(Idx < RplV.size());
    return RplV[Idx];
  }

  /// \brief Joins this to That.
  RplVector *join(RplVector *That) {
    assert(That);
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
      Rpl *Join = LHS->join(RHS);
      ThisI = this->RplV.erase(ThisI);
      //this->RplV.assign(); // can we use assign instead of erase and insert?
      ThisI = this->RplV.insert(ThisI, Join);
    }
    return this;
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
    OSv2 << "DEBUG:: [" << this->toString() << "] is " << (Result?"":"not")
        << " included in [" << That.toString() << "]\n";
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
  void print(raw_ostream &OS) const {
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
}; // end class RplVector

