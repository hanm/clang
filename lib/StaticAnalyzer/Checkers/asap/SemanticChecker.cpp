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
                        RplVector &RV, Rpl *InRpl) {
    assert(!ASaPTypeDeclMap[D]);
    ASaPType *T = new ASaPType(D->getType(), &RV, InRpl);
    OS << "Debug:: RV.size=" << RV.size() << ", T.RV.size="
        << T->getArgVSize() << "\n";
    ASaPTypeDeclMap[D] = T;
    OS << "Debug :: adding type: " << T->toString() << "\n";
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

#ifdef ATTR_REVERSE_ITERATOR_SUPPORTED
  #define ITERATOR_TYPE specific_attr_reverse_iterator<RegionArgAttr>
#else
  #define ITERATOR_TYPE llvm::SmallVector<RegionArgAttr*, 8>::reverse_iterator
#endif
  /// Returns 0 if no further action is needed (i.e., error, or complete type)
  /// Returns *Rpl if the scalar was a pointer (i.e., further action is needed)
  Rpl *checkScalarTypeRegionArgs(ValueDecl *D,
                                 QualType &QT,
                                 Rpl *ImplicitInAnnot,
                                 ITERATOR_TYPE &ArgIt,
                                 ITERATOR_TYPE ArgEnd) {
    Rpl *InAnnot;
    assert(QT->isScalarType());
    OS << "Debug: type is scalar\n";
    if (ImplicitInAnnot) {
      InAnnot = ImplicitInAnnot;
    } else if (ArgIt == ArgEnd) {
      emitMissingRegionArgs(D);
      FatalError = true;
      return 0;
    } else { // isScalar && !ImplicitInAnnot && ArgIt!=ArgEnd
      InAnnot = RplAttrMap[*ArgIt];
      assert(InAnnot || FatalError);
      ArgIt++;
    }
    /// Now InAnnot is set (or we have returned with an error)
    /// Check if we are done... (i.e., if this is a non Pointer Scalar)
    if (!QT->isPointerType()) { // scalar but not pointer
      /// our job here is done
      OS << "Debug:: type is (scalar and) not a pointer\n";
      RplVector RV;
      addASaPTypeToMap(ASaPTypeDeclMap, D, RV, InAnnot);
      emitRemainingArgsAreSuperfluous(D, ArgIt, ArgEnd);
      return 0;
    } else {
      QT = QT->getPointeeType();
      return InAnnot;
    }
  }

  void checkClassTypeRegionArgs(ValueDecl *D,
                                   QualType &QT,
                                   Rpl *ImplicitInAnnot,
                                   ITERATOR_TYPE &ArgIt,
                                   ITERATOR_TYPE ArgEnd) {
    assert(QT->isClassType());
    Rpl *InAnnot = 0; /// FIXME: we may need to substitute P1<-Implicit,
                     /// P2<-Implicit, ...
    RplVector RV;
    if (ImplicitInAnnot)
      RV.push_back(ImplicitInAnnot);
    else {
      if (ArgIt == ArgEnd) {
        emitMissingRegionArgs(D);
        FatalError = true;
        return;
      } else { // !isScalar && !ImplicitInAnnot && ArgIt!=ArgEnd
        checkIsValidTypeForArg(D, QT, *ArgIt);
        Rpl *R = RplAttrMap[*ArgIt];
        assert(R || FatalError);
        RV.push_back(R);
        ArgIt++;
      }
    }
    addASaPTypeToMap(ASaPTypeDeclMap, D, RV, InAnnot);
    /// unless Arg != End
    emitRemainingArgsAreSuperfluous(D, ArgIt, ArgEnd);
    return;
  }

  void checkRestRegionArgs(ValueDecl *D,
                           QualType &QT,
                           Rpl *InAnnot,
                           ITERATOR_TYPE &ArgIt,
                           ITERATOR_TYPE ArgEnd) {
    /// Grab args for multiple pointer levels
    RplVector RV;
    while (ArgIt != ArgEnd && QT->isPointerType()) {
      checkIsValidTypeForArg(D, QT, *ArgIt);
      Rpl *R = RplAttrMap[*ArgIt];
      assert(R || FatalError);
      RV.push_back(R);
      ArgIt ++;
      QT = QT->getPointeeType();
    }
    /// At this point there are 3 possibilities:
    /// 1. There are too few annotations in which case ArgIt==ArgEnd
    /// 2. The number of annotations is correct in which case
    ///    ArgIt+1==ArgEnd && !QT->isPointerType()
    /// 3. There are too many annotations in which case ArgIt+1 < ArgEnd

    /// 1. too few args
    if (ArgIt == ArgEnd) {
      /// TODO attach default annotations
      emitMissingRegionArgs(D);
      FatalError = true;
    } else { /// 2. enough arguments, hopefully not too many
      checkIsValidTypeForArg(D, QT, *ArgIt);
      Rpl *R = RplAttrMap[*ArgIt];
      assert(R || FatalError);
      OS << "Debug: pushing Rpl to ArgV before creating ASaPType (Rpl:"
        << R->toString() << "\n";
      RV.push_back(R);
      /// Add ASaP Type to map
      addASaPTypeToMap(ASaPTypeDeclMap, D, RV, InAnnot);
      ArgIt ++;
    }
    /// 3. check if there were too many arguments
    emitRemainingArgsAreSuperfluous(D, ArgIt, ArgEnd);
  }

  void checkTypeRegionArgs(ValueDecl *D,
                           QualType &QT,
                           Rpl *ImplicitInAnnot,
                           ITERATOR_TYPE ArgIt,
                           ITERATOR_TYPE ArgEnd) {
    if (QT->isReferenceType()) {
      OS << "DEBUG:: found reference type...\n";
      QT = QT.getNonReferenceType();
      checkTypeRegionArgs(D, QT, 0, ArgIt, ArgEnd);
    } else if (QT->isFunctionType()) {
      const FunctionType *FT = dyn_cast<FunctionType>(QT.getTypePtr());
      QualType ResultQT = FT->getResultType();
      OS << "Debug:: Found function type (" << QT.getAsString()
        << ") with return type = " << ResultQT.getAsString() << "\n";
      checkTypeRegionArgs(D, ResultQT, ImplicitInAnnot, ArgIt, ArgEnd);
    } else if (QT->isScalarType()) {
      Rpl *InAnnot = checkScalarTypeRegionArgs(D, QT, ImplicitInAnnot,
                                               ArgIt, ArgEnd);
      if (InAnnot) checkRestRegionArgs(D, QT, InAnnot, ArgIt, ArgEnd);
    } else if (QT->isClassType()) {
      // drop ImplicitInAnnot
      checkClassTypeRegionArgs(D, QT, 0, ArgIt, ArgEnd);
    } else if (QT->isVoidType()) {
      emitRemainingArgsAreSuperfluous(D, ArgIt, ArgEnd);
    } else {
      OS << "DEBUG:: unexpected type: " << QT.getAsString() << "\n";
      OS << "DEBUG:: isReferenceType=" << QT->isReferenceType() << "\n";
      OS << "DEBUG:: isAggregateType=" << QT->isAggregateType() << "\n";
      OS << "DEBUG:: isClassType=" << QT->isClassType() << "\n";
      FatalError = true;
    }
  }

  /// \brief calls emitSuperfluousRegionArg for the rest of the args
  void emitRemainingArgsAreSuperfluous(ValueDecl * D,
                                       ITERATOR_TYPE &ArgIt,
                                       ITERATOR_TYPE ArgEnd) {
    while (ArgIt != ArgEnd) {
      emitSuperfluousRegionArg(D, *ArgIt);
      FatalError = true;
      ArgIt ++;
    }
  }
  /// \brief Region argument A on declaration D is superfluous for type of D.
  void emitSuperfluousRegionArg(Decl *D, RegionArgAttr *A) {
    std::string bugName = "superfluous region argument";

    std::string sbuf;
    llvm::raw_string_ostream strbuf(sbuf);
    A->printPretty(strbuf, Ctx.getPrintingPolicy());

    helperEmitAttributeWarning(D, A, strbuf.str(), bugName);
  }

  /// \brief Wrong number of region arguments for type
  void emitWrongNumberOfArguments(Decl *D, RegionArgAttr *Attr) {
    std::string BugName = "wrong number of region arguments for type";

    std::string sbuf;
    llvm::raw_string_ostream StrBuf(sbuf);
    Attr->printPretty(StrBuf, Ctx.getPrintingPolicy());

    helperEmitAttributeWarning(D, Attr, StrBuf.str(), BugName);
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

  // FIXME
  inline bool isValidArgForType(const QualType Qt,
                                const RegionArgAttr *RegionArg) {
    bool Result = true;
    // TODO what about function pointers, incomplete types, ...
    if (Qt->isClassType()) {
      // alt. isRecordType = isClassType || isStructureType || isInterfaceType
      // TODO is the number of args the same as that of the params on the decl.
      // (currently we only support a single parameter per type.)
    }
    return Result;
  }

  /// \brief Check that the number of region arguments matches the number
  ///        of region parameters
  void checkIsValidTypeForArg(Decl *D, QualType QT, RegionArgAttr *Attr) {
    if (!isValidArgForType(QT, Attr)) {
      emitWrongNumberOfArguments(D, Attr);
    }
  }

  /**
   *  check that the Type T of Decl D has the proper region arg annotations
   *  Types of errors to find
   *  1. Too many arg annotations for type
   *  2. Incompatible arg annotation for type (invalid #of RPLs)
   *  3. Not enough arg annotations for type
   */
  void checkTypeRegionArgs(ValueDecl *D, Rpl *ImplicitInAnnot) {
    QualType QT = D->getType();
    RplVector RV;
    // here we need a reverse iterator over RegionArgAttr
#ifdef ATTR_REVERSE_ITERATOR_SUPPORTED
    specific_attr_reverse_iterator<RegionArgAttr>
      ArgIt = D->specific_attr_rbegin<RegionArgAttr>(),
      ArgEnd = D->specific_attr_rend<RegionArgAttr>();
#else
    llvm::SmallVector<RegionArgAttr*, 8> argv;
    for (specific_attr_iterator<RegionArgAttr>
            ArgIt = D->specific_attr_begin<RegionArgAttr>(),
            ArgEnd = D->specific_attr_end<RegionArgAttr>();
         ArgIt!=ArgEnd; ++ArgIt){
        argv.push_back(*ArgIt);
    }

    llvm::SmallVector<RegionArgAttr*, 8>::reverse_iterator
      ArgIt = argv.rbegin(),
      ArgEnd = argv.rend();
#endif
    /// Compute In Annotation
    checkTypeRegionArgs(D, QT, ImplicitInAnnot, ArgIt, ArgEnd);
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
  // FIXME
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
    //AttrType *A; /// Assertion fails on a null pointer...
    //assert(isa<RegionParamAttr>(A) || isa<RegionAttr>(A));
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

  /// \brief Looks for 'Name' in the declaration 'D' and its parent scopes.
  RplElement *findRegionOrParamName(Decl *D, StringRef Name) {
    if (!D)
      return 0;
    /// 1. try to find among regions or region parameters of function
    RplElement *Result = scanAttributes<RegionAttr>(D, Name);
    if (!Result)
      Result = scanAttributes<RegionParamAttr>(D, Name);
    if (Result)
      return Result;

    /// if not found, search parent DeclContexts
    DeclContext *DC = D->getDeclContext();
    while (DC) {
      if (DC->isFunctionOrMethod()) {
        FunctionDecl* FD = dyn_cast<FunctionDecl>(DC);
        assert(FD);
        return findRegionOrParamName(FD, Name);
      } else if (DC->isRecord()) {
        RecordDecl* RD = dyn_cast<RecordDecl>(DC);
        assert(RD);
        return findRegionOrParamName(RD, Name);
      } else if (DC->isNamespace()) {
        NamespaceDecl *ND = dyn_cast<NamespaceDecl>(DC);
        assert(ND);
        return findRegionOrParamName(ND, Name);
      } else {
        /// no ASaP annotations on other types of declarations
        DC = DC->getParent();
      }
    }

    return 0;
  }

  /// \brief Check that the annotations of type AttrType of declaration
  /// D have RPLs whose elements have been declared, and if so, add RPL
  /// to the map from Attrs to Rpls.
  Rpl* checkRpl(Decl *D, Attr *A, StringRef RplStr) {
    /// First check that we have not already parsed this attribute's RPL
    Rpl *R = RplAttrMap[A];
    if (R)
      return R;
    /// else
    bool Result = true;
    int Count = 0;

    R = new Rpl();
    while(RplStr.size() > 0) { /// for all RPL elements of the RPL
      // FIXME: '::' can appear as part of an RPL element. Splitting must
      // be done differently to account for that.
      std::pair<StringRef,StringRef> Pair = RplStr.split(Rpl::
                                                          RPL_SPLIT_CHARACTER);
      const StringRef& Head = Pair.first;
      /// head: is it a special RPL element? if not, is it declared?
      const RplElement *RplEl = getSpecialRplElement(Head);
      if (!RplEl)
        RplEl = findRegionOrParamName(D, Head);
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
    }
    if (Result == false) {
      delete(R);
      FatalError = true;
      return 0;
    } else {
      RplAttrMap[A] = R;

      //std::string sbuf;
      //llvm::raw_string_ostream StrBuf(sbuf);
      //A->printPretty(StrBuf, Ctx.getPrintingPolicy());
      //OS << "DEBUG:: adding " << R->toString() << "to Map under "
      //  << StrBuf.str() << "\n";

      return R;
    }
  }


  /// \brief wrapper calling checkRpl.
  ///
  /// AttrType must implement getRpl (i.e., RegionArgAttr, & Effect Attributes)
  template<typename AttrType>
  bool checkRpls(Decl* D) {
    bool Result = true;
    for (specific_attr_iterator<AttrType>
         I = D->specific_attr_begin<AttrType>(),
         E = D->specific_attr_end<AttrType>();
         I != E; ++I) {
      if (!checkRpl(D, *I, (*I)->getRpl()))
        Result = false;
    }
    return Result;
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
      Rpl* Tmp = RplAttrMap[*I];

      if (Tmp) { /// Tmp may be NULL if the RPL was ill formed (e.g., contained
                 /// undeclared RPL elements).
        EV.push_back(new Effect(EK, new Rpl(*Tmp), *I));
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
    /// B.2 Check ReturnType and Effect RPLs
    checkRpls<RegionArgAttr>(D); // ReturnType
    checkRpls<ReadsEffectAttr>(D);
    checkRpls<WritesEffectAttr>(D);
    checkRpls<AtomicReadsEffectAttr>(D);
    checkRpls<AtomicWritesEffectAttr>(D);

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
    checkRpls<RegionArgAttr>(D);

    /// C. Check validity of annotations
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
    checkRpls<RegionArgAttr>(D);

    /// C. Check validity of annotations
    Rpl Local(*LOCALRplElmt);
    checkTypeRegionArgs(D, &Local);
    return true;
  }

  bool VisitCXXMethodDecl(clang::CXXMethodDecl *D) {
    // ATTENTION This is called after VisitFunctionDecl
    OS << "DEBUG:: VisitCXXMethodDecl\n";
    Rpl Local(*LOCALRplElmt);
    checkTypeRegionArgs(D, &Local); // check return type
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
