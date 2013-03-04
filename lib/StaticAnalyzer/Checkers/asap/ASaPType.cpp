//=== ASaPType.cpp - Safe Parallelism checker -----*- C++ -*---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This files defines the ASaPType class used by the Safe Parallelism
// checker, which tries to prove the safety of parallelism given region
// and effect annotations.
//
//===----------------------------------------------------------------===//

///-///////////////////////////////////////////////////////////////////////////
/// ASapType Class
//namespace ASaP {
class ASaPType {
  friend class ASaPType;
  private:
  /// Fields
  QualType QT;
  RplVector *ArgV;
  Rpl *InRpl; // can be null

  public:
  /// Constructors
  ASaPType (QualType QT, RplVector *ArgV, Rpl *InRpl = 0, bool Simple=false)
           : QT(QT) {
    // 1. Set InRpl & ArgV
    if (InRpl)
      this->InRpl = new Rpl(*InRpl);
    else
      this->InRpl = 0;
    if (ArgV)
      this->ArgV = new RplVector(*ArgV);
    else
      this->ArgV = 0;
    if (!Simple) {
      // 2. If QT is a function type, we're really interested in the
      // return type
      if (QT->isFunctionType()) {
        const FunctionType *FT = dyn_cast<FunctionType>(QT.getTypePtr());
        QT = FT->getResultType();
      }
      // 3. Check if we might need to set InRpl
      if (!this->InRpl) {
        // Figure out based on QT if we need an InRpl
        if (QT->isScalarType() && !QT->isReferenceType()) {
          // get it from the head of ArgV
          assert(this->ArgV && this->ArgV->size() > 0);
          this->InRpl = this->ArgV->pop_front();
        }
      }
    } // end if (!Simple)
  }
  // copy constructor: deep copy
  ASaPType (const ASaPType &T) : QT(T.QT) {
    this->QT = T.QT;
    if (T.InRpl)
      this->InRpl = new Rpl(*T.InRpl);
    else
      this->InRpl = 0;
    if (T.ArgV)
      this->ArgV = new RplVector(*T.ArgV);
    else
      this->ArgV = 0;
  }

  ~ASaPType() {
    delete InRpl;
    delete ArgV;
  }
  // Methods
  /// \brief Returns true iff this is of FunctionType.
  inline bool isFunctionType() const { return QT->isFunctionType(); }
  /// \brief Return the size of the ArgsVector.
  inline int getArgVSize() const { return ArgV->size(); }
  /// \brief Return the In RPL which can be null.
  inline const Rpl *getInRpl() const { return InRpl; }
  /// \brief Return the In RPL of this after DerefNum dereferences.
  const Rpl *getInRpl(int DerefNum) const {
    assert(DerefNum >= -1);
    if (DerefNum == -1) return 0;
    if (DerefNum == 0) return InRpl;
    return this->ArgV->getRplAt(DerefNum-1);
  }

  /// \brief Return the Argument for substitution after DerefNum dereferences.
  /// FIXME: support multiple region parameters per class type.
  const Rpl *getSubstArg(int DerefNum = 0) const {
    assert(DerefNum >= -1);
    if (DerefNum == -1) return InRpl;
    if (InRpl)
      return this->ArgV->getRplAt(DerefNum-1);
    else
      return this->ArgV->getRplAt(DerefNum);
  }
  /// \brief Return the QualType of this ASapType
  inline QualType getQT() const { return QT; }

  /// \brief Return the QualType of this after DerefNum dereferences.
  QualType getQT(int DerefNum) const {
    assert(DerefNum >= 0);
    QualType Result = QT;
    while (DerefNum > 0) {
      assert(Result->isPointerType());
      Result = Result->getPointeeType();
      --DerefNum;
    }
    return Result;
  }
  /// \brief If this is a function type, return its return type; else null.
  ASaPType *getReturnType() const {
    if (QT->isFunctionType()) {
      const FunctionType *FT = dyn_cast<FunctionType>(QT.getTypePtr());
      QualType ResultQT = FT->getResultType();
      return new ASaPType(ResultQT, ArgV, InRpl);
    } else
      return 0;
  }

