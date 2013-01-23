// TODO
// (b ? x : y) = (c ? w : z)
// (b = exp1 ? x : y) = (c = exp2 ? w :z)
// x = x + q.v // don't gather regions from q.v


///-///////////////////////////////////////////////////////////////////////////
/// Stmt Visitor Classes


class TypeCheckerVisitor
    : public StmtVisitor<TypeCheckerVisitor, void> {

private:
  /// Fields
  ento::BugReporter &BR;
  ASTContext &Ctx;
  AnalysisManager &Mgr;
  AnalysisDeclContext *AC;
  raw_ostream &OS;

  /// true when we need to typecheck an assignment
  //bool TypecheckAssignment;
  /// true when visiting a base expression (e.g., B in B.f, or B->f)
  bool IsBase;
  /// count of number of dereferences on expression (values in [-1, 0, ...] )
  int DerefNum;

  RplAttrMapTy &RplAttrMap;
  Rpl::RplVector *TmpRegions;
  static const int VECVEC_SIZE = 4;
  typedef llvm::SmallVector<Rpl::RplVector*, VECVEC_SIZE> RplVectorSetTy;
  RplVectorSetTy *TmpRegionsVec, *LHSRegionsVec, *RHSRegionsVec;

  /// Private Methods
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
    description_std.append("' [");
    description_std.append(BugName);
    std::string SBuf;
    llvm::raw_string_ostream StrBuf(SBuf);
    StrBuf << ": ";
    S->printPretty(StrBuf, 0, Ctx.getPrintingPolicy());
    StrBuf << "]";
    description_std.append(StrBuf.str());

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

  bool typecheckAssignment (Expr *E) {
    bool Result = true;
    assert(RHSRegionsVec);
    assert(LHSRegionsVec);

    for(RplVectorSetTy::const_iterator
            LHSRegsI = LHSRegionsVec->begin(),
            LHSRegsE = LHSRegionsVec->end();
        LHSRegsI != LHSRegsE; ++LHSRegsI) {

      for(RplVectorSetTy::const_iterator
              RHSRegsI = RHSRegionsVec->begin(),
              RHSRegsE = RHSRegionsVec->end();
          RHSRegsI != RHSRegsE; ++RHSRegsI) {
        Result &= typecheckAssignmentInner(E, *LHSRegsI, *RHSRegsI);
      }
    }
    return Result;
  }

  bool typecheckAssignmentInner (Expr *E,
                                 const Rpl::RplVector *LHSRegs,
                                 const Rpl::RplVector *RHSRegs) {
    bool Result = true;
    assert(RHSRegs);
    assert(LHSRegs);
    OS << "DEBUG:: RHS.size = " << RHSRegs->size()
       << ", LHS.size = " << LHSRegs->size() << "\n";


    if (RHSRegs->size()>0 && LHSRegs->size()>0) {
      // Typecheck
      Rpl::RplVector::const_iterator
              RHSI = RHSRegs->begin(),
              LHSI = LHSRegs->begin(),
              RHSE = RHSRegs->end(),
              LHSE = LHSRegs->end();
      for ( ;
            RHSI != RHSE && LHSI != LHSE;
            RHSI++, LHSI++) {
        Rpl *LHS = *LHSI;
        Rpl *RHS = *RHSI;
        if (!RHS->isIncludedIn(*LHS)) {
          helperEmitInvalidAssignmentWarning(E, LHS, RHS);
          Result = false;
        }
      }
      assert(RHSI==RHSE);
      assert(LHSI==LHSE);
    } else if (RHSRegs->size()>0 && LHSRegs->size()==0) {
      // TODO How is this path reached ?
      if (LHSRegs->begin()!=LHSRegs->end()) {
        helperEmitInvalidAssignmentWarning(E, *LHSRegs->begin(), 0);
        Result = false;
      }
    }
    return Result;
  }

  /// \brief substitute region parameters with arguments in TmpRegions
  void substitute(const FieldDecl *FieldD) {
    /// 1. Set up Reverse iterator over Arg Attributes (annotations)
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

    llvm::SmallVector<RegionArgAttr*, 8>::reverse_iterator
        ArgIt = ArgV.rbegin(),
        EndIt = ArgV.rend();
    // Done preparing reverse iterator
#endif
    // TODO if (!arg) arg = some default
    assert(ArgIt != EndIt);
    OS << "DEBUG:: isBase = " << (IsBase ? "true" : "false") << "\n";

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
    Rpl* ToRpl = RplAttrMap[SubstArg];
    assert(ToRpl);

    if (From.compare(To)) { // if (from != to) then substitute
      /// 2.2 Substitution of Regions (for typechecking)
      for (Rpl::RplVector::const_iterator
                I = TmpRegions->begin(),
                E = TmpRegions->end();
            I != E; ++I) {
        (*I)->substitute(FromElmt, *ToRpl);
      }
    }
  }

public:
  /// Constructor
  TypeCheckerVisitor (
    ento::BugReporter &BR,
    ASTContext &Ctx,
    AnalysisManager &Mgr,
    AnalysisDeclContext *AC,
    raw_ostream &OS,
    RplAttrMapTy &RplAttrMap,
    Expr *E, Expr *LHS, Expr *RHS
    ) : BR(BR),
        Ctx(Ctx),
        Mgr(Mgr),
        AC(AC),
        OS(OS),
        IsBase(false),
        DerefNum(0),
        RplAttrMap(RplAttrMap),
        TmpRegions(0),
        TmpRegionsVec(0),
        LHSRegionsVec(0),
        RHSRegionsVec(0) {

    TmpRegionsVec = new RplVectorSetTy();
    TmpRegionsVec->push_back(new Rpl::RplVector());
    TmpRegions = TmpRegionsVec->back();
    Visit(RHS);
    RHSRegionsVec = TmpRegionsVec;

    TmpRegionsVec = new RplVectorSetTy();
    TmpRegionsVec->push_back(new Rpl::RplVector());
    TmpRegions = TmpRegionsVec->back();
    Visit(LHS);
    LHSRegionsVec = TmpRegionsVec;

    /// Check assignment
    //if(TypecheckAssignment)
      typecheckAssignment(E);

    // propagate up for chains of assignments
    assert(TmpRegionsVec == LHSRegionsVec);
    /// Cleanup
    ASaP::destroyVectorVector(*RHSRegionsVec);
    delete RHSRegionsVec;
    RHSRegionsVec = 0;

  }

  /// Destructor
  virtual ~TypeCheckerVisitor() {
    /// free effectsTmp
    if (TmpRegionsVec) {
      ASaP::destroyVectorVector(*TmpRegionsVec);
      delete TmpRegionsVec;
    }
  }

  /// Getters
  inline RplVectorSetTy *getLHSRegionsVec() {
    TmpRegionsVec = 0;
    return LHSRegionsVec;
  }

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

  /// FIXME Do we need to keep track of DerefNum in TypeChecker?
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

  void VisitDeclRefExpr(DeclRefExpr *E) {
    OS << "DEBUG:: VisitDeclRefExpr --- whatever that is!: ";
    E->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    DerefNum = 0;
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
    //if (!IsBase) { // this should in practice be equivalent to the above
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
      TmpRegions->push_back(new Rpl(Tmp));
      /*
      const RegionParamAttr *Param = RecDecl->getAttr<RegionParamAttr>();
      assert(Param);
      TmpRegions->push_back(new Rpl(new ParamRplElement(Param->getName())));*/
    }
  }

  void VisitBinAssign(BinaryOperator *E) {
    OS << "DEBUG:: >>>>>>>>>>VisitBinAssign<<<<<<<<<<<<<<<<<\n";
    E->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";

    TypeCheckerVisitor TCV(BR, Ctx, Mgr, AC, OS, RplAttrMap,
                           E, E->getLHS(), E->getRHS());

    assert(TmpRegionsVec->size() == 1);
    assert(TmpRegions->size() == 0);
    // FIXME slight memory leak
    TmpRegionsVec = TCV.getLHSRegionsVec();
    TmpRegions = TmpRegionsVec->back();
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
    //const FunctionDecl *FD = dyn_cast<FunctionDecl>(VD);
    //if (FD)
      //helperVisitFunctionDecl(Expr, FD);

    ///-//////////////////////////////////////////////
    /// 2. vd is a FieldDecl
    /// Type_vd <args> vd
    const FieldDecl* FieldD  = dyn_cast<FieldDecl>(VD);
    if (FieldD) {
      //StringRef S;
      if (IsBase) {
        /// Substitution
        substitute(FieldD);
      } else { //if (TypecheckAssignment) {
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
          #if 0 /// TODO implement capture properly!
          if (HasWriteSemantics && DerefNum > 0
              && Tmp->isFullySpecified() == false
              /* FIXME: && (*i)->/isaPointerType/ )*/) {
            /// emit capture error
            std::string Str = Tmp->toString();
            helperEmitInvalidAliasingModificationWarning(Expr, VD, Str);
          }
          #endif

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

      /// 2.3. Visit Base with read semantics, then restore write semantics
      bool SavedIsBase = IsBase; // probably not needed to save

      DerefNum = Expr->isArrow() ? 1 : 0;
      IsBase = true;
      Visit(Expr->getBase());

      /// Post visitation checking
      IsBase = SavedIsBase;
    } // end if FieldDecl
  } // end VisitMemberExpr


}; // end class StmtVisitor
