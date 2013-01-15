///-///////////////////////////////////////////////////////////////////////////
/// Stmt Visitor Classes


class EffectCollectorVisitor
    : public StmtVisitor<EffectCollectorVisitor, void> {

private:
  /// Fields
  ento::BugReporter &BR;
  ASTContext &Ctx;
  AnalysisManager &Mgr;
  AnalysisDeclContext *AC;
  raw_ostream &OS;

  /** true when we need to typecheck an assignment */
  bool TypecheckAssignment;
  /** true when visiting an expression that is being written to */
  bool HasWriteSemantics;
  /** true when visiting a base expression (e.g., B in B.f, or B->f) */
  bool IsBase;
  /** count of number of dereferences on expression (values in [-1, 0, ...] ) */
  int DerefNum;

  Effect::EffectVector EffectsTmp;
  Effect::EffectVector effects;
  Effect::EffectVector &EffectSummary;
  EffectSummaryMapTy &EffectSummaryMap;
  RplAttrMapTy &RplAttrMap;
  Rpl::RplVector *LHSRegions, *RHSRegions, *TmpRegions;
  bool IsCoveredBySummary;

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

  void helperEmitInvalidAliasingModificationWarning(Stmt *S, Decl *D,
                                                    const StringRef &Str) {
    StringRef BugName =
      "cannot modify aliasing through pointer to partly specified region";
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

  void helperEmitInvalidAssignmentWarning(Stmt *S, Rpl *LHS, Rpl *RHS) {
    StringRef BugName = "invalid assignment";

    std::string description_std = "RHS region '";
    description_std.append(RHS ? RHS->toString() : "");
    description_std.append("' is not included in LHS region '");
    description_std.append(LHS ? LHS->toString() : "");
    description_std.append("' ");
    description_std.append(BugName);

    StringRef BugCategory = "Safe Parallelism";
    StringRef BugStr = description_std;

    PathDiagnosticLocation VDLoc =
       PathDiagnosticLocation::createBegin(S, BR.getSourceManager(), AC);

    BugType *BT = new BugType(BugName, BugCategory);
    BugReport *R = new BugReport(*BT, BugStr, VDLoc);
    BR.emitReport(R);
    //BR.EmitBasicReport(D, bugName, bugCategory,
    //                   bugStr, VDLoc, S->getSourceRange());
  }

public:
  /// Constructor
  EffectCollectorVisitor (
    ento::BugReporter &BR,
    ASTContext &Ctx,
    AnalysisManager &Mgr,
    AnalysisDeclContext *AC,
    raw_ostream &OS,
    Effect::EffectVector &Effectsummary,
    EffectSummaryMapTy &EffectSummaryMap,
    RplAttrMapTy &RplAttrMap,
    Stmt *S
    ) : BR(BR),
        Ctx(Ctx),
        Mgr(Mgr),
        AC(AC),
        OS(OS),
        TypecheckAssignment(false),
        HasWriteSemantics(false),
        IsBase(false),
        DerefNum(0),
        EffectSummary(Effectsummary),
        EffectSummaryMap(EffectSummaryMap),
        RplAttrMap(RplAttrMap),
        LHSRegions(0), RHSRegions(0), TmpRegions(0),
        IsCoveredBySummary(true) {
    S->printPretty(OS, 0, Ctx.getPrintingPolicy());
    Visit(S);
  }

  /// Destructor
  virtual ~EffectCollectorVisitor() {
    /// free effectsTmp
    ASaP::destroyVector(EffectsTmp);
    if (TmpRegions) {
      ASaP::destroyVector(*TmpRegions);
      delete TmpRegions;
    }
  }

  /// Getters
  inline bool getIsCoveredBySummary() { return IsCoveredBySummary; }

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
      OS << "DEBUG:: found function param";
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
    bool hws = HasWriteSemantics;
    HasWriteSemantics = false;
    Visit(Expr->getBase());
    HasWriteSemantics = hws;

    /// Post-Visit Actions: check that effects (after substitution)
    /// are covered by effect summary
    if (E) {
      E = EffectsTmp.pop_back_val();
      OS << "### "; E->print(OS); OS << "\n";
      if (!E->isCoveredBy(EffectSummary)) {
        std::string Str = E->toString();
        helperEmitEffectNotCoveredWarning(Expr, FunDecl, Str);
        IsCoveredBySummary = false;
      }
    }
  }

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
        OS << "DEBUG::substarg : ";
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
        Rpl* ToRpl = RplAttrMap[SubstArg];
        assert(ToRpl);

        if (From.compare(To)) { // if (from != to) then substitute
          /// 2.1.1 Substitution of effects
          for (Effect::EffectVector::const_iterator
                  I = EffectsTmp.begin(),
                  E = EffectsTmp.end();
                I != E; ++I) {
            (*I)->substitute(FromElmt, *ToRpl);
          }
          /// 2.1.2 Substitution of Regions (for typechecking)
          if (TypecheckAssignment) {
            for (Rpl::RplVector::const_iterator
                    I = TmpRegions->begin(),
                    E = TmpRegions->end();
                  I != E; ++I) {
              (*I)->substitute(FromElmt, *ToRpl);
            }
          }
        }
      } else if (TypecheckAssignment) {
        // isBase == false ==> init regions
#ifdef ATTR_REVERSE_ITERATOR_SUPPORTED
        specific_attr_reverse_iterator<RegionArgAttr>
          I = FieldD->specific_attr_rbegin<RegionArgAttr>(),
          E = FieldD->specific_attr_rend<RegionArgAttr>();
#else
        llvm::SmallVector<RegionArgAttr*, 8>::reverse_iterator I =
          ArgV.rbegin(), E = argv.rend();
#endif

        /// 2.1.2.1 drop region args that are sidestepped by dereferrences.
        int N = DerefNum;
        while (N>0 && I != E) {
          I ++;
          N--;
        }

        assert(N == 0 || N == -1);

        if (N==0) { /// skip the 1st arg, then add the rest
          assert(I != E);
          Rpl *Tmp = RplAttrMap[(*I)];
          assert(Tmp);
          if (HasWriteSemantics && DerefNum > 0
              && Tmp->isFullySpecified() == false
              /* FIXME: && (*i)->/isaPointerType/ )*/) {
            /// emit capture error
            std::string Str = Tmp->toString();
            helperEmitInvalidAliasingModificationWarning(Expr, VD, Str);
          }

          I++;
        }

        /// add rest of region types
        while(I != E) {
          RegionArgAttr *Arg = *I;
          Rpl *R = RplAttrMap[Arg];
          assert(R);
          TmpRegions->push_back(new Rpl(R)); // make a copy because rpl may
                                               // be modified by substitution.
          OS << "DEBUG:: adding RPL for typechecking ~~~~~~~~~~ "
             << R->toString() << "\n";
          ++I;
        }
      }

      /// 2.2 Collect Effects
      OS << "DEBUG:: isBase = " << (IsBase ? "true" : "false") << "\n";
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
          /// FIXME memory leak
          EffectsTmp.push_back(
            new Effect(Effect::EK_ReadsEffect,
                       new Rpl(RplAttrMap[(*ArgIt)]),
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
              new Effect(EK, new Rpl(RplAttrMap[(*ArgIt)]), *ArgIt));
            EffectNr++;
          }
        }
      }

      /// 2.3. Visit Base with read semantics, then restore write semantics
      bool saved_hws = HasWriteSemantics;
      bool saved_isBase = IsBase; // probably not needed to save

      DerefNum = Expr->isArrow() ? 1 : 0;
      HasWriteSemantics = false;
      IsBase = true;
      Visit(Expr->getBase());

      /// Post visitation checking
      HasWriteSemantics = saved_hws;
      IsBase = saved_isBase;
      /// Post-Visit Actions: check that effects (after substitutions)
      /// are covered by effect summary
      while (EffectNr) {
        Effect* E = EffectsTmp.pop_back_val();
        OS << "### "; E->print(OS); OS << "\n";
        if (!E->isCoveredBy(EffectSummary)) {
          std::string Str = E->toString();
          helperEmitEffectNotCoveredWarning(Expr, VD, Str);
          IsCoveredBySummary = false;
        }

        EffectNr --;
      }
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
    OS << "DEBUG:: visiting 'this' expression\n";
    DerefNum = 0;
    if (TmpRegions && TmpRegions->empty() && !E->isImplicit()) {
      // Add parameter as implicit argument
      CXXRecordDecl* RecDecl = const_cast<CXXRecordDecl*>(E->
                                                     getBestDynamicClassType());
      assert(RecDecl);
      /// If the declaration does not yet have an implicit region argument
      /// add it to the Declaration
      if (!RecDecl->getAttr<RegionArgAttr>()) {
        const RegionParamAttr *Param = RecDecl->getAttr<RegionParamAttr>();
        assert(Param);
        RegionArgAttr *Arg =
          ::new (RecDecl->getASTContext()) RegionArgAttr(Param->getRange(),
                                                    RecDecl->getASTContext(),
                                                    Param->getName());
        RecDecl->addAttr(Arg);

        /// also add it to RplAttrMap
        RplAttrMap[Arg] = new Rpl(new ParamRplElement(Param->getName()));
      }
      RegionArgAttr* Arg = RecDecl->getAttr<RegionArgAttr>();
      assert(Arg);
      Rpl *Tmp = RplAttrMap[Arg];
      assert(Tmp);
      Rpl *R = new Rpl(Tmp);///FIXME memory leak?
      TmpRegions->push_back(R);
    }
  }

  inline void helperVisitAssignment(BinaryOperator *E) {
    OS << "DEBUG:: helperVisitAssignment (typecheck="
       << (TypecheckAssignment?"true":"false") <<")\n";
    bool saved_hws = HasWriteSemantics;
    if (TmpRegions) {
    /// FIXME doesn't this disallow nested assignments (a=b=c)?
      ASaP::destroyVector(*TmpRegions);
      delete TmpRegions;
    }

    TmpRegions = new Rpl::RplVector();
    Visit(E->getRHS());
    RHSRegions = TmpRegions;

    TmpRegions = new Rpl::RplVector();
    HasWriteSemantics = true;
    Visit(E->getLHS());
    LHSRegions = TmpRegions;
    TmpRegions = 0;

    /// Check assignment
    OS << "DEBUG:: typecheck = " << TypecheckAssignment
       << ", lhs=" << (LHSRegions==0?"NULL":"Not_NULL")
       << ", rhs=" << (RHSRegions==0?"NULL":"Not_NULL")
       <<"\n";
    if(TypecheckAssignment) {
      if (RHSRegions && LHSRegions) {
        // Typecheck
        Rpl::RplVector::const_iterator
                RHSI = RHSRegions->begin(),
                LHSI = LHSRegions->begin(),
                RHSE = RHSRegions->end(),
                LHSE = LHSRegions->end();
        for ( ;
              RHSI != RHSE && LHSI != LHSE;
              RHSI++, LHSI++) {
          Rpl *LHS = *LHSI;
          Rpl *RHS = *RHSI;
          if (!RHS->isIncludedIn(*LHS)) {
            helperEmitInvalidAssignmentWarning(E, LHS, RHS);
          }
        }
        assert(RHSI==RHSE);
        assert(LHSI==LHSE);
      } else if (!RHSRegions && LHSRegions) {
        // TODO How is this path reached ?
        if (LHSRegions->begin()!=LHSRegions->end()) {
          helperEmitInvalidAssignmentWarning(E, *LHSRegions->begin(), 0);
        }
      }
    }

    /// Cleanup
    HasWriteSemantics = saved_hws;
    //delete lhsRegions;
    TmpRegions = LHSRegions; // propagate up for chains of assignments
    ASaP::destroyVector(*RHSRegions);
    delete RHSRegions;
    RHSRegions = 0;
  }

  void VisitCompoundAssignOperator(CompoundAssignOperator *E) {
    OS << "DEBUG:: !!!!!!!!!!! Mother of compound Assign!!!!!!!!!!!!!\n";
    E->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    bool saved_tca = TypecheckAssignment;
    TypecheckAssignment = false;
    helperVisitAssignment(E);
    TypecheckAssignment = saved_tca;
  }

  void VisitBinAssign(BinaryOperator *E) {
    OS << "DEBUG:: >>>>>>>>>>VisitBinAssign<<<<<<<<<<<<<<<<<\n";
    E->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    bool saved_tca = TypecheckAssignment;
    TypecheckAssignment = true;
    helperVisitAssignment(E);
    TypecheckAssignment = saved_tca;
  }

  void VisitCallExpr(CallExpr *E) {
    OS << "DEBUG:: VisitCallExpr\n";
  }

  /// Visit non-static C++ member function call
  void VisitCXXMemberCallExpr(CXXMemberCallExpr *Exp) {
    OS << "DEBUG:: VisitCXXMemberCallExpr\n";
    /// 1. Typecheck assignment of actuals to formals
    /*for(ExprIterator
            I = Exp->arg_begin(),
            E = Exp->arg_end();
         I != E; ++I) {
      //Visit(*I);
    }*/
    /// 2. Check that the Effects are covered
    //CXXMethodDecl *D = dyn_cast<CXXMethodDecl>(Exp->getCalleeDecl());
    //assert(D);
    //Effect::EffectVector *EV = effectSummaryMap[D];
  }

  /// Visits a C++ overloaded operator call where the operator
  /// is implemented as a non-static member function
  void VisitCXXOperatorCallExpr(CXXOperatorCallExpr *E) {
    OS << "DEBUG:: VisitCXXOperatorCall\n";
    E->dump(OS, BR.getSourceManager());
    E->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
  }

  // TODO ++ etc operators
}; // end class StmtVisitor

