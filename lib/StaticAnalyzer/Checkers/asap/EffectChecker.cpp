///-///////////////////////////////////////////////////////////////////////////
/// Stmt Visitor Classes


class EffectCollectorVisitor
    : public StmtVisitor<EffectCollectorVisitor, void> {

private:
  /// Fields
  ento::BugReporter& BR;
  ASTContext& Ctx;
  AnalysisManager& mgr;
  AnalysisDeclContext* AC;
  raw_ostream& os;

  /** true when we need to typecheck an assignment */
  bool typecheckAssignment;
  /** true when visiting an expression that is being written to */
  bool hasWriteSemantics;
  /** true when visiting a base expression (e.g., B in B.f, or B->f) */
  bool isBase;
  /** count of number of dereferences on expression (values in [-1, 0, ...] ) */
  int nDerefs;

  Effect::EffectVector effectsTmp;
  Effect::EffectVector effects;
  Effect::EffectVector& effectSummary;
  EffectSummaryMapTy& effectSummaryMap;
  RplAttrMapTy& rplAttrMap;
  Rpl::RplVector *lhsRegions, *rhsRegions, *tmpRegions;
  bool isCoveredBySummary;

  /// Private Methods
  void helperEmitEffectNotCoveredWarning(Stmt *S, Decl *D,
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
    ento::BugReporter& BR,
    ASTContext& Ctx,
    AnalysisManager& mgr,
    AnalysisDeclContext* AC,
    raw_ostream& os,
    Effect::EffectVector& effectsummary,
    EffectSummaryMapTy& effectSummaryMap,
    RplAttrMapTy& rplAttrMap,
    Stmt* stmt
    ) : BR(BR),
        Ctx(Ctx),
        mgr(mgr),
        AC(AC),
        os(os),
        typecheckAssignment(false),
        hasWriteSemantics(false),
        isBase(false),
        nDerefs(0),
        effectSummary(effectsummary),
        effectSummaryMap(effectSummaryMap),
        rplAttrMap(rplAttrMap),
        lhsRegions(0), rhsRegions(0), tmpRegions(0),
        isCoveredBySummary(true) {
    stmt->printPretty(os, 0, Ctx.getPrintingPolicy());
    Visit(stmt);
  }

  /// Destructor
  virtual ~EffectCollectorVisitor() {
    /// free effectsTmp
    Effect::destroyEffectVector(effectsTmp);
    if (tmpRegions) {
      Rpl::destroyRplVector(*tmpRegions);
      delete tmpRegions;
    }
  }

  /// Getters
  inline bool getIsCoveredBySummary() { return isCoveredBySummary; }

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

  void VisitMemberExpr(MemberExpr *Expr) {
    os << "DEBUG:: VisitMemberExpr: ";
    Expr->printPretty(os, 0, Ctx.getPrintingPolicy());
    os << "\n";
    /*os << "Rvalue=" << E->isRValue()
       << ", Lvalue=" << E->isLValue()
       << ", Xvalue=" << E->isGLValue()
       << ", GLvalue=" << E->isGLValue() << "\n";
    Expr::LValueClassification lvc = E->ClassifyLValue(Ctx);
    if (lvc==Expr::LV_Valid)
      os << "LV_Valid\n";
    else
      os << "not LV_Valid\n";*/
    ValueDecl* VD = Expr->getMemberDecl();
    VD->print(os, Ctx.getPrintingPolicy());
    os << "\n";

    /// 1. VD is a FunctionDecl
    const FunctionDecl *FD = dyn_cast<FunctionDecl>(VD);
    if (FD) {
      // TODO
      Effect *E = 0; // TODO here we may have a long list of effects

      /// find declaration -> find parameter(s) ->
      /// find argument(s) -> substitute
      const RegionParamAttr* Param = FD->getAttr<RegionParamAttr>();
      if (Param) { 
        /// if function has region params, find the region args on
        /// the invokation
        os << "DEBUG:: found function param";
        Param->printPretty(os, Ctx.getPrintingPolicy());
        os << "\n";
      } else {
        /// no params
        os << "DEBUG:: didn't find function param\n";
      }

      /// parameters read after substitution, invoke effects after substitution
      ///
      /// return type
      /// TODO Merge with FieldDecl (Duplicate code)
      /// 1.3. Visit Base with read semantics, then restore write semantics
      bool hws = hasWriteSemantics;
      hasWriteSemantics = false;
      Visit(Expr->getBase());
      hasWriteSemantics = hws;

      /// Post-Visit Actions: check that effects (after substitution)
      /// are covered by effect summary
      if (E) {
        E = effectsTmp.pop_back_val();
        os << "### "; E->print(os); os << "\n";
        if (!E->isCoveredBy(effectSummary)) {
          std::string Str = E->toString();
          helperEmitEffectNotCoveredWarning(Expr, VD, Str);
          isCoveredBySummary = false;
        }
      }
    }

    ///-//////////////////////////////////////////////
    /// 2. vd is a FieldDecl
    /// Type_vd <args> vd
    const FieldDecl* FieldD  = dyn_cast<FieldDecl>(VD);
    if (FieldD) {
      StringRef S;
      int EffectNr = 0;
#ifdef ATTR_REVERSE_ITERATOR_SUPPORTED
      specific_attr_reverse_iterator<RegionArgAttr>
        argit = FieldD->specific_attr_rbegin<RegionArgAttr>(),
        endit = FieldD->specific_attr_rend<RegionArgAttr>();
#else
      /// Build a reverse iterator over RegionArgAttr
      llvm::SmallVector<RegionArgAttr*, 8> argv;
      for (specific_attr_iterator<RegionArgAttr> I =
           FieldD->specific_attr_begin<RegionArgAttr>(),
           E = FieldD->specific_attr_end<RegionArgAttr>();
           I != E; ++ I) {
          argv.push_back(*I);
      }

      llvm::SmallVector<RegionArgAttr*, 8>::reverse_iterator argit =
        argv.rbegin(), endit = argv.rend();

      // Done preparing reverse iterator
#endif
      // TODO if (!arg) arg = some default
      assert(argit != endit);
      if (isBase) {
        /// 2.1 Region Substitution for expressions under this base
        RegionArgAttr* substarg;
        QualType substqt = FieldD->getType();

        os << "DEBUG:: nDerefs = " << nDerefs << "\n";
        /// Find region-argument annotation that appertains to pointee type
        for (int i = nDerefs; i>0; i--) {
          assert(argit!=endit);
          argit++;
          substqt = substqt->getPointeeType();
        }
        assert(argit!=endit);
        substarg = *argit;

        //os << "arg : ";
        //arg->printPretty(os, Ctx.getPrintingPolicy());
        //os << "\n";
        os << "DEBUG::substarg : ";
        substarg->printPretty(os, Ctx.getPrintingPolicy());
        os << "\n";

        const RegionParamAttr* rpa = getRegionParamAttr(substqt.getTypePtr());
        assert(rpa);
        // TODO support multiple Parameters
        StringRef from = rpa->getName();
        //FIXME do we need to capture here?
        const ParamRplElement fromElmt(from);
                                    
        // apply substitution to temp effects
        StringRef to = substarg->getRpl();
        Rpl* toRpl = rplAttrMap[substarg];
        assert(toRpl);

        if (from.compare(to)) { // if (from != to) then substitute
          /// 2.1.1 Substitution of effects
          for (Effect::EffectVector::const_iterator
                  it = effectsTmp.begin(),
                  end = effectsTmp.end();
                it != end; it++) {
            (*it)->substitute(fromElmt, *toRpl);
          }
          /// 2.1.2 Substitution of Regions (for typechecking)
          if (typecheckAssignment) {
            for (Rpl::RplVector::const_iterator
                    it = tmpRegions->begin(),
                    end = tmpRegions->end();
                  it != end; it++) {
              (*it)->substitute(fromElmt, *toRpl);
            }
          }
        }
      } else if (typecheckAssignment) {
        // isBase == false ==> init regions
#ifdef ATTR_REVERSE_ITERATOR_SUPPORTED
        specific_attr_reverse_iterator<RegionArgAttr> I =
          FieldD->specific_attr_rbegin<RegionArgAttr>(),
          E = FieldD->specific_attr_rend<RegionArgAttr>();
#else
        llvm::SmallVector<RegionArgAttr*, 8>::reverse_iterator I =
          argv.rbegin(), E = argv.rend();
#endif

        /// 2.1.2.1 drop region args that are sidestepped by dereferrences.
        int ndrfs = nDerefs;
        while (ndrfs>0 && I != E) {
          I ++;
          ndrfs--;
        }

        assert(ndrfs == 0 || ndrfs == -1);

        if (ndrfs==0) { /// skip the 1st arg, then add the rest
          assert(I != E);
          Rpl *tmp = rplAttrMap[(*I)];
          assert(tmp);
          if (hasWriteSemantics && nDerefs > 0
              && tmp->isFullySpecified() == false
              /* FIXME: && (*i)->/isaPointerType/ )*/) {
            /// emit capture error
            std::string Str = tmp->toString();
            helperEmitInvalidAliasingModificationWarning(Expr, VD, Str);
          }

          I++;
        }

        /// add rest of region types
        while(I != E) {
          RegionArgAttr *arg = *I;
          Rpl *rpl = rplAttrMap[arg];
          assert(rpl);
          tmpRegions->push_back(new Rpl(rpl)); // make a copy because rpl may
                                               // be modified by substitution.
          os << "DEBUG:: adding RPL for typechecking ~~~~~~~~~~ "
             << rpl->toString() << "\n";
          I ++;
        }
      }

      /// 2.2 Collect Effects
      os << "DEBUG:: isBase = " << (isBase ? "true" : "false") << "\n";
      if (nDerefs<0) { // nDeref<0 ==> AddrOf Taken
        // Do nothing. Aliasing captured by type-checker
      } else { // nDeref >=0
        /// 2.2.1. Take care of dereferences first
#ifdef ATTR_REVERSE_ITERATOR_SUPPORTED
        specific_attr_reverse_iterator<RegionArgAttr> argit =
          FieldD->specific_attr_rbegin<RegionArgAttr>(),
          endit = FieldD->specific_attr_rend<RegionArgAttr>();
#else
        llvm::SmallVector<RegionArgAttr*, 8>::reverse_iterator argit =
          argv.rbegin(), endit = argv.rend();
#endif
        QualType QT = FieldD->getType();

        for (int i = nDerefs; i > 0; i --) {
          assert(QT->isPointerType());
          assert(argit!=endit);
          /// TODO is this atomic or not? ignore atomic for now
          effectsTmp.push_back(
            new Effect(EK_ReadsEffect,
                       new Rpl(rplAttrMap[(*argit)]),
                       *argit));
          EffectNr++;
          QT = QT->getPointeeType();
          argit++;
        }

        /// 2.2.2 Take care of the destination of dereferrences, unless this
        /// is the base of a member expression. In that case, the '.' operator
        /// describes the offset from the base, and the substitution performed
        /// in earlier in 2.1 takes care of that offset.
        assert(argit!=endit);
        if (!isBase) {
          /// TODO is this atomic or not? just ignore atomic for now
          EffectKind ec = (hasWriteSemantics) ? 
                              EK_WritesEffect : EK_ReadsEffect;
          // if it is an aggregate type we have to capture all the copy effects
          // at most one of isAddrOf and isDeref can be true
          // last type to work on
          if (FieldD->getType()->isStructureOrClassType()) {
            // TODO for each field add effect & i++
            /// Actually this translates into an implicit call to an 
            /// implicit copy function... treat it as a function call.
            /// i.e., not here!
          } else {
            effectsTmp.push_back(
              new Effect(ec, new Rpl(rplAttrMap[(*argit)]), *argit));
            EffectNr++;
          }
        }
      }

      /// 2.3. Visit Base with read semantics, then restore write semantics
      bool saved_hws = hasWriteSemantics;
      bool saved_isBase = isBase; // probably not needed to save

      nDerefs = Expr->isArrow() ? 1 : 0;
      hasWriteSemantics = false;
      isBase = true;
      Visit(Expr->getBase());

      /// Post visitation checking
      hasWriteSemantics = saved_hws;
      isBase = saved_isBase;
      /// Post-Visit Actions: check that effects (after substitutions) 
      /// are covered by effect summary
      while (EffectNr) {
        Effect* e = effectsTmp.pop_back_val();
        os << "### "; e->print(os); os << "\n";
        if (!e->isCoveredBy(effectSummary)) {
          std::string Str = e->toString();
          helperEmitEffectNotCoveredWarning(Expr, VD, Str);
          isCoveredBySummary = false;
        }

        EffectNr --;
      }
    } // end if FieldDecl
  } // end VisitMemberExpr

  void VisitUnaryAddrOf(UnaryOperator *E)  {
    assert(nDerefs>=0);
    nDerefs--;
    os << "DEBUG:: Visit Unary: AddrOf (nDerefs=" << nDerefs << ")\n";
    Visit(E->getSubExpr());
  }

  void VisitUnaryDeref(UnaryOperator *E) {
    nDerefs++;
    os << "DEBUG:: Visit Unary: Deref (nDerefs=" << nDerefs << ")\n";
    Visit(E->getSubExpr());
  }

  inline void VisitPrePostIncDec(UnaryOperator *E) {
    bool savedHws = hasWriteSemantics;
    hasWriteSemantics=true;
    Visit(E->getSubExpr());
    hasWriteSemantics = savedHws;
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
    os << "DEBUG:: VisitDeclRefExpr --- whatever that is!: ";
    E->printPretty(os, 0, Ctx.getPrintingPolicy());
    os << "\n";
    nDerefs = 0;
    //TODO Collect effects
    /*os << "Rvalue=" << E->isRValue()
       << ", Lvalue=" << E->isLValue()
       << ", Xvalue=" << E->isGLValue()
       << ", GLvalue=" << E->isGLValue() << "\n";
    Expr::LValueClassification lvc = E->ClassifyLValue(Ctx);
    if (lvc==Expr::LV_Valid)
      os << "LV_Valid\n";
    else
      os << "not LV_Valid\n";
    ValueDecl* vd = E->getDecl();
    vd->print(os, Ctx.getPrintingPolicy());
    os << "\n";*/
  }

  void VisitCXXThisExpr(CXXThisExpr *E) {
    os << "DEBUG:: visiting 'this' expression\n";
    nDerefs = 0;
    if (tmpRegions && tmpRegions->empty() && !E->isImplicit()) {
      // Add parameter as implicit argument
      CXXRecordDecl* rd = const_cast<CXXRecordDecl*>(E->
                                                     getBestDynamicClassType());
      assert(rd);
      /// If the declaration does not yet have an implicit region argument
      /// add it to the Declaration
      if (!rd->getAttr<RegionArgAttr>()) {
        const RegionParamAttr *param = rd->getAttr<RegionParamAttr>();
        assert(param);
        RegionArgAttr *arg =
          ::new (rd->getASTContext()) RegionArgAttr(param->getRange(),
                                                    rd->getASTContext(),
                                                    param->getName());
        rd->addAttr(arg);

        /// also add it to rplAttrMap
        rplAttrMap[arg] = new Rpl(new ParamRplElement(param->getName()));
      }
      RegionArgAttr* arg = rd->getAttr<RegionArgAttr>();
      assert(arg);
      Rpl *tmp = rplAttrMap[arg];
      assert(tmp);
      Rpl *rpl = new Rpl(tmp);///FIXME
      tmpRegions->push_back(rpl);
    }
  }

  inline void helperVisitAssignment(BinaryOperator *E) {
    os << "DEBUG:: helperVisitAssignment (typecheck=" 
       << (typecheckAssignment?"true":"false") <<")\n";
    bool saved_hws = hasWriteSemantics;
    if (tmpRegions) {
    /// FIXME doesn't this disallow nested assignments (a=b=c)?
      Rpl::destroyRplVector(*tmpRegions);
      delete tmpRegions;
    }

    tmpRegions = new Rpl::RplVector();
    Visit(E->getRHS()); 
    rhsRegions = tmpRegions;

    tmpRegions = new Rpl::RplVector();
    hasWriteSemantics = true;
    Visit(E->getLHS());
    lhsRegions = tmpRegions;
    tmpRegions = 0;

    /// Check assignment
    os << "DEBUG:: typecheck = " << typecheckAssignment
       << ", lhs=" << (lhsRegions==0?"NULL":"Not_NULL")
       << ", rhs=" << (rhsRegions==0?"NULL":"Not_NULL")
       <<"\n";
    if(typecheckAssignment) {
      if (rhsRegions && lhsRegions) {
        // Typecheck
        Rpl::RplVector::const_iterator
                rhsI = rhsRegions->begin(),
                lhsI = lhsRegions->begin(),
                rhsE = rhsRegions->end(),
                lhsE = lhsRegions->end();
        for ( ;
              rhsI != rhsE && lhsI != lhsE;
              rhsI++, lhsI++) {
          Rpl *lhs = *lhsI;
          Rpl *rhs = *rhsI;
          if (!rhs->isIncludedIn(*lhs)) {
            helperEmitInvalidAssignmentWarning(E, lhs, rhs);
          }
        }
        assert(rhsI==rhsE);
        assert(lhsI==lhsE);
      } else if (!rhsRegions && lhsRegions) {
        // TODO How is this path reached ?
        if (lhsRegions->begin()!=lhsRegions->end()) {
          helperEmitInvalidAssignmentWarning(E, *lhsRegions->begin(), 0);
        }
      }
    }

    /// Cleanup
    hasWriteSemantics = saved_hws;
    //delete lhsRegions;
    tmpRegions = lhsRegions; // propagate up for chains of assignments
    Rpl::destroyRplVector(*rhsRegions);
    delete rhsRegions;
    rhsRegions = 0;
  }

  void VisitCompoundAssignOperator(CompoundAssignOperator *E) {
    os << "DEBUG:: !!!!!!!!!!! Mother of compound Assign!!!!!!!!!!!!!\n";
    E->printPretty(os, 0, Ctx.getPrintingPolicy());
    os << "\n";
    bool saved_tca = typecheckAssignment;
    typecheckAssignment = false;
    helperVisitAssignment(E);
    typecheckAssignment = saved_tca;
  }

  void VisitBinAssign(BinaryOperator *E) {
    os << "DEBUG:: >>>>>>>>>>VisitBinAssign<<<<<<<<<<<<<<<<<\n";
    E->printPretty(os, 0, Ctx.getPrintingPolicy());
    os << "\n";
    bool saved_tca = typecheckAssignment;
    typecheckAssignment = true;
    helperVisitAssignment(E);
    typecheckAssignment = saved_tca;
  }

  void VisitCallExpr(CallExpr *E) {
    os << "DEBUG:: VisitCallExpr\n";
  }

  /// Visit non-static C++ member function call
  void VisitCXXMemberCallExpr(CXXMemberCallExpr *E) {
    os << "DEBUG:: VisitCXXMemberCallExpr\n";
  }

  /// Visits a C++ overloaded operator call where the operator
  /// is implemented as a non-static member function
  void VisitCXXOperatorCallExpr(CXXOperatorCallExpr *E) {
    os << "DEBUG:: VisitCXXOperatorCall\n";
    E->dump(os, BR.getSourceManager());
    E->printPretty(os, 0, Ctx.getPrintingPolicy());
    os << "\n";
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
  ento::BugReporter& BR;
  ASTContext& Ctx;
  AnalysisManager& mgr;
  AnalysisDeclContext* AC;
  raw_ostream& os;
  bool fatalError;
  EffectSummaryMapTy& effectSummaryMap;
  RplAttrMapTy& rplAttrMap;

public:

  typedef RecursiveASTVisitor<ASaPEffectsCheckerTraverser> BaseClass;

  /// Constructor
  explicit ASaPEffectsCheckerTraverser(
    ento::BugReporter& BR, ASTContext& ctx,
    AnalysisManager& mgr, AnalysisDeclContext* AC, raw_ostream& os,
    EffectSummaryMapTy& esm, RplAttrMapTy& rplAttrMap
    ) : BR(BR),
        Ctx(ctx),
        mgr(mgr),
        AC(AC),
        os(os),
        fatalError(false),
        effectSummaryMap(esm),
        rplAttrMap(rplAttrMap)
  {}

  /// Visitors
  bool VisitFunctionDecl(FunctionDecl* D) {
    const FunctionDecl* Definition;
    if (D->hasBody(Definition)) {
      Stmt* st = Definition->getBody(Definition);
      assert(st);
      //os << "DEBUG:: calling Stmt Visitor\n";
      ///  Check that Effect Summary covers method effects
      Effect::EffectVector *ev = effectSummaryMap[D];
      assert(ev);
      EffectCollectorVisitor ecv(BR, Ctx, mgr, AC, os,
                                 *ev, effectSummaryMap, rplAttrMap, st);
      os << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ "
         << "(END EffectCollectorVisitor)\n";
      //os << "DEBUG:: DONE!! \n";
    }
    return true;
  }
}; /// class ASaPEffectsCheckerTraverser
