// TODO
// (b ? x : y) = (c ? w : z)
// (b = exp1 ? x : y) = (c = exp2 ? w :z)
// x = x + q.v // don't gather regions from q.v

/// Find assignments and call Typechecking on them. Assignments include
/// * simple assignments: a = b
/// * complex assignments: a = b (where a and b are compound objects) TODO
/// * assignment of actuals to formals: f(a) TODO
/// * return statements assigning expr to formal return type TODO
/// * ...stay tuned, more to come
class AssignmentCheckerVisitor
    : public StmtVisitor<AssignmentCheckerVisitor> {

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

  ASaPType *Type;

public:
  /// Constructor
  AssignmentCheckerVisitor (
    ento::BugReporter &BR,
    ASTContext &Ctx,
    AnalysisManager &Mgr,
    AnalysisDeclContext *AC,
    raw_ostream &OS,
    RplElementAttrMapTy &RplElementMap,
    RplAttrMapTy &RplMap,
    ASaPTypeDeclMapTy &ASaPTypeDeclMap,
    EffectSummaryMapTy &EffectSummaryMap,
    const FunctionDecl *Def,
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
        FatalError(false), Type(0) {

    //ResultType = getReturnType(Def);
    //Effect::EffectVector *EffectSummary = EffectSummaryMap[Def];
    //assert(EffectSummary);
    OS << "DEBUG:: ******** INVOKING AssignmentCheckerVisitor...\n";
    S->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    Visit(S);
    OS << "DEBUG:: ******** DONE INVOKING AssignmentCheckerVisitor ***\n";
  }
  /// Destructor
  ~AssignmentCheckerVisitor() {
    if (Type)
      delete Type;
  }

  /// Getters
  inline bool encounteredFatalError() { return FatalError; }

  ASaPType *stealType() {
    ASaPType *Result = Type;
    Type = 0;
    return Result;
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

  /// Declarations only. Implementation later to support mutual recursion
  /// with TypeChecker
  void VisitBinAssign(BinaryOperator *E);
  void VisitReturnStmt(ReturnStmt *Ret);

private:
  /// private helper methods
  ASaPType *getReturnType(const FunctionDecl *Def) {
    ASaPType *T = ASaPTypeDeclMap[Def];
    assert(T);
    return T;
  }
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

  void helperEmitInvalidAssignmentWarning(const Stmt *S,
                                          const ASaPType *LHS,
                                          const ASaPType *RHS) {
    StringRef BugName = "invalid assignment";

    std::string description_std = "The RHS type [";
    description_std.append(RHS ? RHS->toString() : "");
    description_std.append("] is not a subtype of the LHS type [");
    description_std.append(LHS ? LHS->toString() : "");
    description_std.append("] ");
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
  }


}; // end class

///-///////////////////////////////////////////////////////////////////////////
/// Stmt Visitor Classes

class TypeBuilderVisitor
    : public StmtVisitor<TypeBuilderVisitor, void> {

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

  /// true when visiting a base expression (e.g., B in B.f, or B->f)
  bool IsBase;
  /// count of number of dereferences on expression (values in [-1, 0, ...] )
  int DerefNum;

  //Effect::EffectVector &EffectSummary;
  ASaPType *Type;

  /// \brief substitute region parameters in TmpT with arguments.
  void memberSubstitute(const FieldDecl *FieldD) {

    OS << "DEBUG:: isBase = " << (IsBase ? "true" : "false") << "\n";
    OS << "DEBUG:: DerefNum = " << DerefNum << "\n";
    ASaPType *T = ASaPTypeDeclMap[FieldD];
    assert(T);
    OS << "op\n";
    //ASaPType *T1 = T->getInRpl(DerefNum);
    //assert(T1);
    QualType QT = T->getQT(DerefNum);
    OS << "ep\n";

    const RegionParamAttr* RPA = getRegionParamAttr(QT.getTypePtr());
    assert(RPA);
    OS<< "aloha\n";
    // TODO support multiple Parameters
    RplElement *RplEl = RplElementMap[RPA];
    assert(RplEl);
    const ParamRplElement *FromEl = dyn_cast<ParamRplElement>(RplEl);
    assert(FromEl);
    OS << "you minky\n";

    const Rpl *ToRpl = T->getInRpl(DerefNum);
    assert(ToRpl);
    OS << "DEBUG:: gonna substitute...\n";

    if (FromEl->getName().compare(ToRpl->toString())) {
      // if (from != to) then substitute
      OS <<" GO!!\n";
      assert(Type);
      Type->substitute(*FromEl, *ToRpl);
    }
    OS << "   DONE\n";
  }

  /// \brief collect the region arguments for a field
  void setType(const ValueDecl *D) {
    ASaPType *T = ASaPTypeDeclMap[D];
    assert(T);

    assert(!Type);
    Type = new ASaPType(*T); // make a copy
    OS << "DEBUG :: calling ASaPType::deref(" << DerefNum << ")\n";
    //Type->deref(DerefNum);
    OS << "DEBUG :: DONE calling ASaPType::deref\n";
  }

public:
  /// Constructor
  TypeBuilderVisitor (
    ento::BugReporter &BR,
    ASTContext &Ctx,
    AnalysisManager &Mgr,
    AnalysisDeclContext *AC,
    raw_ostream &OS,
    RplElementAttrMapTy &RplElementMap,
    RplAttrMapTy &RplMap,
    ASaPTypeDeclMapTy &ASaPTypeDeclMap,
    EffectSummaryMapTy &EffectSummaryMap,
    const FunctionDecl *Def,
    Expr *E
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
        IsBase(false),
        DerefNum(0),
        Type(0) {

    OS << "DEBUG:: ******** INVOKING TypeBuilderVisitor...\n";
    E->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";

    Visit(E);
    OS << "DEBUG:: ******** DONE WITH TypeBuilderVisitor...\n";
  }

  /// Destructor
  virtual ~TypeBuilderVisitor() {
    if (Type)
      delete Type;
  }

  /// Getters
  inline bool encounteredFatalError() { return FatalError; }

  inline ASaPType *getType() { return Type; }

  ASaPType *stealType() {
    ASaPType *Result = Type;
    Type = 0;
    return Result;
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
    ValueDecl* VD = E->getDecl();
    assert(VD);
    setType(VD);
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
    assert(E);
    DerefNum = 0;
    //if (TmpRegions && TmpRegions->empty() && !E->isImplicit()) {
    if (!IsBase) { // this condition should be equivalent to the above
      assert(!Type);
      // Add parameter as implicit argument
      CXXRecordDecl *RecDecl = const_cast<CXXRecordDecl*>(E->
                                                     getBestDynamicClassType());
      assert(RecDecl);

      /* Keeping this code below as an example of how to add nodes to the AST
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

        /// also add it to RplMap
        RplMap[Arg] = new Rpl(new ParamRplElement(Param->getName()));
      }
      RegionArgAttr* Arg = RecDecl->getAttr<RegionArgAttr>();
      assert(Arg);
      Rpl *Tmp = RplMap[Arg];
      assert(Tmp);
      TmpRegions->push_back(new Rpl(Tmp));*/
      const RegionParamAttr *Param = RecDecl->getAttr<RegionParamAttr>();
      assert(Param);
      QualType ThisQT; // FIXME

      RplElement *El = RplElementMap[Param];
      assert(El);
      ParamRplElement *ParamEl = dyn_cast<ParamRplElement>(El);
      assert(ParamEl);

      Rpl R(*ParamEl);
      RplVector RV(R);

      Type = new ASaPType(ThisQT, &RV, 0);
      //TmpRegions->push_back(new Rpl(new ParamRplElement(Param->getName())));
    }
    OS << "DEBUG:: DONE visiting 'this' expression\n";
  }

  void VisitMemberExpr(MemberExpr *Exp) {
    OS << "DEBUG:: VisitMemberExpr: ";
    Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
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
    ValueDecl* VD = Exp->getMemberDecl();
    VD->print(OS, Ctx.getPrintingPolicy());
    OS << "\n";

    /// 1. VD is a FunctionDecl
    /// TODO
    //const FunctionDecl *FD = dyn_cast<FunctionDecl>(VD);
    //if (FD)
      //helperVisitFunctionDecl(Expr, FD);

    ///-//////////////////////////////////////////////
    /// 2. vd is a FieldDecl
    /// Type_vd <args> vd
    const FieldDecl* FieldD  = dyn_cast<FieldDecl>(VD);
    if (FieldD) {
      //StringRef S;
      if (IsBase)
        memberSubstitute(FieldD);
      else // not IsBase --> HEAD
        setType(FieldD);

      /// 2.3. Visit Base with read semantics, then restore write semantics
      bool SavedIsBase = IsBase; // probably not needed to save

      DerefNum = Exp->isArrow() ? 1 : 0;
      IsBase = true;
      Visit(Exp->getBase());
      IsBase = SavedIsBase;
    } // end if FieldDecl
  } // end VisitMemberExpr

  void VisitBinAssign(BinaryOperator *E) {
    OS << "DEBUG:: >>>>>>>>>>VisitBinAssign<<<<<<<<<<<<<<<<<\n";
    E->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";

    AssignmentCheckerVisitor ACV(BR, Ctx, Mgr, AC, OS, RplElementMap,
                                RplMap, ASaPTypeDeclMap, EffectSummaryMap,
                                Def, E);
    assert(!Type);
    Type = ACV.stealType();
    assert(Type);
  }

  void VisitConditionalOperator(ConditionalOperator *Exp) {
    OS << "DEBUG:: @@@@@@@@@@@@VisitConditionalOp@@@@@@@@@@@@@@\n";
    Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    AssignmentCheckerVisitor ACV(BR, Ctx, Mgr, AC, OS,
                                RplElementMap, RplMap,
                                ASaPTypeDeclMap, EffectSummaryMap,
                                Def, Exp->getCond());
    FatalError |= ACV.encounteredFatalError();

    assert(!Type);
    Visit(Exp->getLHS());
    ASaPType *LHSType = stealType();

    Visit(Exp->getRHS());
    Type->join(LHSType);

  }

  void VisitBinaryConditionalOperator(BinaryConditionalOperator *Exp) {
    OS << "DEBUG:: @@@@@@@@@@@@VisitConditionalOp@@@@@@@@@@@@@@\n";
    Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    // TODO?
  }

  void VisitReturnStmt(ReturnStmt *Ret) {
    //Expr *RHS = Ret->getRetValue();
    /// TODO
  }

  void VisitCXXMemberCallExpr(CXXMemberCallExpr *Exp) {
    //TODO
    CXXMethodDecl *CalleeDecl = Exp->getMethodDecl();
    assert(CalleeDecl);
    ExprIterator ArgI, ArgE;
    FunctionDecl::param_iterator ParamI, ParamE;
    /// FIXME What about default arguments? Is this assertion too conservative?
    assert(CalleeDecl->getNumParams() == Exp->getNumArgs());

    for(ArgI = Exp->arg_begin(), ArgE = Exp->arg_end(),
        ParamI = CalleeDecl->param_begin(), ParamE = CalleeDecl->param_end();
         ArgI != ArgE && ParamI != ParamE; ++ArgI, ++ParamI) {
      OS << "DEBUG:: " << "\n";
      /// Typecheck implicit assignment
      /// Note: we don't only need to check LValues, as &x may
      /// not be an LValue but it may carry region information.

      /// TODO finish implementing this!
      ///Rpl::RplVector ParamRegs = getRegions(*ParamI);
      /// get types Visit(*ArgI);
    }

  }
  void VisitCallExpr(CallExpr *Exp) {
    //TODO
  }
  void VisitCXXOperatorCallExpr(CXXOperatorCallExpr *Exp) {
    //TODO
  }


}; // end class TypeBuilderVisitor

/// Find assignments and call Typechecking on them. Assignments include
/// * simple assignments: a = b
/// * complex assignments: a = b (where a and b are compound objects)
/// * assignment of actuals to formals: f(a)
/// * ...stay tuned, more to come
#if 0
class AssignmentDetectorVisitor :
    public ASaPStmtVisitorBase {
  private:
  /// Fields
  ASaPType *Type;

  public:
  /// Constructor
  AssignmentDetectorVisitor(
    ento::BugReporter &BR,
    ASTContext &Ctx,
    AnalysisManager &Mgr,
    AnalysisDeclContext *AC,
    raw_ostream &OS,

    RplElementAttrMapTy &RplElementMap,
    RplAttrMapTy &RplMap,
    ASaPTypeDeclMapTy &ASaPTypeDeclMap,
    EffectSummaryMapTy &EffectSummaryMap,

    const FunctionDecl *Def,
    Stmt *S)
      : ASaPStmtVisitorBase//<AssignmentDetectorVisitor>
                           (BR, Ctx, Mgr, AC, OS, RplElementMap, RplMap,
                            ASaPTypeDeclMap, EffectSummaryMap, Def, S),
        Type(0) {
    Visit(S);
  }
  /// Destructor
  ~AssignmentDetectorVisitor() {
    if (Type)
      delete Type;
  }
  /*void VisitStmt(Stmt *S) {
    OS << "DEBUG:: Where is my Visitor now??\n";
    VisitChildren(S);
  }*/
  ASaPType *stealType() {
    ASaPType *Result = Type;
    Type = 0;

  }
  /// Visitors
  void VisitBinAssign(BinaryOperator *E) {
    OS << "DEBUG:: >>>>>>>>>> TYPECHECKING BinAssign<<<<<<<<<<<<<<<<<\n";
    E->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    TypeCheckerVisitor TCV(BR, Ctx, Mgr, AC, OS, RplElementMap, RplMap,
                           ASaPTypeDeclMap, EffectSummaryMap, Def,
                           E, E->getLHS(), E->getRHS());
  }

}; // end class AssignmentDetectorVisitor
#endif

///-/////////////////////////////////////////////////////////////////////
/// Implementation of AssignmentCheckerVisitor

void AssignmentCheckerVisitor::VisitBinAssign(BinaryOperator *E) {
  OS << "DEBUG:: >>>>>>>>>> TYPECHECKING BinAssign<<<<<<<<<<<<<<<<<\n";
  E->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  TypeBuilderVisitor TBVR(BR, Ctx, Mgr, AC, OS, RplElementMap, RplMap,
                          ASaPTypeDeclMap, EffectSummaryMap, Def,
                          E->getRHS());
  TypeBuilderVisitor TBVL(BR, Ctx, Mgr, AC, OS, RplElementMap, RplMap,
                          ASaPTypeDeclMap, EffectSummaryMap, Def,
                          E->getLHS());
  OS << "DEBUG:: Ran type builder on RHS & LHS\n";
  E->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  ASaPType *LHSType = TBVL.getType();
  ASaPType *RHSType = TBVR.getType();
  assert(LHSType);
  assert(RHSType);

  if ( !RHSType->subtype(*LHSType) ) {
    OS << "DEBUG:: invalid assignment: gonna emit an error\n";
    helperEmitInvalidAssignmentWarning(E, LHSType, RHSType);
    FatalError = true;
  }
  if (Type)
    delete Type;
  Type = TBVR.stealType();
  assert(Type);

  OS << "DEBUG:: >>>>>>>>>> DONE TYPECHECKING BinAssign<<<<<<<<<<<<<<<<<\n";
}

void AssignmentCheckerVisitor::VisitReturnStmt(ReturnStmt *Ret) {
    //Expr *RV = Ret->getRetValue();
    /// TODO ASaPType AT = GetASaPType(RV);
    ///ASaPType *RetType = ASaPTypeDeclMap[Def];
}