///-///////////////////////////////////////////////////////////////////////////
/// AST Traverser Class


/**
 * Wrapper pass that calls effect checker on each function definition.
 * This pass also typechecks assignments to avoid code duplication by having
 * a separate pass do that
 */
class ASaPEffectsCheckerTraverser :
  public RecursiveASTVisitor<ASaPEffectsCheckerTraverser> {

private:
  /// Private Fields
  ento::BugReporter &BR;
  ASTContext &Ctx;
  AnalysisManager &Mgr;
  AnalysisDeclContext *AC;
  raw_ostream &OS;
  bool FatalError;
  EffectSummaryMapTy &EffectSummaryMap;
  RplAttrMapTy &RplAttrMap;

public:

  typedef RecursiveASTVisitor<ASaPEffectsCheckerTraverser> BaseClass;

  /// Constructor
  explicit ASaPEffectsCheckerTraverser(
    ento::BugReporter &BR, ASTContext &Ctx,
    AnalysisManager &Mgr, AnalysisDeclContext *AC, raw_ostream &OS,
    EffectSummaryMapTy &ESM, RplAttrMapTy &RplAttrMap
    ) : BR(BR),
        Ctx(Ctx),
        Mgr(Mgr),
        AC(AC),
        OS(OS),
        FatalError(false),
        EffectSummaryMap(ESM),
        RplAttrMap(RplAttrMap)
  {}

  /// Visitors
  bool VisitFunctionDecl(FunctionDecl* D) {
    const FunctionDecl* Definition;
    if (D->hasBody(Definition)) {
      Stmt* St = Definition->getBody(Definition);
      assert(St);
      //OS << "DEBUG:: calling Stmt Visitor\n";
      ///  Check that Effect Summary covers method effects
      Effect::EffectVector *EV = EffectSummaryMap[D];
      assert(EV);
      EffectCollectorVisitor ECV(BR, Ctx, Mgr, AC, OS,
                                 *EV, EffectSummaryMap, RplAttrMap, St);
      OS << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ "
         << "(END EffectCollectorVisitor)\n";
      //OS << "DEBUG:: DONE!! \n";
    }
    return true;
  }
}; /// class ASaPEffectsCheckerTraverser
