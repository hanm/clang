///-///////////////////////////////////////////////////////////////////////////
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
  typedef llvm::SmallVector<Rpl*, RPL_VECTOR_SIZE> RplVectorTy;

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

  /// Getters
  inline const RplElement* getLastElement() {
    return RplElements.back();
  }

  inline const RplElement* getFirstElement() {
    return RplElements.front();
  }

  /** returns the number of RPL elements */
  inline size_t length() {
    return RplElements.size();
  }
  /// Setters
  inline void appendElement(const RplElement* rplElm) {
    RplElements.push_back(rplElm);
    if (rplElm->isFullySpecified() == false)
      FullySpecified = false;
  }
  /// Predicates
  inline bool isFullySpecified() { return FullySpecified; }
  inline bool isEmpty() { return RplElements.empty(); }

  /// Nesting (Under)
  bool isUnder(const Rpl& rhsRpl) const {
    const CaptureRplElement *cap = dyn_cast<CaptureRplElement>(RplElements.front());
    if (cap) {
      cap->upperBound().isUnder(rhsRpl);
    }

    RplRef* lhs = new RplRef(*this);
    RplRef* rhs = new RplRef(rhsRpl);
    bool result = lhs->isIncludedIn(*rhs);
    delete lhs; delete rhs;
    return result;
  }
  /// Inclusion
  bool isIncludedIn(const Rpl& rhsRpl) const {
    const CaptureRplElement *cap = dyn_cast<CaptureRplElement>(RplElements.front());
    if (cap) {
      cap->upperBound().isIncludedIn(rhsRpl);
    }

    RplRef* lhs = new RplRef(*this);
    RplRef* rhs = new RplRef(rhsRpl);
    bool result = lhs->isIncludedIn(*rhs);
    delete lhs; delete rhs;
    OSv2 << "DEBUG:: ~~~~~ isIncludedIn[RPL](" << this->toString() << ", "
        << rhsRpl.toString() << ")=" << (result ? "true" : "false") << "\n";
    return result;
  }

  /// Substitution (Rpl)
  void substitute(const RplElement& From, const Rpl& To) {
    os << "DEBUG:: before substitution(" << From.getName() << "<-";
    To.print(os);
    os <<"): ";
    assert(RplElements.size()>0);
    print(os);
    os << "\n";
    /// A parameter is only allowed at the head of an Rpl
    RplElementVectorTy::iterator I = RplElements.begin();
    if (*(*I) == From) {
      OSv2 << "DEBUG:: found '" << From.getName()
        << "' replaced with '" ;
      To.print(OSv2);
      I = RplElements.erase(I);
      I = RplElements.insert(I, To.RplElements.begin(), To.RplElements.end());
      OSv2 << "' == '";
      print(OSv2);
      OSv2 << "'\n";
    }
    os << "DEBUG:: after substitution(" << From.getName() << "<-";
    To.print(os);
    os << "): ";
    print(os);
    os << "\n";
  }

  /// Append to this Rpl the argument Rpl without its head element
  inline void appendRplTail(Rpl* That) {
    if (That->length()>1)
      RplElements.append(That->length()-1, (*(That->RplElements.begin() + 1)));
  }

  Rpl *upperBound() {
    if (isEmpty() || !isa<CaptureRplElement>(RplElements.front()))
      return this;
    // else
    Rpl* upperBound = & dyn_cast<CaptureRplElement>(RplElements.front())->upperBound();
    upperBound->appendRplTail(this);
    return upperBound;
  }

  /// \brief join this to That
  Rpl *join(Rpl* That) {
    assert(That);
    Rpl Result;
    /// join from the left
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
      /// put a star in the middle and join from the right
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
    /// return
    this->RplElements = Result.RplElements;
    return this;
  }

  /// Capture
  // TODO: caller must deallocate Rpl and its element
  inline Rpl* capture() {
    if (this->isFullySpecified()) return this;
    else return new Rpl(*new CaptureRplElement(*this));
  }

}; // end class Rpl


class RplVector {
  friend class RplVector;
  private:
  /// Fields
  Rpl::RplVectorTy RplV;

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
    for (Rpl::RplVectorTy::const_iterator
            I = RV.RplV.begin(),
            E = RV.RplV.end();
         I != E; ++I) {
      RplV.push_back(new Rpl(*(*I))); // copy Rpls
    }
  }

  /// Destructor
  ~RplVector() {
    for (Rpl::RplVectorTy::const_iterator
            I = RplV.begin(),
            E = RplV.end();
         I != E; ++I) {
      delete (*I);
    }
  }

  /// Methods
  inline Rpl::RplVectorTy::iterator begin () { return RplV.begin(); }

  inline Rpl::RplVectorTy::iterator end () { return RplV.end(); }

  inline Rpl::RplVectorTy::const_iterator begin () const { return RplV.begin(); }

  inline Rpl::RplVectorTy::const_iterator end () const { return RplV.end(); }

  inline size_t size () const { return RplV.size(); }

  inline void push_back (const Rpl *R) {
    assert(R);
    RplV.push_back(new Rpl(*R));
  }

  inline void push_front (const Rpl *R) {
    assert(R);
    RplV.insert(RplV.begin(), new Rpl(*R));
  }

  Rpl *pop_front() {
    assert(RplV.size() > 0);
    Rpl *Result = RplV.front();
    RplV.erase(RplV.begin());
    return Result;
  }

  inline const Rpl *getRplAt(size_t idx) const {
    assert(idx>=0 && idx < RplV.size());
    return RplV[idx];
  }

  /// \brief joins this to That
  RplVector *join(RplVector *That) {
    assert(That);
    assert(That->size() == this->size());

    Rpl::RplVectorTy::iterator
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

  /// \brief return true when this <= That, false otherwise
  bool isIncludedIn (const RplVector &That) const {
    bool Result = true;
    assert(That.RplV.size() == this->RplV.size());

    Rpl::RplVectorTy::const_iterator
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

  /// substitution (RplVector)
  void substitute(const RplElement &FromEl, const Rpl &ToRpl) {
    for(Rpl::RplVectorTy::const_iterator
            I = RplV.begin(),
            E = RplV.end();
         I != E; ++I) {
      if (*I)
        (*I)->substitute(FromEl, ToRpl);
    }
  }

  Rpl *deref() {
    assert(RplV.size() > 0);
    Rpl *Result = RplV.front();
    RplV.erase(RplV.begin());
    return Result;
  }

  Rpl *deref(size_t DerefNum) {
    Rpl *Result = 0;
    assert(DerefNum >=0 && DerefNum < RplV.size());
    for (Rpl::RplVectorTy::iterator
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

  /// Print (RplVector)
  void print(raw_ostream &OS) const {
    Rpl::RplVectorTy::const_iterator
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

  inline std::string toString() const {
    std::string SBuf;
    llvm::raw_string_ostream OS(SBuf);
    print(OS);
    return std::string(OS.str());
  }

  /// \brief returns the union of two RPL Vectors by copying its inputs
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
    Rpl::RplVectorTy::const_iterator RHSI = RHS->begin(), RHSE = RHS->end();
    while (RHSI != RHSE) {
      LHS->push_back(*RHSI);
      ++RHSI;
    }
    return LHS;
  }

  /// \brief returns the union of two RPL Vectors but destroys its inputs
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
    Rpl::RplVectorTy::iterator RHSI = RHS->begin(), RHSE = RHS->end();
    while (RHSI != RHSE) {
      LHS->RplV.push_back(*RHSI);
      RHSI = RHS->RplV.erase(RHSI);
    }
    delete RHS;
    A = B = 0;
    return LHS;
  }
}; // end class RplVector

