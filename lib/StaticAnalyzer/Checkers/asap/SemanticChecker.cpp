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
  ento::BugReporter &BR;
  ASTContext &Ctx;
  AnalysisDeclContext *AC;
  raw_ostream &OS;
  bool FatalError;
  EffectSummaryMapTy &EffectSummaryMap;
  RplAttrMapTy &RplAttrMap;

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
    llvm::raw_string_ostream strbuf(sbuf);
    Attr->printPretty(strbuf, Ctx.getPrintingPolicy());

    helperEmitAttributeWarning(D, Attr, strbuf.str(), BugName);
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
    if (Qt->isAggregateType()) {
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
  void checkTypeRegionArgs(ValueDecl *D, bool LocalVar) {
    QualType QT = D->getType();
    // here we need a reverse iterator over RegionArgAttr
#ifdef ATTR_REVERSE_ITERATOR_SUPPORTED
    specific_attr_reverse_iterator<RegionArgAttr>
      I = D->specific_attr_rbegin<RegionArgAttr>(),
      E = D->specific_attr_rend<RegionArgAttr>();
#else
    llvm::SmallVector<RegionArgAttr*, 8> argv;
    for (specific_attr_iterator<RegionArgAttr>
            I = D->specific_attr_begin<RegionArgAttr>(),
            E = D->specific_attr_end<RegionArgAttr>();
         I!=E; i++){
        argv.push_back(*I);
    }

    llvm::SmallVector<RegionArgAttr*, 8>::reverse_iterator
      I = argv.rbegin(),
      E = argv.rend();
#endif
    while (I != E && QT->isPointerType()) {
      checkIsValidTypeForArg(D, QT, *I);
      I ++;
      QT = QT->getPointeeType();
    }
    /// At this point there are 3 possibilities:
    /// 1. The number of annotations is correct in which case 
    ///    I+1==E && ~QT->isPointerType()
    /// 2. There are too few annotations in which case I==E
    /// 3. There are too many annotations in which case I+1 < E
    if (I == E) {
      /// TODO attach default annotations
      if (!LocalVar) {
        emitMissingRegionArgs(D);
        FatalError = true;
      }
    } else {
      checkIsValidTypeForArg(D, QT, *I);
      I ++;
    }

    // check if there are annotations left
    while (I != E) {
      emitSuperfluousRegionArg(D, *I);
      FatalError = true;
      I ++;
    }
  }

  /// \brief Print to the debug output stream (os) the attribute
  template<typename AttrType>
  inline void helperPrintAttributes(Decl *D) {
    for (specific_attr_iterator<AttrType>
         i = D->specific_attr_begin<AttrType>(),
         e = D->specific_attr_end<AttrType>();
         i != e; ++i) {
      (*i)->printPretty(os, Ctx.getPrintingPolicy());
      os << "\n";
    }
  }

  /// \brief Check that the region and region parameter declarations
  ///        of D are well formed (don't contain illegal characters).
  template<typename AttrType>
  bool checkRegionOrParamDecls(Decl* D) {
    bool Result = true;
    
    for (specific_attr_iterator<AttrType>
         I = D->specific_attr_begin<AttrType>(),
         E = D->specific_attr_end<AttrType>();
         I != E; ++I) {
      //const Attr* attr = *i;
      const StringRef Name = getRegionOrParamName(*I);
      if (!isValidRegionName(Name)) {
        // Emit bug report!
        assert(isa<RegionAttr>(*I) || isa<RegionParamAttr>(*I));
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
    RplElement* Result = scanAttributes<RegionAttr>(D, Name);
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
  /// D have RPLs whose elements have been declared.
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
      return R;
    }
  }


  /// \brief wrapper calling checkRpl.
  ///
  /// AttrType must implement getRpl (i.e., RegionArgAttr, & Effect Attributes)
  template<typename AttrType>
  bool checkRpls(Decl* D) {
    bool result = true;
    for (specific_attr_iterator<AttrType>
         i = D->specific_attr_begin<AttrType>(),
         e = D->specific_attr_end<AttrType>();
         i != e; ++i) {
      if (!checkRpl(D, *i, (*i)->getRpl()))
        result = false;
    }
    return result;
  }

  /// Get Effect Kind from Attr type
  inline Effect::EffectKind getEffectKind(const NoEffectAttr* attr) {
    return Effect::EK_NoEffect;
  }
  inline Effect::EffectKind getEffectKind(const ReadsEffectAttr* attr) {
    return Effect::EK_ReadsEffect;
  }
  inline Effect::EffectKind getEffectKind(const WritesEffectAttr* attr) {
    return Effect::EK_WritesEffect;
  }
  inline Effect::EffectKind getEffectKind(const AtomicReadsEffectAttr* attr) {
    return Effect::EK_AtomicReadsEffect;
  }
  inline Effect::EffectKind getEffectKind(const AtomicWritesEffectAttr* attr) {
    return Effect::EK_AtomicWritesEffect;
  }

  template<typename AttrType>
  void buildPartialEffectSummary(Decl* D, Effect::EffectVector& EV) {
    for (specific_attr_iterator<AttrType>
         I = D->specific_attr_begin<AttrType>(),
         E = D->specific_attr_end<AttrType>();
         I != E; ++I) {
      Effect::EffectKind EK = getEffectKind(*I); 
      Rpl* R = 0; // TODO: I would like to be able to call this on 
                    // NoEffectAttr as well, but the compiler complains
                    // that such attributes don't have a getRpl method...
      if (Rpl* Tmp = RplAttrMap[*I]) {        
        R = new Rpl(Tmp); // FIXME: Do we need to copy here?
        EV.push_back(new Effect(EK, R, *I));
      }
    }
  }

  void buildEffectSummary(Decl* D, Effect::EffectVector& ev) {
    buildPartialEffectSummary<ReadsEffectAttr>(D, ev);
    buildPartialEffectSummary<WritesEffectAttr>(D, ev);
    buildPartialEffectSummary<AtomicReadsEffectAttr>(D, ev);
    buildPartialEffectSummary<AtomicWritesEffectAttr>(D, ev);
    if (const NoEffectAttr* attr = D->getAttr<NoEffectAttr>()) {
      Effect* e = new Effect(Effect::EK_NoEffect, 0, attr);
      ev.push_back(e);
    }
  }

  /**
   *  Check that an effect summary is minimal and, if not, remove 
   *  superluous effects
   */
  void checkEffectSummary(Decl* D, Effect::EffectVector& ev) {
    Effect::EffectVector::iterator I = ev.begin(); // not a const iterator
    while (I != ev.end()) { // ev.end() is not loop invariant
      bool found = false;
      for (Effect::EffectVector::iterator
            J = ev.begin(); J != ev.end(); J++) {
        if (I != J && (*I)->isSubEffectOf(*(*J))) {
          emitEffectCovered(D, *I, *J);
          found = true;
          break;
        } // end if
      } // end inner for loop
      /// optimization: remove e from effect Summary
      if (found) I = ev.erase(I);
      else       I++;
    } // end while loop
  }

public:

  typedef RecursiveASTVisitor<ASaPSemanticCheckerTraverser> BaseClass;

  /// Constructor
  explicit ASaPSemanticCheckerTraverser (
    ento::BugReporter &BR, ASTContext &Ctx,
    AnalysisDeclContext *AC, raw_ostream &OS,
    EffectSummaryMapTy &EffectSummaryMap, RplAttrMapTy &RplAttrMap
    ) : BR(BR),
        Ctx(Ctx),
        AC(AC),
        OS(OS),
        FatalError(false),
        EffectSummaryMap(EffectSummaryMap),
        RplAttrMap(RplAttrMap)
  {}

  /// Getters & Setters
  inline bool encounteredFatalError() { return FatalError; }

  ///=///////////////////////////////////////////////////////////////
  /// Visitors
  bool VisitValueDecl(ValueDecl* E) {
    os << "DEBUG:: VisitValueDecl : ";
    E->print(os, Ctx.getPrintingPolicy());
    os << "\n";
    return true;
  }

  bool VisitFunctionDecl(FunctionDecl* D) {
    os << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"
       << "DEBUG:: printing ASaP attributes for method or function '";
    D->getDeclName().printName(os);
    os << "':\n";
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
    /// B.2 Check Effect RPLs
    checkRpls<ReadsEffectAttr>(D);
    checkRpls<WritesEffectAttr>(D);
    checkRpls<AtomicReadsEffectAttr>(D);
    checkRpls<AtomicWritesEffectAttr>(D);

    /// C. Check effect summary
    /// C.1. Build Effect Summary
    Effect::EffectVector *ev = new Effect::EffectVector();
    Effect::EffectVector &effectSummary = *ev;
    buildEffectSummary(D, effectSummary);
    os << "Effect Summary from annotation:\n";
    Effect::printEffectSummary(effectSummary, os);

    /// C.2. Check Effect Summary is consistent and minimal
    checkEffectSummary(D, effectSummary);
    os << "Minimal Effect Summary:\n";
    Effect::printEffectSummary(effectSummary, os);
    EffectSummaryMap[D] = ev;    
    return true;
  }

  bool VisitRecordDecl (RecordDecl* D) {
    os << "DEBUG:: printing ASaP attributes for class or struct '";
    D->getDeclName().printName(os);
    os << "':\n";
    /// A. Detect Region & Param Annotations
    helperPrintAttributes<RegionAttr>(D);
    helperPrintAttributes<RegionParamAttr>(D);

    /// B. Check Region & Param Names
    checkRegionOrParamDecls<RegionAttr>(D);
    checkRegionOrParamDecls<RegionParamAttr>(D);

    return true;
  }

  bool VisitFieldDecl(FieldDecl *D) {
    os << "DEBUG:: VisitFieldDecl\n";
    /// A. Detect Region In & Arg annotations
    helperPrintAttributes<RegionArgAttr>(D); /// in region

    /// B. Check RPLs
    checkRpls<RegionArgAttr>(D);
    
    /// C. Check validity of annotations
    checkTypeRegionArgs(D, false);
    return true;
  }

  bool VisitVarDecl(VarDecl *D) {
    os << "DEBUG:: VisitVarDecl\n";
    checkTypeRegionArgs(D, true);
    return true;
  }

  bool VisitCXXMethodDecl(clang::CXXMethodDecl *D) {
    // ATTENTION This is called after VisitFunctionDecl
    os << "DEBUG:: VisitCXXMethodDecl\n";
    return true;
  }

  bool VisitCXXConstructorDecl(CXXConstructorDecl *D) {
    // ATTENTION This is called after VisitCXXMethodDecl
    os << "DEBUG:: VisitCXXConstructorDecl\n";
    return true;
  }
  bool VisitCXXDestructorDecl(CXXDestructorDecl *D) {
    // ATTENTION This is called after VisitCXXMethodDecl
    os << "DEBUG:: VisitCXXDestructorDecl\n";
    return true;
  }
  bool VisitCXXConversionDecl(CXXConversionDecl *D) {
    // ATTENTION This is called after VisitCXXMethodDecl
    os << "DEBUG:: VisitCXXConversionDecl\n";
    return true;
  }

  /*bool VisitCastExpr(CastExpr* E) {
    os << "DEBUG:: VisitCastExpr: ";
    E->printPretty(os, 0, Ctx.getPrintingPolicy());
    os << "\n";
    os << "Rvalue=" << E->isRValue()
       << ", Lvalue=" << E->isLValue()
       << ", Xvalue=" << E->isGLValue()
       << ", GLvalue=" << E->isGLValue() << "\n";
    Expr::LValueClassification lvc = E->ClassifyLValue(Ctx);
    if (lvc==Expr::LV_Valid)
      os << "LV_Valid\n";
    else
      os << "not LV_Valid\n";

    return true;
  }


  bool VisitCallExpr(CallExpr* E) { return true; }

  /// Visit non-static C++ member function call
  bool VisitCXXMemberCallExpr(CXXMemberCallExpr *E) {
    os << "DEBUG:: VisitCXXMemberCallExpr\n";
    return true;
  }

  /// Visits a C++ overloaded operator call where the operator
  /// is implemented as a non-static member function
  bool VisitCXXOperatorCallExpr(CXXOperatorCallExpr *E) {
    os << "DEBUG:: VisitCXXOperatorCall\n";
    return true;
  }
*/
  /*bool VisitAssignmentExpression() {
    os << "DEBUG:: VisitAssignmentExpression\n"
    return true;
  }*/

}; // end class ASaPSemanticCheckerTraverser