  /// \brief Dereferences this type DerefNum times.
  void deref(int DerefNum) {
    assert(DerefNum >= 0);
    if (DerefNum == 0)
      return;
    assert(ArgV);
    while (DerefNum > 0) {
      if (InRpl)
        delete InRpl;
      InRpl = ArgV->deref();
      assert(QT->isPointerType());
      QT = QT->getPointeeType();
      DerefNum--;
    }
  }
  /// \brief Modifies this type as if its address were taken.
  void addrOf(QualType RefQT) {
    assert(RefQT->isPointerType());
    QualType TestQT = RefQT->getPointeeType();
    assert(this->QT == TestQT);
    this->QT = RefQT;

    if (InRpl) {
      ArgV->push_front(InRpl);
      InRpl = 0;
    }
  }

  /// \brief Set the InAnnotation to NULL (and free the Rpl)
  void dropInRpl() {
    delete InRpl;
    InRpl = 0;
  }
  /// \brief Returns a string describing this ASaPType
  std::string toString(ASTContext &Ctx) const {
    std::string SBuf;
    llvm::raw_string_ostream OS(SBuf);
    QT.print(OS, Ctx.getPrintingPolicy());
    OS << ", ";
    if (InRpl)
      OS << "IN:" << InRpl->toString();
    else
      OS << "IN:<empty>";

    OS << ", ArgV:" << ArgV->toString();
    return std::string(OS.str());
  }

  /// \brief  true when 'this' is a subtype (derived type) of 'that'
  inline bool subtype(const ASaPType &That) const { return *this <= That; }

  /// \brief true when 'this' is a subtype (derived type) of 'that'
  bool operator <= (const ASaPType &That) const {
    if (this->QT!=That.QT) {
      /// Typechecking has passed so we assume that this->QT <= that->QT
      /// but we have to find follow the mapping and substitute Rpls....
      /// TODO :)
      OSv2 << "DEBUG:: Failing ASaP::subtype because QT != QT'\n";
      return false; // until we support inheritance this is good enough
    }
    assert(this->QT == That.QT);
    /// Note that we're ignoring InRpl
    assert(That.ArgV);
    return this->ArgV->isIncludedIn(*That.ArgV);
  }

  /// \brief Joins this to That.
  /// Join returns the smallest common supertype (Base Type)
  ASaPType *join(ASaPType *That) {
    if (!That) return this;
    if (this->QT!=That->QT) {
      /// Typechecking has passed so we assume that this->QT <= that->QT
      /// but we have to find follow the mapping and substitute Rpls....
      /// TODO :)
      assert(false); // ...just fail
      return 0; // until we support inheritance this is good enough
    }
    assert(this->QT == That->QT);
    this->InRpl->join(That->InRpl);
    this->ArgV->join(That->ArgV);
    return this;
  }

  // Substitution (ASaPType)
  void substitute(const SubstitutionVector &SubV) {
    for(SubstitutionVector::SubstitutionVecT::const_iterator
          I = SubV.begin(),
          E = SubV.end();
        I != E; ++I) {
      Substitution &S = *(*I);
      substitute(S);
    }
  }

  /// \brief Performs substitution on type: this[FromEl <- ToRpl]
  void substitute(Substitution &S) {
    const RplElement *FromEl = S.getFrom();
    const Rpl *ToRpl = S.getTo();
    assert(FromEl);
    assert(ToRpl);
    if (InRpl)
      InRpl->substitute(*FromEl, *ToRpl);
    if (ArgV)
      ArgV->substitute(*FromEl, *ToRpl);
  }



  private:
  // Private Methods

}; // end class ASaPType
//} // end namespace ASaP
