/**
 *  Traversal that checks semantic properties of the ASaP Annotations
 *  1. Region and parameter declarations are well formed
 *  2. Rpls are valid
 *  2.a Rpl Elements are declared
 *  2.b Parameters only appear at the first position
 *  3. Correct number of region argument
 *  4. Declaration has too many region arguments
 *  5. Declaration has too few region arguments (ignored when default
 *     arguments are enabled).
 *  6. Check that effect summaries are minimal
 *  7. Build map from FunctionDecl to effect summaries
 */
class ASaPSemanticCheckerTraverser :
  public RecursiveASTVisitor<ASaPSemanticCheckerTraverser> {

private:
  /// Fields
  ento::BugReporter &BR;
  ASTContext &Ctx;
  AnalysisDeclContext *AC;
  raw_ostream &OS;

  RplElementAttrMapTy &RplElementMap;
  RplAttrMapTy &RplAttrMap;
  ASaPTypeDeclMapTy &ASaPTypeDeclMap;
  EffectSummaryMapTy &EffectSummaryMap;

  bool FatalError;

  /// Private Methods
  void addASaPTypeToMap(ASaPTypeDeclMapTy &Map, ValueDecl* D,
                        RplVector *RV, Rpl *InRpl) {
    assert(!Map[D]);
    ASaPType *T = new ASaPType(D->getType(), RV, InRpl);
    OS << "Debug:: RV.size=" << (RV ? RV->size() : 0)
       << ", T.RV.size=" << T->getArgVSize() << "\n";
    Map[D] = T;
    OS << "Debug :: adding type: " << T->toString(Ctx) << "\n";
  }

  /// \brief Issues Warning: '<str>' <bugName> on Declaration
  void helperEmitDeclarationWarning(const Decl *D,
                                    const StringRef &str,
                                    std::string bugName) {

    std::string description_std = "'";
    description_std.append(str);
    description_std.append("' ");
    description_std.append(bugName);
    StringRef bugCategory = "Safe Parallelism";
    StringRef bugStr = description_std;

    PathDiagnosticLocation VDLoc(D->getLocation(), BR.getSourceManager());
    BR.EmitBasicReport(D, bugName, bugCategory,
                       bugStr, VDLoc, D->getSourceRange());

  }

  /// \brief Issues Warning: '<str>' <bugName> on Attribute
  void helperEmitAttributeWarning(const Decl *D,
                                  const Attr *attr,
                                  const StringRef &str,
                                  std::string bugName) {

    std::string description_std = "'";
    description_std.append(str);
    description_std.append("' ");
    description_std.append(bugName);
    StringRef bugCategory = "Safe Parallelism";
    StringRef bugStr = description_std;

    PathDiagnosticLocation VDLoc(attr->getLocation(), BR.getSourceManager());
    BR.EmitBasicReport(D, bugName, bugCategory,
                       bugStr, VDLoc, attr->getRange());
  }

  /// \brief Emit error for misplaced region parameter within RPL.
  void emitMisplacedRegionParameter(const Decl *D,
                                    const Attr* A,
                                    const StringRef &S) {
    StringRef BugName = "Misplaced Region Parameter: Region parameters "
                        "may only appear at the head of an RPL.";
    helperEmitAttributeWarning(D, A, S, BugName);
  }

  /// \brief Emit a Warning when the input string (which is assumed to be an
  /// RPL element) is not declared.
  void emitUndeclaredRplElement(const Decl *D,
                                const Attr *Attr,
                                const StringRef &Str) {
    StringRef BugName = "RPL element was not declared";
    helperEmitAttributeWarning(D, Attr, Str, BugName);
  }

  /// \brief  Declaration D is missing region argument(s)
  void emitMissingRegionArgs(Decl *D) {
    std::string bugName = "missing region argument(s)";

    std::string sbuf;
    llvm::raw_string_ostream strbuf(sbuf);
    D->print(strbuf, Ctx.getPrintingPolicy());

    helperEmitDeclarationWarning(D, strbuf.str(), bugName);
  }

  /// \brief  Declaration D is missing region argument(s)
  void emitUnknownNumberOfRegionParamsForType(Decl *D) {
    std::string bugName = "unknown number of region parameters for type";

    std::string sbuf;
    llvm::raw_string_ostream strbuf(sbuf);
    D->print(strbuf, Ctx.getPrintingPolicy());

    helperEmitDeclarationWarning(D, strbuf.str(), bugName);
  }

  /// \brief Region arguments Str on declaration D are superfluous for its type
  void emitSuperfluousRegionArg(Decl *D, StringRef Str) {
    std::string bugName = "superfluous region argument(s)";
    helperEmitDeclarationWarning(D, Str, bugName);
  }

  /// \brief Region name or parameter contains illegal characters
  void emitIllFormedRegionNameOrParameter(Decl *D, Attr *A) {
    StringRef Name = getRegionOrParamName(A);
    std::string AttrTypeStr = "";
    if (isa<RegionAttr>(A))
      AttrTypeStr = "region";
    else if
      (isa<RegionParamAttr>(A)) AttrTypeStr = "region parameter";
    std::string BugName = "invalid ";
    BugName.append(AttrTypeStr);
    BugName.append(" name");
    helperEmitAttributeWarning(D, A, Name, BugName);
  }

  /// \brief Effect summary not minimal: effect E1 is covered by effect E2
  void emitEffectCovered(Decl *D, Effect *E1, Effect *E2) {
    // warning: e is covered by *J
    StringRef BugName = "effect summary is not minimal";
    std::string sbuf;
    llvm::raw_string_ostream StrBuf(sbuf);
    StrBuf << "'"; E1->print(StrBuf);
    StrBuf << "' covered by '";
    E2->print(StrBuf); StrBuf << "': ";
    StrBuf << BugName;

    StringRef BugStr = StrBuf.str();
    helperEmitAttributeWarning(D, E1->getAttr(), BugStr, BugName);
  }

  /// \brief return the number of In/Arg annotations needed for type or -1
  /// if unknown
  long getRegionParamCount(QualType QT) {
    if (isNonPointerScalarType(QT)) {
      return 1;
    } else if (QT->isPointerType()) {
      long Result = getRegionParamCount(QT->getPointeeType());
      return (Result == -1) ? Result : Result + 1;
    } else if (QT->isReferenceType()) {
      return getRegionParamCount(QT->getPointeeType());
    } else if (QT->isStructureOrClassType()) {
      // FIXME allow different numbers of parameters on class types
      const RecordType *RT = QT->getAs<RecordType>();
      assert(RT);
      //RegionParamVector *RPV = ParamVectorDeclMap[RT->getDecl()];
      return 1;
    } else if (QT->isFunctionType()) {
      const FunctionType *FT = dyn_cast<FunctionType>(QT.getTypePtr());
      QualType ResultQT = FT->getResultType();
      return getRegionParamCount(ResultQT);
    } else if (QT->isVoidType()) {
      return 0;
    } else {
      // This should not happen: unknown number of region arguments for type
      return -1;
    }
  }

  /// \brief Print to the debug output stream (os) the attribute
  template<typename AttrType>
  inline void helperPrintAttributes(Decl *D) {
    for (specific_attr_iterator<AttrType>
         I = D->specific_attr_begin<AttrType>(),
         E = D->specific_attr_end<AttrType>();
         I != E; ++I) {
      (*I)->printPretty(OS, Ctx.getPrintingPolicy());
      OS << "\n";
    }
  }

    /**
   * Return the string name of the region or region parameter declaration
   * based on the Kind of the Attribute (RegionAttr or RegionParamAttr)
   */
  inline StringRef getRegionOrParamName(const Attr *Attribute) {
    StringRef Result = "";
    switch(Attribute->getKind()) {
    case attr::Region:
      Result = dyn_cast<RegionAttr>(Attribute)->getName(); break;
    case attr::RegionParam:
      Result = dyn_cast<RegionParamAttr>(Attribute)->getName(); break;
    default:
      Result = "";
    }

    return Result;
  }

  /// FIXME probably the source of a memory leak
  inline RplElement *createRegionOrParamElement(const Attr *Attribute) {
    RplElement* Result = 0;
    switch(Attribute->getKind()) {
    case attr::Region:
      Result = new NamedRplElement(dyn_cast<RegionAttr>(Attribute)->getName());
      break;
    case attr::RegionParam:
      Result = new ParamRplElement(dyn_cast<RegionParamAttr>(Attribute)->
                                   getName());
      break;
    default:
      Result = 0;
    }
    return Result;
  }

  /**
   *  Return true if any of the attributes of type AttrType of the
   *  declaration Decl* D have a region name or a param name that is
   *  the same as the 'name' provided as the second argument
   */
  template<typename AttrType>
  bool scanAttributesBool(Decl* D, const StringRef &Name)
  {
    //AttrType *A;  /// Assertion fails on a null pointer...
    //assert(isa<RegionParamAttr>(A) || isa<RegionAttr>(A));
    for (specific_attr_iterator<AttrType>
         I = D->specific_attr_begin<AttrType>(),
         E = D->specific_attr_end<AttrType>();
         I != E; ++I) {
      if (getRegionOrParamName(*I) == Name)
        return true;
    }

    return false;
  }

  /// \brief  Return an RplElement if any of the attributes of type AttrType
  /// of the declaration Decl* D have a region name or a param name that is
  /// the same as the 'name' provided as the 2nd argument.
  template<typename AttrType>
  RplElement* scanAttributes(Decl* D, const StringRef &Name)
  {
    for (specific_attr_iterator<AttrType>
         I = D->specific_attr_begin<AttrType>(),
         E = D->specific_attr_end<AttrType>();
         I != E; ++ I) {
      if (getRegionOrParamName(*I) == Name) {
        RplElement *El = RplElementMap[*I];

        std::string sbuf;
        llvm::raw_string_ostream StrBuf(sbuf);
        (*I)->printPretty(StrBuf, Ctx.getPrintingPolicy());
        OS << "DEBUG:: " << StrBuf.str() << " maps to " <<
          (El? El->getName() :"NULL") << "\n";
        assert(El);
        return El;
      }
    }

    return 0;
  }

  /// \brief Check that the region and region parameter declarations
  ///        of D are well formed (don't contain illegal characters)
  ///        and add an RplElement to the corresponding map.
  template<typename AttrType>
  bool checkRegionOrParamDecls(Decl* D) {
    bool Result = true;

    for (specific_attr_iterator<AttrType>
         I = D->specific_attr_begin<AttrType>(),
         E = D->specific_attr_end<AttrType>();
         I != E; ++I) {
      assert(isa<RegionAttr>(*I) || isa<RegionParamAttr>(*I));
      const StringRef Name = getRegionOrParamName(*I);
      OS << "DEBUG:: checking RPL Element called " << Name << "\n";
      if (isValidRegionName(Name)) {
        /// Add it to the map
        OS << "DEBUG:: creating RPL Element called " << Name << "\n";
        RplElementMap[*I] = createRegionOrParamElement(*I);
      } else {
        /// Emit bug report: ill formed region or parameter name
        emitIllFormedRegionNameOrParameter(D, *I);
        Result = false;
      }
    }
    return Result;
  }

  RplElement *findRegionOrParamName(Decl *D, StringRef Name) {
    if (!D)
      return 0;
    /// 1. try to find among regions or region parameters of function
    RplElement *Result = scanAttributes<RegionAttr>(D, Name);
    if (!Result)
      Result = scanAttributes<RegionParamAttr>(D, Name);
    return Result;
  }
  /// \brief Looks for 'Name' in the declaration 'D' and its parent scopes.
  RplElement *recursiveFindRegionOrParamName(Decl *D, StringRef Name) {

    /// 1. try to find among regions or region parameters of function
    RplElement *Result = findRegionOrParamName(D, Name);
    if (Result)
      return Result;

    /// if not found, search parent DeclContexts
    DeclContext *DC = D->getDeclContext();
    while (DC) {
      if (DC->isFunctionOrMethod()) {
        FunctionDecl* FD = dyn_cast<FunctionDecl>(DC);
        assert(FD);
        return recursiveFindRegionOrParamName(FD, Name);
      } else if (DC->isRecord()) {
        RecordDecl* RD = dyn_cast<RecordDecl>(DC);
        assert(RD);
        return recursiveFindRegionOrParamName(RD, Name);
      } else if (DC->isNamespace()) {
        NamespaceDecl *ND = dyn_cast<NamespaceDecl>(DC);
        assert(ND);
        return recursiveFindRegionOrParamName(ND, Name);
      } else {
        /// no ASaP annotations on other types of declarations
        DC = DC->getParent();
      }
    }

    return 0;
  }

  void checkTypeRegionArgs(ValueDecl *D, const Rpl *DefaultInRpl) {
    RegionArgAttr *A = D->getAttr<RegionArgAttr>();
    RplVector *RV = (A) ? RplAttrMap[A] : 0;
    if (A && !RV && FatalError)
      return; // don't check an error already occured

    QualType QT = D->getType();
    // How many In/Arg annotations does the type require?
    OS << "DEBUG:: calling 'getRegionParamCount(QT)'...";
    int ParamCount = getRegionParamCount(QT);
    OS << "(" << ParamCount << ") DONE!\n";
    long Size = (RV) ? RV->size() : 0;
    OS << "size = " << Size << "\n";

    if (ParamCount < 0) {
      FatalError = true;
      emitUnknownNumberOfRegionParamsForType(D);
    } else if (ParamCount > Size &&
               ParamCount > Size + (DefaultInRpl?1:0)) {
      FatalError = true;
      emitMissingRegionArgs(D);
    } else if (ParamCount < Size &&
               ParamCount < Size + (DefaultInRpl?1:0)) {
      std::string SBuf;
      llvm::raw_string_ostream BufStream(SBuf);
      int I = ParamCount;
      for (; I < Size-1; ++I) {
        RV->getRplAt(I)->print(BufStream);
        BufStream << ", ";
      }
      if (I <Size)
        RV->getRplAt(I)->print(BufStream);
      FatalError = true;
      emitSuperfluousRegionArg(D, BufStream.str());
    } else if (ParamCount > 0) {
      RplVector *RplVec = (RV) ? new RplVector(*RV) : 0;
      if (ParamCount > Size) {
        assert(DefaultInRpl);
        if (RplVec) {
          RplVec->push_front(DefaultInRpl);
        } else {
          RplVec = new RplVector(*DefaultInRpl);
        }
      }
      assert(ParamCount == 0 ||
             (size_t)ParamCount == RplVec->size());

      addASaPTypeToMap(ASaPTypeDeclMap, D, RplVec, 0);
      delete RplVec;
    }
    OS << "DEBUG:: DONE checkTypeRegionArgs\n";
  }

  /// \brief Check that the annotations of type AttrType of declaration
  /// D have RPLs whose elements have been declared, and if so, add RPL
  /// to the map from Attrs to Rpls.
  bool checkRpls(Decl *D, Attr *A, StringRef RplsStr) {
    /// First check that we have not already parsed this attribute's RPL
    RplVector *RV = RplAttrMap[A];
    if (RV)
      return new RplVector(*RV);
    /// else
    bool Failed = false;

    RV = new RplVector();
    llvm::SmallVector<StringRef, 8> RplVec;
    RplsStr.split(RplVec, ",");

    for (size_t I = 0 ; !Failed && I != RplVec.size(); ++I) {
      Rpl *R = checkRpl(D, A, RplVec[I].trim());
      if (R) {
        RV->push_back(R);
      } else {
        Failed = true;
      }
    }
    if (Failed) {
      delete RV;
      RV = 0;
      return false;
    } else {
      RplAttrMap[A] = RV;
      return true;
    }
  }

  /// \brief Check that the annotations of type AttrType of declaration
  /// D have RPLs whose elements have been declared, and if so, add RPL
  /// to the map from Attrs to Rpls.
  Rpl* checkRpl(Decl *D, Attr *A, StringRef RplStr) {
    /// First check that we have not already parsed this attribute's RPL

    bool Result = true;


    int Count = 0;

    Rpl *R = new Rpl();
    while(RplStr.size() > 0) { /// for all RPL elements of the RPL
      const RplElement *RplEl = 0;
      std::pair<StringRef,StringRef> Pair = Rpl::splitRpl(RplStr);
      StringRef Head = Pair.first;
      llvm::SmallVector<StringRef, 8> Vec;
      Head.split(Vec, "::");
      OS << "DEBUG:: Vec.size = " << Vec.size() << ", Vec.back() = " << Vec.back() <<"\n";

      if (Vec.size() > 1) {
        // Find the specified declaration
        DeclContext *DC = D->getDeclContext();
        DeclContextLookupResult Res;
        IdentifierInfo &II = Ctx.Idents.get(Vec[0]);
        DeclarationName DN(&II);
        OS << "DEBUG:: IdentifierInfo.getName = " << II.getName() << "\n";
        OS << "DEBUG:: DeclContext: ";
        //DC->printPretty(OS, Ctx.getPrintingPolicy());
        OS << "\n";
        while (DC && Res.size() == 0) {
          Res = DC->lookup(DN);
          OS << "DEBUG:: Lookup Result Size = " << Res.size() << "\n";
          DC = DC->getParent();
        }
        assert(Res.size() == 1); //emit warning
        DC = Decl::castToDeclContext(Res[0]);
        assert(DC);

        for(size_t I = 1; I < Vec.size() - 1; ++I) {
          IdentifierInfo &II = Ctx.Idents.get(Vec[I]);
          DeclarationName DN(&II);
          OS << "DEBUG:: IdentifierInfo.getName = " << II.getName() << "\n";
          OS << "DEBUG:: DeclContext: ";
          //DC->printPretty(OS, Ctx.getPrintingPolicy());
          OS << "\n";
          Res = DC->lookup(DN);
          OS << "DEBUG:: Lookup Result Size = " << Res.size() << "\n";
          assert(Res.size() == 1); //emit warning
          DC = Decl::castToDeclContext(Res[0]);
          assert(DC);
        }
        RplEl = findRegionOrParamName(Res[0], Vec.back());
      } else {
        assert(Vec.size() == 1);
        Head = Vec.back();
        /// head: is it a special RPL element? if not, is it declared?
        RplEl = getSpecialRplElement(Head);
        if (!RplEl)
          RplEl = recursiveFindRegionOrParamName(D, Head);
      }
      if (!RplEl) {
        // Emit bug report!
        emitUndeclaredRplElement(D, A, Head);
        Result = false;
      } else { // RplEl != NULL
        if (Count>0 && (isa<ParamRplElement>(RplEl)
                        || isa<CaptureRplElement>(RplEl))) {
          /// Error: region parameter is only allowed at the head of an Rpl
          emitMisplacedRegionParameter(D, A, Head);
        } else
          R->appendElement(RplEl);
      }
      /// Proceed to next iteration
      RplStr = Pair.second;
      ++Count;
    } // end while RplStr
    if (Result == false) {
      delete(R);
      R = 0;
    }
    return R;
  }


  /// \brief wrapper calling checkRpl.
  ///
  /// AttrType must implement getRpl (i.e., RegionArgAttr, & Effect Attributes)
  template<typename AttrType>
  bool checkRpls(Decl* D) {
    bool Success = true;
    const RplVector *RV = 0;
    for (specific_attr_iterator<AttrType>
         I = D->specific_attr_begin<AttrType>(),
         E = D->specific_attr_end<AttrType>();
         I != E; ++I) {
      Success &= checkRpls(D, *I, (*I)->getRpl());
    }
    if (!Success) {
      delete RV;
      RV = 0;
      FatalError = true;
    }
    return Success;
  }

  /// Map AttrType to Effect Kind
  inline Effect::EffectKind getEffectKind(const NoEffectAttr* Attr) {
    return Effect::EK_NoEffect;
  }
  inline Effect::EffectKind getEffectKind(const ReadsEffectAttr* Attr) {
    return Effect::EK_ReadsEffect;
  }
  inline Effect::EffectKind getEffectKind(const WritesEffectAttr* Attr) {
    return Effect::EK_WritesEffect;
  }
  inline Effect::EffectKind getEffectKind(const AtomicReadsEffectAttr* Attr) {
    return Effect::EK_AtomicReadsEffect;
  }
  inline Effect::EffectKind getEffectKind(const AtomicWritesEffectAttr* Attr) {
    return Effect::EK_AtomicWritesEffect;
  }

  /// Called with AttrType being one of ReadsEffectAttr, WritesEffectAttr,
  /// ΑtomicReadsEffectAttr, or AtomicWritesEffectAttr.
  template<typename AttrType>
  void buildPartialEffectSummary(FunctionDecl* D, Effect::EffectVector& EV) {
    for (specific_attr_iterator<AttrType>
         I = D->specific_attr_begin<AttrType>(),
         E = D->specific_attr_end<AttrType>();
         I != E; ++I) {
      Effect::EffectKind EK = getEffectKind(*I);
      RplVector *Tmp = RplAttrMap[*I];

      if (Tmp) { /// Tmp may be NULL if the RPL was ill formed (e.g., contained
                 /// undeclared RPL elements).
        for (size_t Idx = 0; Idx < Tmp->size(); ++Idx) {
          EV.push_back(new Effect(EK, Tmp->getRplAt(Idx), *I));
        }
      }
    }
  }

  void buildEffectSummary(FunctionDecl* D, Effect::EffectVector& EV) {
    buildPartialEffectSummary<ReadsEffectAttr>(D, EV);
    buildPartialEffectSummary<WritesEffectAttr>(D, EV);
    buildPartialEffectSummary<AtomicReadsEffectAttr>(D, EV);
    buildPartialEffectSummary<AtomicWritesEffectAttr>(D, EV);
    if (const NoEffectAttr* Attr = D->getAttr<NoEffectAttr>()) {
      Effect* E = new Effect(Effect::EK_NoEffect, 0, Attr);
      EV.push_back(E);
    }
  }


  /// \brief Check that an effect summary is minimal and, if not, remove
  /// superluous effects
  void checkEffectSummary(Decl* D, Effect::EffectVector& EV) {
    Effect::EffectVector::iterator I = EV.begin(); // not a const iterator
    while (I != EV.end()) { // EV.end() is not loop invariant
      bool found = false;
      for (Effect::EffectVector::iterator
            J = EV.begin(); J != EV.end(); ++J) {
        if (I != J && (*I)->isSubEffectOf(*(*J))) {
          emitEffectCovered(D, *I, *J);
          found = true;
          break;
        } // end if
      } // end inner for loop
      /// optimization: remove e from effect Summary
      if (found) I = EV.erase(I);
      else       ++I;
    } // end while loop
  }

public:

  typedef RecursiveASTVisitor<ASaPSemanticCheckerTraverser> BaseClass;

  /// Constructor
  explicit ASaPSemanticCheckerTraverser (
    ento::BugReporter &BR, ASTContext &Ctx,
    AnalysisDeclContext *AC, raw_ostream &OS,
    RplElementAttrMapTy &RplElementMap,
    RplAttrMapTy &RplAttrMap,
    ASaPTypeDeclMapTy &ASaPTypeDeclMap,
    EffectSummaryMapTy &EffectSummaryMap
    ) : BR(BR),
        Ctx(Ctx),
        AC(AC),
        OS(OS),
        RplElementMap(RplElementMap),
        RplAttrMap(RplAttrMap),
        ASaPTypeDeclMap(ASaPTypeDeclMap),
        EffectSummaryMap(EffectSummaryMap),
        FatalError(false)
  {}

  /// Getters & Setters
  inline bool encounteredFatalError() { return FatalError; }

  ///=///////////////////////////////////////////////////////////////
  /// Visitors
  bool VisitValueDecl(ValueDecl* D) {
    OS << "DEBUG:: VisitValueDecl : ";
    D->print(OS, Ctx.getPrintingPolicy());
    OS << "\n";
    return true;
  }

  bool VisitFunctionDecl(FunctionDecl* D) {
    OS << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"
       << "DEBUG:: printing ASaP attributes for method or function '";
    D->getDeclName().printName(OS);
    OS << "':\n";
    /// A. Detect Annotations
    /// A.1. Detect Region and Parameter Declarations
    helperPrintAttributes<RegionAttr>(D);

    /// A.2. Detect Region Parameter Declarations
    helperPrintAttributes<RegionParamAttr>(D);

    /// A.3. Detect Effects
    helperPrintAttributes<NoEffectAttr>(D); /// pure
    helperPrintAttributes<ReadsEffectAttr>(D); /// reads
    helperPrintAttributes<WritesEffectAttr>(D); /// writes
    helperPrintAttributes<AtomicReadsEffectAttr>(D); /// atomic reads
    helperPrintAttributes<AtomicWritesEffectAttr>(D); /// atomic writes

    /// B. Check Annotations
    /// B.1 Check Regions & Params
    checkRegionOrParamDecls<RegionAttr>(D);
    checkRegionOrParamDecls<RegionParamAttr>(D);
    /// B.2 Check ReturnType
    bool Success = checkRpls<RegionArgAttr>(D); // ReturnType
    if (Success) {
      Rpl Local(*LOCALRplElmt);
      checkTypeRegionArgs(D, &Local); // check return type
    }

    /// B.3 Check Effect RPLs
    Success = true;
    Success &= checkRpls<ReadsEffectAttr>(D);
    Success &= checkRpls<WritesEffectAttr>(D);
    Success &= checkRpls<AtomicReadsEffectAttr>(D);
    Success &= checkRpls<AtomicWritesEffectAttr>(D);

    if (Success) {
      /// C. Check effect summary
      /// C.1. Build Effect Summary
      Effect::EffectVector *EV = new Effect::EffectVector();
      Effect::EffectVector &EffectSummary = *EV;
      buildEffectSummary(D, EffectSummary);
      OS << "Effect Summary from annotation:\n";
      Effect::printEffectSummary(EffectSummary, OS);

      /// C.2. Check Effect Summary is minimal
      checkEffectSummary(D, EffectSummary);
      OS << "Minimal Effect Summary:\n";
      Effect::printEffectSummary(EffectSummary, OS);
      EffectSummaryMap[D] = EV;
    }
    return true;
  }

  bool VisitRecordDecl (RecordDecl* D) {
    OS << "DEBUG:: printing ASaP attributes for class or struct '";
    D->getDeclName().printName(OS);
    OS << "':\n";
    /// A. Detect Region & Param Annotations
    helperPrintAttributes<RegionAttr>(D);
    helperPrintAttributes<RegionParamAttr>(D);

    /// B. Check Region & Param Names
    checkRegionOrParamDecls<RegionAttr>(D);
    checkRegionOrParamDecls<RegionParamAttr>(D);

    return true;
  }

  bool VisitNamespaceDecl (NamespaceDecl *D) {
    OS << "DEBUG:: printing ASaP attributes for namespace '";
    D->getDeclName().printName(OS);
    OS << "':\n";
    /// A. Detect Region & Param Annotations
    helperPrintAttributes<RegionAttr>(D);

    /// B. Check Region & Param Names
    checkRegionOrParamDecls<RegionAttr>(D);

    return true;
  }

  bool VisitFieldDecl(FieldDecl *D) {
    OS << "DEBUG:: VisitFieldDecl : ";
    D->print(OS, Ctx.getPrintingPolicy());
    OS << "\n";

    /// A. Detect Region In & Arg annotations
    helperPrintAttributes<RegionArgAttr>(D); /// in region

    /// B. Check RPLs
    bool Success = checkRpls<RegionArgAttr>(D);

    /// C. Check validity of annotations
    if (Success)
      checkTypeRegionArgs(D, 0);

    return true;
  }

  bool VisitVarDecl(VarDecl *D) {
    OS << "DEBUG:: VisitVarDecl: ";
    D->print(OS, Ctx.getPrintingPolicy());
    OS << "\n";

    /// A. Detect Region In & Arg annotations
    helperPrintAttributes<RegionArgAttr>(D); /// in region

    /// B. Check RPLs
    bool Success = checkRpls<RegionArgAttr>(D);

    /// C. Check validity of annotations
    if (Success) {
      Rpl Local(*LOCALRplElmt);
      checkTypeRegionArgs(D, &Local);
    }

    return true;
  }

  bool VisitCXXMethodDecl(clang::CXXMethodDecl *D) {
    // ATTENTION This is called after VisitFunctionDecl
    OS << "DEBUG:: VisitCXXMethodDecl\n";
    return true;
  }

  bool VisitCXXConstructorDecl(CXXConstructorDecl *D) {
    // ATTENTION This is called after VisitCXXMethodDecl
    OS << "DEBUG:: VisitCXXConstructorDecl\n";
    return true;
  }
  bool VisitCXXDestructorDecl(CXXDestructorDecl *D) {
    // ATTENTION This is called after VisitCXXMethodDecl
    OS << "DEBUG:: VisitCXXDestructorDecl\n";
    return true;
  }
  bool VisitCXXConversionDecl(CXXConversionDecl *D) {
    // ATTENTION This is called after VisitCXXMethodDecl
    OS << "DEBUG:: VisitCXXConversionDecl\n";
    return true;
  }

  /*bool VisitCastExpr(CastExpr* E) {
    OS << "DEBUG:: VisitCastExpr: ";
    E->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    OS << "Rvalue=" << E->isRValue()
       << ", Lvalue=" << E->isLValue()
       << ", Xvalue=" << E->isGLValue()
       << ", GLvalue=" << E->isGLValue() << "\n";
    Expr::LValueClassification lvc = E->ClassifyLValue(Ctx);
    if (lvc==Expr::LV_Valid)
      OS << "LV_Valid\n";
    else
      OS << "not LV_Valid\n";

    return true;
  }


  bool VisitCallExpr(CallExpr* E) { return true; }

  /// Visit non-static C++ member function call
  bool VisitCXXMemberCallExpr(CXXMemberCallExpr *E) {
    OS << "DEBUG:: VisitCXXMemberCallExpr\n";
    return true;
  }

  /// Visits a C++ overloaded operator call where the operator
  /// is implemented as a non-static member function
  bool VisitCXXOperatorCallExpr(CXXOperatorCallExpr *E) {
    OS << "DEBUG:: VisitCXXOperatorCall\n";
    return true;
  }
*/
  /*bool VisitAssignmentExpression() {
    OS << "DEBUG:: VisitAssignmentExpression\n"
    return true;
  }*/

}; // end class ASaPSemanticCheckerTraverser
