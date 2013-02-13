///-///////////////////////////////////////////////////////////////////////////
/// ASapType Class

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
  /// Methods
  inline int getArgVSize() { return ArgV->size(); }

  inline const Rpl *getInRpl() { return InRpl; }

  const Rpl *getInRpl(int DerefNum) {
    assert(DerefNum >= -1);
    if (DerefNum == -1) return 0;
    if (DerefNum == 0) return InRpl;
    return this->ArgV->getRplAt(DerefNum-1);
  }

  const Rpl *getSubstArg(int DerefNum) {
    assert(DerefNum >= -1);
    if (DerefNum == -1) return InRpl;
    if (InRpl)
      return this->ArgV->getRplAt(DerefNum-1);
    else
      return this->ArgV->getRplAt(DerefNum);
  }

  inline QualType getQT() { return QT; };

  QualType getQT(int DerefNum) {
    assert(DerefNum >= 0);
    QualType Result = QT;
    while (DerefNum > 0) {
      assert(Result->isPointerType());
      Result = Result->getPointeeType();
      --DerefNum;
    }
    return Result;
  }

  /// \brief if this is a function type, return its return type;
  /// else return null
  ASaPType *getReturnType() {
    if (QT->isFunctionType()) {
      const FunctionType *FT = dyn_cast<FunctionType>(QT.getTypePtr());
      QualType ResultQT = FT->getResultType();
      return new ASaPType(ResultQT, ArgV, InRpl);
    } else
      return 0;
  }

  /// \brief dereference this type DerefNum times
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
  inline bool subtype(const ASaPType &That) { return *this <= That; }

  /// \brief true when 'this' is a subtype (derived type) of 'that'
  bool operator <= (const ASaPType &That) {
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

  /// \brief joins this to That.
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

  /// Substitution (ASaPType)
  void substitute(const RplElement &FromEl, const Rpl &ToRpl) {
    if (InRpl)
      InRpl->substitute(FromEl, ToRpl);
    if (ArgV)
      ArgV->substitute(FromEl, ToRpl);
  }

  private:
  /// Private Methods

}; // end class ASaPType

