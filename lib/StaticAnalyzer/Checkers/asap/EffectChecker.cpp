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

  RplElementAttrMapTy &RplElementMap;
  RplAttrMapTy &RplMap;
  ASaPTypeDeclMapTy &ASaPTypeDeclMap;
  EffectSummaryMapTy &EffectSummaryMap;

  const FunctionDecl *Def;
  bool FatalError;

  Effect::EffectVector EffectsTmp;
  /// true when we need to typecheck an assignment
  bool TypecheckAssignment;
  /// true when visiting an expression that is being written to
  bool HasWriteSemantics;
  /// true when visiting a base expression (e.g., B in B.f, or B->f)
  bool IsBase;
  /// count of number of dereferences on expression (values in [-1, 0, ...] )
  int DerefNum;

  bool IsCoveredBySummary;

  Effect::EffectVector *EffectSummary;

  /// Private Methods
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

  int copyAndPushEffects(FunctionDecl *D) {
    Effect::EffectVector *EV = EffectSummaryMap[D];
    assert(EV);
    // Must make copies because we will substitute, cannot use append:
    //EffectsTmp.append(EV->size(), (*EV->begin()));
    for(Effect::EffectVector::const_iterator
            I = EV->begin(),
            E = EV->end();
         I != E; ++I) {
        EffectsTmp.push_back(new Effect(*I));
    }
    return EV->size();
  }

  /// \brief Check that the 'N' last effects are covered by the summary
  bool checkEffectCoverage(Expr *Exp, Decl *D, int N) {
    bool Result = true;
    for (int I=0; I<N; ++I){
      Effect* E = EffectsTmp.pop_back_val();
      OS << "### "; E->print(OS); OS << "\n";
      if (!E->isCoveredBy(*EffectSummary)) {
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
    Effect *E = 0; // TODO here we may have a long list of effects

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
    OS << "DEBUG:: helperVisitAssignment (typecheck="
       << (TypecheckAssignment?"true":"false") << ". ";
    E->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS <<")\n";
    /*if (TypecheckAssignment) {
      /// TODO
      TypeCheckerVisitor TCV(BR, Ctx, Mgr, AC, OS, RplMap,
                             E, E->getLHS(), E->getRHS());
    }*/

    bool SavedHWS = HasWriteSemantics;
    bool SavedTCA = TypecheckAssignment;
    TypecheckAssignment = false; // Don't typecheck subtree (already done)
    HasWriteSemantics = false;  // TODO hm... is this right?
    Visit(E->getRHS());

    HasWriteSemantics = true;
    Visit(E->getLHS());

    /// Restore flags
    HasWriteSemantics = SavedHWS;
    TypecheckAssignment = SavedTCA;
  }


public:
  /// Constructor
  EffectCollectorVisitor (
    ento::BugReporter &BR,
    ASTContext &Ctx,
    AnalysisManager &Mgr,
    AnalysisDeclContext *AC,
    raw_ostream &OS,
    RplElementAttrMapTy &RplElementMap,
    RplAttrMapTy &RplMap,
    ASaPTypeDeclMapTy &ASaPTypeDeclMap,
    EffectSummaryMapTy &EffectSummaryMap,
    const FunctionDecl* Def,
    Stmt *S
    ) : BR(BR),
        Ctx(Ctx),
        Mgr(Mgr),
        AC(AC),
        OS(OS),
        RplElementMap(RplElementMap),
        RplMap(RplMap),
        ASaPTypeDeclMap(ASaPTypeDeclMap),
        EffectSummaryMap(EffectSummaryMap),
        Def(Def),
        FatalError(false),
        TypecheckAssignment(false),
        HasWriteSemantics(false),
        IsBase(false),
        DerefNum(0),
        IsCoveredBySummary(true) {

    EffectSummary = EffectSummaryMap[Def];
    assert(EffectSummary);

    Visit(S);
  }

  /// Destructor
  virtual ~EffectCollectorVisitor() {
    ASaP::destroyVector(EffectsTmp);
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
      StringRef S;
      int EffectNr = 0;
#ifdef ATTR_REVERSE_ITERATOR_SUPPORTED
      specific_attr_reverse_iterator<RegionArgAttr>
        ArgIt = FieldD->specific_attr_rbegin<RegionArgAttr>(),
        EndIt = FieldD->specific_attr_rend<RegionArgAttr>();
#else
      /// Build a reverse iterator over RegionArgAttr
      llvm::SmallVector<RegionArgAttr*, 8> ArgV;
      for (specific_attr_iterator<RegionArgAttr> I =
           FieldD->specific_attr_begin<RegionArgAttr>(),
           E = FieldD->specific_attr_end<RegionArgAttr>();
           I != E; ++ I) {
          ArgV.push_back(*I);
      }

      llvm::SmallVector<RegionArgAttr*, 8>::reverse_iterator ArgIt =
        ArgV.rbegin(), EndIt = ArgV.rend();

      // Done preparing reverse iterator
#endif
      // TODO if (!arg) arg = some default
      assert(ArgIt != EndIt);
      OS << "DEBUG:: isBase = " << (IsBase ? "true" : "false") << "\n";
      if (IsBase) {
        /// 2.1 Region Substitution for expressions under this base
        RegionArgAttr* SubstArg;
        QualType SubstQT = FieldD->getType();

        OS << "DEBUG:: DerefNum = " << DerefNum << "\n";
        /// Find region-argument annotation that appertains to pointee type
        for (int I = DerefNum; I>0; I--) {
          assert(ArgIt!=EndIt);
          ArgIt++;
          SubstQT = SubstQT->getPointeeType();
        }
        assert(ArgIt!=EndIt);
        SubstArg = *ArgIt;

        //OS << "arg : ";
        //arg->printPretty(OS, Ctx.getPrintingPolicy());
        //OS << "\n";
        OS << "DEBUG::SubstArg : ";
        SubstArg->printPretty(OS, Ctx.getPrintingPolicy());
        OS << "\n";

        const RegionParamAttr* RPA = getRegionParamAttr(SubstQT.getTypePtr());
        assert(RPA);
        // TODO support multiple Parameters
        StringRef From = RPA->getName();
        //FIXME do we need to capture here?
        // FIXME get the param from the new map
        const ParamRplElement FromElmt(From);

        // apply substitution to temp effects
        StringRef To = SubstArg->getRpl();
        Rpl* ToRpl = RplMap[SubstArg];
        assert(ToRpl);

        if (From.compare(To)) { // if (from != to) then substitute
          /// 2.1.1 Substitution of effects
          for (Effect::EffectVector::const_iterator
                  I = EffectsTmp.begin(),
                  E = EffectsTmp.end();
                I != E; ++I) {
            (*I)->substitute(FromElmt, *ToRpl);
          }
        }
      }

      /// 2.2 Collect Effects
      if (DerefNum<0) { // DerefNum<0 ==> AddrOf Taken
        // Do nothing. Aliasing captured by type-checker
      } else { // DerefNum >=0
        /// 2.2.1. Take care of dereferences first
#ifdef ATTR_REVERSE_ITERATOR_SUPPORTED
        specific_attr_reverse_iterator<RegionArgAttr>
          ArgIt = FieldD->specific_attr_rbegin<RegionArgAttr>(),
          EndIt = FieldD->specific_attr_rend<RegionArgAttr>();
#else
        llvm::SmallVector<RegionArgAttr*, 8>::reverse_iterator
          ArgIt = ArgV.rbegin(), EndIt = ArgV.rend();
#endif
        QualType QT = FieldD->getType();

        for (int I = DerefNum; I > 0; --I) {
          assert(QT->isPointerType());
          assert(ArgIt!=EndIt);
          /// TODO is this atomic or not? ignore atomic for now
          EffectsTmp.push_back(
            new Effect(Effect::EK_ReadsEffect,
                       new Rpl(RplMap[(*ArgIt)]),
                       *ArgIt));
          EffectNr++;
          QT = QT->getPointeeType();
          ArgIt++;
        }

        /// 2.2.2 Take care of the destination of dereferrences, unless this
        /// is the base of a member expression. In that case, the '.' operator
        /// describes the offset from the base, and the substitution performed
        /// in earlier in 2.1 takes care of that offset.
        assert(ArgIt!=EndIt);
        if (!IsBase) {
          /// TODO is this atomic or not? just ignore atomic for now
          Effect::EffectKind EK = (HasWriteSemantics) ?
                              Effect::EK_WritesEffect : Effect::EK_ReadsEffect;
          // if it is an aggregate type we have to capture all the copy effects
          // at most one of isAddrOf and isDeref can be true
          // last type to work on
          if (FieldD->getType()->isStructureOrClassType()) {
            // TODO for each field add effect & i++
            /// Actually this translates into an implicit call to an
            /// implicit copy function... treat it as a function call.
            /// i.e., not here!
          } else {
            EffectsTmp.push_back(
              new Effect(EK, new Rpl(RplMap[(*ArgIt)]), *ArgIt));
            EffectNr++;
          }
        }
      }

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
    bool SavedTCA = TypecheckAssignment;
    TypecheckAssignment = false;
    helperVisitAssignment(E);
    TypecheckAssignment = SavedTCA;
  }

  void VisitBinAssign(BinaryOperator *E) {
    OS << "DEBUG:: >>>>>>>>>>VisitBinAssign<<<<<<<<<<<<<<<<<\n";
    E->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    bool SavedTCA = TypecheckAssignment;
    TypecheckAssignment = true;
    helperVisitAssignment(E);
    TypecheckAssignment = SavedTCA;
  }

  void VisitCallExpr(CallExpr *E) {
    OS << "DEBUG:: VisitCallExpr\n";
  }

  /// Visit non-static C++ member function call
  void VisitCXXMemberCallExpr(CXXMemberCallExpr *Exp) {
    OS << "DEBUG:: VisitCXXMemberCallExpr\n";
    CXXMethodDecl *D = Exp->getMethodDecl();
    assert(D);

    /// 1. Typecheck assignment of actuals to formals
    /*ExprIterator ArgI, ArgE;
    FunctionDecl::param_iterator ParamI, ParamE;
    assert(D->getNumParams() == Exp->getNumArgs());

    for(ArgI = Exp->arg_begin(), ArgE = Exp->arg_end(),
        ParamI = D->param_begin(), ParamE = D->param_end();
         ArgI != ArgE && ParamI != ParamE; ++ArgI, ++ParamI) {
      OS << "DEBUG:: " << "\n";
      if ((*ArgI)->isLValue()) {
        /// Typecheck implicit assignment
      }
      Visit(*ArgI);
    }*/
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
    OS << "DEBUG:: VisitCXXMemberCallExpr\n";

    Decl *D = Exp->getCalleeDecl();
    assert(D);
    FunctionDecl *FD = dyn_cast<FunctionDecl>(D);
    assert(FD);
    /// 1. Typecheck assignment of actuals to formals
    /*ExprIterator ArgI, ArgE;
    FunctionDecl::param_iterator ParamI, ParamE;
    assert(D->getNumParams() == Exp->getNumArgs());

    for(ArgI = Exp->arg_begin(), ArgE = Exp->arg_end(),
        ParamI = D->param_begin(), ParamE = D->param_end();
         ArgI != ArgE && ParamI != ParamE; ++ArgI, ++ParamI) {
      OS << "DEBUG:: " << "\n";
      if ((*ArgI)->isLValue()) {
        /// Typecheck implicit assignment
      }
      Visit(*ArgI);
    }*/
    /// 2. Add effects to tmp effects
    int EffectCount = copyAndPushEffects(FD);
    /// 3. Visit base if it exists
    VisitChildren(Exp);

    /// 4. Check coverage
    checkEffectCoverage(Exp, D, EffectCount);
  }

}; // end class StmtVisitor
