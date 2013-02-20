///-///////////////////////////////////////////////////////////////////////////
/// Stmt Visitor Classes


class EffectCollectorVisitor
    : public StmtVisitor<EffectCollectorVisitor> {

private:
  /// Fields
  ento::BugReporter &BR;
  ASTContext &Ctx;
  AnalysisManager &Mgr;
  AnalysisDeclContext *AC;
  raw_ostream &OS;

  SymbolTable &SymT;

  const FunctionDecl *Def;
  bool FatalError;

  Effect::EffectVector EffectsTmp;

  /// true when visiting an expression that is being written to
  bool HasWriteSemantics;
  /// true when visiting a base expression (e.g., B in B.f, or B->f)
  bool IsBase;
  /// count of number of dereferences on expression (values in [-1, 0, ...] )
  int DerefNum;

  bool IsCoveredBySummary;

  const EffectSummary *EffSummary;

  /// Private Methods
  /// \brief using Type with DerefNum perform substitution on all TmpEffects
  void memberSubstitute(const ASaPType *Type, int DerefNum) {
    assert(Type);
    QualType QT = Type->getQT(DerefNum);

    const ParameterVector *ParamVec = SymT.getParameterVectorFromQualType(QT);
    // TODO support multiple Parameters
    const ParamRplElement *FromEl = ParamVec->getParamAt(0);
    assert(FromEl);

    const Rpl *ToRpl = Type->getSubstArg(DerefNum);
    assert(ToRpl);
    OS << "DEBUG:: gonna substitute... " << FromEl->getName()
       << "->" << ToRpl->toString() << "\n";

    if (FromEl->getName().compare(ToRpl->toString())) {
      // if (from != to) then substitute
      /// 2.1.1 Substitution of effects
      for (Effect::EffectVector::const_iterator
              I = EffectsTmp.begin(),
              E = EffectsTmp.end();
            I != E; ++I) {
        (*I)->substitute(*FromEl, *ToRpl);
      }
    }
    OS << "   DONE\n";
  }

  /// \brief adds effects to TmpEffects and returns the number of effects added
  int collectEffects(const ASaPType *T, int DerefNum, bool IsBase) {
    if (DerefNum < 0) return 0;
    assert(T);
    int EffectNr = 0;
    ASaPType *Type = new ASaPType(*T); // Make a copy of T

    // Dereferences have read effects
    // TODO is this atomic or not? just ignore atomic for now
    for (int I = DerefNum; I > 0; --I) {
      const Rpl *InRpl = Type->getInRpl();
      assert(InRpl);
      EffectsTmp.push_back(new Effect(Effect::EK_ReadsEffect, InRpl));
      EffectNr++;
      Type->deref(DerefNum);
    }
    if (!IsBase) {
      // TODO is this atomic or not? just ignore atomic for now
      Effect::EffectKind EK = (HasWriteSemantics) ?
                              Effect::EK_WritesEffect : Effect::EK_ReadsEffect;
      const Rpl *InRpl = Type->getInRpl();
      assert(InRpl);
      EffectsTmp.push_back(new Effect(EK, InRpl));
      EffectNr++;
    }
    delete Type;
    return EffectNr;
  }

  void helperEmitEffectNotCoveredWarning(const Stmt *S,
                                         const Decl *D,
                                         const StringRef &Str) {
    StringRef BugName = "effect not covered by effect summary";
    std::string description_std = "'";
    description_std.append(Str);
    description_std.append("' ");
    description_std.append(BugName);

    StringRef BugCategory = "Safe Parallelism";
    StringRef BugStr = description_std;

    PathDiagnosticLocation VDLoc =
       PathDiagnosticLocation::createBegin(S, BR.getSourceManager(), AC);

    BR.EmitBasicReport(D, BugName, BugCategory,
                       BugStr, VDLoc, S->getSourceRange());
  }

  /// \brief Copy the effect summary of FunD and push it to the TmpEffects
  int copyAndPushEffects(FunctionDecl *FunD) {
    const EffectSummary *FunEffects = SymT.getEffectSummary(FunD);
    assert(FunEffects);
    // Must make copies because we will substitute, cannot use append:
    //EffectsTmp.append(EV->size(), (*EV->begin()));
    for(EffectSummary::const_iterator
            I = FunEffects->begin(),
            E = FunEffects->end();
         I != E; ++I) {
        EffectsTmp.push_back(new Effect(*(*I)));
    }
    return FunEffects->size();
  }

  /// \brief Check that the 'N' last effects are covered by the summary
  bool checkEffectCoverage(Expr *Exp, Decl *D, int N) {
    bool Result = true;
    for (int I=0; I<N; ++I){
      Effect* E = EffectsTmp.pop_back_val();
      OS << "### "; E->print(OS); OS << "\n";
      if (!E->isCoveredBy(*EffSummary)) {
        std::string Str = E->toString();
        helperEmitEffectNotCoveredWarning(Exp, D, Str);
        Result = false;
      }
    }
    IsCoveredBySummary &= Result;
    return Result;
  }

  /// \brief This is currently a mess and does next to nothing.
  /// Michael avert your eyes! :p
  void helperVisitFunctionDecl(MemberExpr *Expr, const FunctionDecl *FunDecl) {
    // TODO
    OS << "DEBUG:: helperVisitFunctionDecl!\n";
    //Effect *E = 0; // TODO here we may have a long list of effects

    /// find declaration -> find parameter(s) ->
    /// find argument(s) -> substitute
    const RegionParamAttr* Param = FunDecl->getAttr<RegionParamAttr>();
    if (Param) {
      /// if function has region params, find the region args on
      /// the invokation
      OS << "DEBUG:: found region param on function";
      Param->printPretty(OS, Ctx.getPrintingPolicy());
      OS << "\n";
    } else {
      /// no params
      OS << "DEBUG:: didn't find function param\n";
    }

    /// parameters read after substitution, invoke effects after substitution
    ///
    /// return type
    /// TODO Merge with FieldDecl (Duplicate code)
    /// 1.3. Visit Base with read semantics, then restore write semantics
    bool SavedHWS = HasWriteSemantics;
    bool SavedIsBase = IsBase; // probably not needed to save

    DerefNum = Expr->isArrow() ? 1 : 0;
    HasWriteSemantics = false;
    IsBase = true;
    Visit(Expr->getBase());

    /// Post visitation checking
    HasWriteSemantics = SavedHWS;
    IsBase = SavedIsBase;
    /// Post-Visit Actions: check that effects (after substitutions)
    /// are covered by effect summary
    //checkEffectCoverage(Expr, FunDecl, EffectNr); // checked up the AST
    /// Post-Visit Actions: check that effects (after substitution)
    /// are covered by effect summary
  }

  inline void helperVisitAssignment(BinaryOperator *E) {
    OS << "DEBUG:: helperVisitAssignment. ";
    E->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS <<")\n";

    bool SavedHWS = HasWriteSemantics;
    HasWriteSemantics = false;
    Visit(E->getRHS());

    HasWriteSemantics = true;
    Visit(E->getLHS());

    /// Restore flags
    HasWriteSemantics = SavedHWS;
  }


public:
  /// Constructor
  EffectCollectorVisitor (
    ento::BugReporter &BR,
    ASTContext &Ctx,
    AnalysisManager &Mgr,
    AnalysisDeclContext *AC,
    raw_ostream &OS,
    SymbolTable &SymT,
    const FunctionDecl* Def,
    Stmt *S
    ) : BR(BR),
        Ctx(Ctx),
        Mgr(Mgr),
        AC(AC),
        OS(OS),
        SymT(SymT),
        Def(Def),
        FatalError(false),
        HasWriteSemantics(false),
        IsBase(false),
        DerefNum(0),
        IsCoveredBySummary(true) {

    OS << "DEBUG:: ******** INVOKING EffectCheckerVisitor...\n";
    S->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    EffSummary = SymT.getEffectSummary(this->Def);
    assert(EffSummary);

    Visit(S);
    OS << "DEBUG:: ******** DONE INVOKING EffectCheckerVisitor ***\n";
  }

  /// Destructor
  virtual ~EffectCollectorVisitor() {
    destroyVector(EffectsTmp);
  }

  /// Getters
  inline bool getIsCoveredBySummary() { return IsCoveredBySummary; }
  inline bool encounteredFatalError() { return FatalError; }

  /// Visitors
  void VisitChildren(Stmt *S) {
    for (Stmt::child_iterator I = S->child_begin(), E = S->child_end();
         I!=E; ++I)
      if (Stmt *child = *I)
        Visit(child);
  }

  void VisitStmt(Stmt *S) {
    VisitChildren(S);
  }

  /// TODO Factor out most of this code into helper functions
  void VisitMemberExpr(MemberExpr *Expr) {
    OS << "DEBUG:: VisitMemberExpr: ";
    Expr->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    /*OS << "Rvalue=" << E->isRValue()
       << ", Lvalue=" << E->isLValue()
       << ", Xvalue=" << E->isGLValue()
       << ", GLvalue=" << E->isGLValue() << "\n";
    Expr::LValueClassification lvc = E->ClassifyLValue(Ctx);
    if (lvc==Expr::LV_Valid)
      OS << "LV_Valid\n";
    else
      OS << "not LV_Valid\n";*/
    ValueDecl* VD = Expr->getMemberDecl();
    VD->print(OS, Ctx.getPrintingPolicy());
    OS << "\n";

    /// 1. VD is a FunctionDecl
    const FunctionDecl *FD = dyn_cast<FunctionDecl>(VD);
    if (FD)
      helperVisitFunctionDecl(Expr, FD);

    ///-//////////////////////////////////////////////
    /// 2. vd is a FieldDecl
    /// Type_vd <args> vd
    const FieldDecl* FieldD  = dyn_cast<FieldDecl>(VD);
    if (FieldD) {

      const ASaPType *T = SymT.getType(FieldD);
      assert(T);
      if (IsBase)
        memberSubstitute(T, DerefNum);

      int EffectNr = collectEffects(T, DerefNum, IsBase);

      /// 2.3. Visit Base with read semantics, then restore write semantics
      bool SavedHWS = HasWriteSemantics;
      bool SavedIsBase = IsBase; // probably not needed to save

      DerefNum = Expr->isArrow() ? 1 : 0;
      HasWriteSemantics = false;
      IsBase = true;
      Visit(Expr->getBase());

      /// Post visitation checking
      HasWriteSemantics = SavedHWS;
      IsBase = SavedIsBase;
      /// Post-Visit Actions: check that effects (after substitutions)
      /// are covered by effect summary
      checkEffectCoverage(Expr, VD, EffectNr);
    } // end if FieldDecl
  } // end VisitMemberExpr

  void VisitUnaryAddrOf(UnaryOperator *E)  {
    assert(DerefNum>=0);
    DerefNum--;
    OS << "DEBUG:: Visit Unary: AddrOf (DerefNum=" << DerefNum << ")\n";
    Visit(E->getSubExpr());
  }

  void VisitUnaryDeref(UnaryOperator *E) {
    DerefNum++;
    OS << "DEBUG:: Visit Unary: Deref (DerefNum=" << DerefNum << ")\n";
    Visit(E->getSubExpr());
  }

  inline void VisitPrePostIncDec(UnaryOperator *E) {
    bool savedHws = HasWriteSemantics;
    HasWriteSemantics=true;
    Visit(E->getSubExpr());
    HasWriteSemantics = savedHws;
  }

  void VisitUnaryPostInc(UnaryOperator *E) {
    VisitPrePostIncDec(E);
  }

  void VisitUnaryPostDec(UnaryOperator *E) {
    VisitPrePostIncDec(E);
  }

  void VisitUnaryPreInc(UnaryOperator *E) {
    VisitPrePostIncDec(E);
  }

  void VisitUnaryPreDec(UnaryOperator *E) {
    VisitPrePostIncDec(E);
  }

  // TODO collect effects
  void VisitDeclRefExpr(DeclRefExpr *E) {
    OS << "DEBUG:: VisitDeclRefExpr --- whatever that is!: ";
    E->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    DerefNum = 0;
    //TODO Collect effects
    /*OS << "Rvalue=" << E->isRValue()
       << ", Lvalue=" << E->isLValue()
       << ", Xvalue=" << E->isGLValue()
       << ", GLvalue=" << E->isGLValue() << "\n";
    Expr::LValueClassification lvc = E->ClassifyLValue(Ctx);
    if (lvc==Expr::LV_Valid)
      OS << "LV_Valid\n";
    else
      OS << "not LV_Valid\n";
    ValueDecl* vd = E->getDecl();
    vd->print(OS, Ctx.getPrintingPolicy());
    OS << "\n";*/
  }

  void VisitCXXThisExpr(CXXThisExpr *E) {
    //OS << "DEBUG:: visiting 'this' expression\n";
    DerefNum = 0;
  }

  void VisitCompoundAssignOperator(CompoundAssignOperator *E) {
    OS << "DEBUG:: !!!!!!!!!!! Mother of compound Assign!!!!!!!!!!!!!\n";
    E->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    helperVisitAssignment(E);
  }

  void VisitBinAssign(BinaryOperator *E) {
    OS << "DEBUG:: >>>>>>>>>>VisitBinAssign<<<<<<<<<<<<<<<<<\n";
    E->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    helperVisitAssignment(E);
  }

  void VisitCallExpr(CallExpr *E) {
    OS << "DEBUG:: VisitCallExpr\n";
  }

  /// Visit non-static C++ member function call
  void VisitCXXMemberCallExpr(CXXMemberCallExpr *Exp) {
    OS << "DEBUG:: VisitCXXMemberCallExpr\n";
    CXXMethodDecl *D = Exp->getMethodDecl();
    assert(D);

    /// 2. Add effects to tmp effects
    int EffectCount = copyAndPushEffects(D);
    /// 3. Visit base if it exists
    VisitChildren(Exp);
    /// 4. Check coverage
    checkEffectCoverage(Exp, D, EffectCount);
  }

  /// Visits a C++ overloaded operator call where the operator
  /// is implemented as a non-static member function
  void VisitCXXOperatorCallExpr(CXXOperatorCallExpr *Exp) {
    OS << "DEBUG:: VisitCXXOperatorCall\n";
    Exp->dump(OS, BR.getSourceManager());
    Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";

    Decl *D = Exp->getCalleeDecl();
    assert(D);
    FunctionDecl *FD = dyn_cast<FunctionDecl>(D);
    assert(FD);
    OS << "DEBUG:: FunctionDecl = ";
    FD->print(OS, Ctx.getPrintingPolicy());
    OS << "DEBUG:: isOverloadedOperator = " << FD->isOverloadedOperator() << "\n";

    /// 2. Add effects to tmp effects
    int EffectCount = copyAndPushEffects(FD);
    /// 3. Visit base if it exists
    VisitChildren(Exp);

    /// 4. Check coverage
    checkEffectCoverage(Exp, D, EffectCount);
  }

}; // end class StmtVisitor
