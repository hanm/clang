/// Find assignments and call Typechecking on them. Assignments include
/// * simple assignments: a = b
/// * complex assignments: a = b (where a and b are not scalars) TODO
/// * assignment of actuals to formals: f(a)
/// * return statements assigning expr to formal return type
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

  SymbolTable &SymT;

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
    SymbolTable &SymT,
    const FunctionDecl *Def,
    Stmt *S
    ) : BR(BR),
        Ctx(Ctx),
        Mgr(Mgr),
        AC(AC),
        OS(OS),
        SymT(SymT),
        Def(Def),
        FatalError(false), Type(0) {

    OS << "DEBUG:: ******** INVOKING AssignmentCheckerVisitor...\n";
    S->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    Visit(S);
    OS << "DEBUG:: ******** DONE INVOKING AssignmentCheckerVisitor ***\n";
  }
  /// Destructor
  ~AssignmentCheckerVisitor() {
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
    OS << "DEBUG:: GENERIC:: Visiting Stmt/Expr = \n";
    S->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    VisitChildren(S);
  }

  /// Declarations only. Implementation later to support mutual recursion
  /// with TypeChecker
  void VisitBinAssign(BinaryOperator *E);
  void VisitReturnStmt(ReturnStmt *Ret);
  void VisitCXXMemberCallExpr(CXXMemberCallExpr *Exp);

  void VisitDesignatedInitExpr(DesignatedInitExpr *Exp) {
    OS << "Designated INIT Expr!!\n";
    // TODO
  }

  void VisitCXXScalarValueInitExpr(CXXScalarValueInitExpr *Exp) {
    OS << "CXX Scalar Value INIT Expr!!\n";
    // TODO
  }

  void VisitDeclStmt(DeclStmt *S) {
    OS << "Decl Stmt INIT ?? (";
    S->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << ")\n";
    for(DeclGroupRef::const_iterator I = S->decl_begin(), E = S->decl_end();
        I != E; ++I) {
      if (VarDecl *VD = dyn_cast<VarDecl>(*I)) {
        if (VD->hasInit()) {
          helperTypecheckDeclWithInit(VD, VD->getInit());
        }
      }
    } // end for each declaration
  }

private:
  /// Private Methods
  bool typecheck(const ASaPType *LHSType, const ASaPType *RHSType) {
    assert(LHSType);
    if (RHSType && !RHSType->subtype(*LHSType) )
      return false;
    else
      return true;
  }
  bool typecheckParamAssignment(ParmVarDecl *Param, Expr *Arg);

  void helperTypecheckDeclWithInit(const VarDecl *VD, Expr *Init);

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
                                          const ASaPType *RHS,
                                          StringRef BugName) {

    std::string description_std = "The RHS type [";
    description_std.append(RHS ? RHS->toString(Ctx) : "");
    description_std.append("] is not a subtype of the LHS type [");
    description_std.append(LHS ? LHS->toString(Ctx) : "");
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

  void helperEmitInvalidArgToFunctionWarning(const Stmt *S,
                                                  const ASaPType *LHS,
                                                  const ASaPType *RHS) {
    StringRef BugName = "invalid argument to function call";
    helperEmitInvalidAssignmentWarning(S, LHS, RHS, BugName);
  }

  void helperEmitInvalidExplicitAssignmentWarning(const Stmt *S,
                                                  const ASaPType *LHS,
                                                  const ASaPType *RHS) {
    StringRef BugName = "invalid assignment";
    helperEmitInvalidAssignmentWarning(S, LHS, RHS, BugName);
  }

  void helperEmitInvalidReturnTypeWarning(const Stmt *S,
                                          const ASaPType *LHS,
                                          const ASaPType *RHS) {
    StringRef BugName = "invalid return type";
    helperEmitInvalidAssignmentWarning(S, LHS, RHS, BugName);
  }

  void helperEmitInvalidInitializationWarning(const Stmt *S,
                                              const ASaPType *LHS,
                                              const ASaPType *RHS) {
    StringRef BugName = "invalid initialization";
    helperEmitInvalidAssignmentWarning(S, LHS, RHS, BugName);
  }

}; // end class AssignmentCheckerVisitor

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

  SymbolTable &SymT;

  const FunctionDecl *Def;
  bool FatalError;

  /// true when visiting a base expression (e.g., B in B.f, or B->f)
  bool IsBase;
  /// count of number of dereferences on expression (values in [-1, 0, ...] )
  int DerefNum;

  ASaPType *Type;
  QualType RefQT;

  /// \brief substitute region parameters in Type with arguments.
  void memberSubstitute(const ValueDecl *D) {
    OS << "DEBUG:: in TypeBuilder::memberSubstitute:\n";
    OS << "DEBUG:: isBase = " << (IsBase ? "true" : "false") << "\n";
    OS << "DEBUG:: DerefNum = " << DerefNum << "\n";

    const ASaPType *T = SymT.getType(D);
    assert(T);
    bool NeedsCleanup = false;
    if (T->isFunctionType()) {
      NeedsCleanup = true;
      T = T->getReturnType();
    }
    OS << "DEBUG:: Type used for substitution = " << T->toString(Ctx) << "\n";

    QualType QT = T->getQT(DerefNum);

    const ParameterVector *ParamVec = SymT.getParameterVectorFromQualType(QT);
    // TODO support multiple Parameters
    const ParamRplElement *FromEl = ParamVec->getParamAt(0);
    assert(FromEl);

    const Rpl *ToRpl = T->getSubstArg(DerefNum);
    assert(ToRpl);
    OS << "DEBUG:: gonna substitute... " << FromEl->getName()
       << "<-" << ToRpl->toString() << "\n";

    if (FromEl->getName().compare(ToRpl->toString())) {
      OS <<" GO!!\n";
      assert(Type);
      Type->substitute(*FromEl, *ToRpl);
    }
    OS << "   DONE\n";
    if (NeedsCleanup) {
      //delete T;
    }
  }

  /// \brief collect the region arguments for a field
  void setType(const ValueDecl *D) {
    OS << "DEBUG:: in TypeBuilder::setType:\n";
    const ASaPType *T = SymT.getType(D);
    assert(T);

    assert(!Type);
    if (T->isFunctionType())
      Type = T->getReturnType(); // makes a copy
    else
      Type = new ASaPType(*T); // make a copy
    if (DerefNum == -1)
      Type->addrOf(RefQT);
    else {
      OS << "DEBUG :: calling ASaPType::deref(" << DerefNum << ")\n";
      Type->deref(DerefNum);
      OS << "DEBUG :: DONE calling ASaPType::deref\n";
    }
    OS << "DEBUG :: set TypeBuilder::Type = " << Type->toString(Ctx) << "\n";
  }

public:
  /// Constructor
  TypeBuilderVisitor (
    ento::BugReporter &BR,
    ASTContext &Ctx,
    AnalysisManager &Mgr,
    AnalysisDeclContext *AC,
    raw_ostream &OS,
    SymbolTable &SymT,
    const FunctionDecl *Def,
    Expr *E
    ) : BR(BR),
        Ctx(Ctx),
        Mgr(Mgr),
        AC(AC),
        OS(OS),
        SymT(SymT),
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
    OS << "DEBUG:: GENERIC:: Visiting Stmt/Expr = \n";
    S->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    VisitChildren(S);
  }

  void VisitUnaryAddrOf(UnaryOperator *Exp)  {
    assert(DerefNum>=0);
    DerefNum--;
    OS << "DEBUG:: Visit Unary: AddrOf (DerefNum=" << DerefNum << ") Type = ";
    Exp->getType().print(OS, Ctx.getPrintingPolicy());
    OS << "\n";

    RefQT = Exp->getType();
    assert(RefQT->isPointerType());

    Visit(Exp->getSubExpr());
    DerefNum++;
  }

  void VisitUnaryDeref(UnaryOperator *E) {
    DerefNum++;
    OS << "DEBUG:: Visit Unary: Deref (DerefNum=" << DerefNum << ")\n";
    Visit(E->getSubExpr());
    DerefNum--;
  }

  void VisitDeclRefExpr(DeclRefExpr *E) {
    OS << "DEBUG:: VisitDeclRefExpr --- whatever that is!: ";
    E->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    ValueDecl* VD = E->getDecl();
    assert(VD);
    setType(VD);
  }

  void VisitCXXThisExpr(CXXThisExpr *E) {
    OS << "DEBUG:: visiting 'this' expression\n";
    assert(E);
    //DerefNum = 0;
    if (!IsBase) {
      assert(!Type);
      // Add parameter as implicit argument
      CXXRecordDecl *RecDecl =
        const_cast<CXXRecordDecl*>(E->getBestDynamicClassType());
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

      //const RegionParamAttr *Param = RecDecl->getAttr<RegionParamAttr>();
      //assert(Param);

      //RplElement *El = RplElementMap[Param];
      //assert(El);
      //ParamRplElement *ParamEl = dyn_cast<ParamRplElement>(El);
      //assert(ParamEl);

      const ParameterVector *ParamVec = SymT.getParameterVector(RecDecl);
      QualType ThisQT = E->getType();

      RplVector RV(*ParamVec);

      OS << "DEBUG:: adding 'this' type : ";
      ThisQT.print(OS, Ctx.getPrintingPolicy());
      OS << "\n";
      Type = new ASaPType(ThisQT, &RV, 0, true);
      OS << "DEBUG:: type actually added: " << Type->toString(Ctx) << "\n";

      //TmpRegions->push_back(new Rpl(new ParamRplElement(Param->getName())));
    }
    OS << "DEBUG:: DONE visiting 'this' expression\n";
  }

  /*void VisitCastExpr(CastExpr *Exp) {
    OS << "DEBUG:: VisitCastExpr: ";
    Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    Visit(Exp->getSubExpr());
  }*/

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
    /// TODO: For Michael ;-)
    const FunctionDecl *FunD = dyn_cast<FunctionDecl>(VD);
    if (FunD) {
      if (IsBase)
        memberSubstitute(FunD);
      else
        setType(FunD);
    }

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
    } // end if FieldDecl
      /// 2.3. Visit Base with read semantics, then restore write semantics
      bool SavedIsBase = IsBase; // probably not necessary to save & restore
      int SavedDerefNum = DerefNum;
      DerefNum = Exp->isArrow() ? 1 : 0;
      IsBase = true;
      Visit(Exp->getBase());
      IsBase = SavedIsBase;
      DerefNum = SavedDerefNum;

  } // end VisitMemberExpr

  void helperBinAddSub(Expr *LHS, Expr* RHS) {
    Visit(LHS);
    ASaPType *T = stealType();
    Visit(RHS);
    if (Type)
      Type->join(T);
    else
      Type = T;
  }

  void VisitBinSub(BinaryOperator *Exp) {
    helperBinAddSub(Exp->getLHS(), Exp->getRHS());
  }

  void VisitBinAdd(BinaryOperator *Exp) {
    helperBinAddSub(Exp->getLHS(), Exp->getRHS());
  }

  void VisitBinAssign(BinaryOperator *Exp) {
    OS << "DEBUG:: >>>>>>>>>>VisitBinAssign<<<<<<<<<<<<<<<<<\n";
    Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";

    AssignmentCheckerVisitor ACV(BR, Ctx, Mgr, AC, OS, SymT, Def, Exp);
    assert(!Type);
    Type = ACV.stealType();
    assert(Type);
  }

  void VisitConditionalOperator(ConditionalOperator *Exp) {
    OS << "DEBUG:: @@@@@@@@@@@@VisitConditionalOp@@@@@@@@@@@@@@\n";
    Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    AssignmentCheckerVisitor ACV(BR, Ctx, Mgr, AC, OS,
                                 SymT, Def, Exp->getCond());
    FatalError |= ACV.encounteredFatalError();

    assert(!Type);
    OS << "DEBUG:: Visiting Cond LHS\n";
    Visit(Exp->getLHS());
    OS << "DEBUG:: DONE Visiting Cond LHS\n";
    ASaPType *LHSType = stealType();

    OS << "DEBUG:: Visiting Cond RHS\n";
    Visit(Exp->getRHS());
    OS << "DEBUG:: DONE Visiting Cond RHS\n";
    if (Type)
      Type->join(LHSType);
    else
      Type = LHSType;
    OS << "DEBUG:: Joining Cond LHS & RHS\n";

  }

  void VisitBinaryConditionalOperator(BinaryConditionalOperator *Exp) {
    OS << "DEBUG:: @@@@@@@@@@@@VisitConditionalOp@@@@@@@@@@@@@@\n";
    Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    // TODO?
  }

  void VisitCXXNewExpr(CXXNewExpr *Exp) {
  }

  void VisitCallExpr(CallExpr *Exp) {
    // Don't visit call arguments. Typechecking those is initiated
    // by the AssignmentCheckerVisitor.
    OS << "DEBUG:: VisitCXXNewExpr:";
    Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\nNOTHING TODO HERE!";
    return;
  }
  /*void VisitCXXMemberCallExpr(CXXMemberCallExpr *Exp) {
    // Don't visit call arguments. Typechecking those is initiated
    // by the AssignmentCheckerVisitor.
    OS << "DEBUG:: VisitCXXMemberCallExpr:";
    Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\nNOTHING TODO HERE!";
    return;
  }
  void VisitCXXOperatorCallExpr(CXXOperatorCallExpr *Exp) {
    // Don't visit call arguments. Typechecking those is initiated
    // by the AssignmentCheckerVisitor.
    OS << "DEBUG:: VisitCXXOperatorCallExpr:";
    Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\nNOTHING TODO HERE!";
    return;
  }*/

  void VisitReturnStmt(ReturnStmt *Ret) {
    assert(false);
    // Don't visit call arguments. Typechecking those is initiated
    // by the AssignmentCheckerVisitor.
    return;
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

    SymbolTable &SymT,

    const FunctionDecl *Def,
    Stmt *S)
      : ASaPStmtVisitorBase//<AssignmentDetectorVisitor>
                           (BR, Ctx, Mgr, AC, OS, SymT, Def, S),
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
  void VisitBinOp

  void VisitBinAssign(BinaryOperator *E) {
    OS << "DEBUG:: >>>>>>>>>> TYPECHECKING BinAssign<<<<<<<<<<<<<<<<<\n";
    E->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    TypeCheckerVisitor TCV(BR, Ctx, Mgr, AC, OS, SymT, Def,
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
  TypeBuilderVisitor TBVR(BR, Ctx, Mgr, AC, OS, SymT, Def, E->getRHS());
  TypeBuilderVisitor TBVL(BR, Ctx, Mgr, AC, OS, SymT, Def, E->getLHS());
  OS << "DEBUG:: Ran type builder on RHS & LHS\n";
  E->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  ASaPType *LHSType = TBVL.getType();
  ASaPType *RHSType = TBVR.getType();
  assert(LHSType);

  // allow RHSType to be NULL, e.g., we don't create ASaP Types for constants
  // because they don't have any interesting regions to typecheck.
  if (! typecheck(LHSType, RHSType)) {
    OS << "DEBUG:: invalid assignment: gonna emit an error\n";
    helperEmitInvalidExplicitAssignmentWarning(E, LHSType, RHSType);
    FatalError = true;
  }

  // The type of the assignment is the type of the LHS. Set it in case
  // AssignmentChecker was called recursively by a TypeBuilderVisitor
  delete Type;
  Type = TBVL.stealType();
  assert(Type);

  OS << "DEBUG:: >>>>>>>>>> DONE TYPECHECKING BinAssign<<<<<<<<<<<<<<<<<\n";
}

void AssignmentCheckerVisitor::VisitReturnStmt(ReturnStmt *Ret) {
  Expr *RetExp = Ret->getRetValue();
  OS << "DEBUG:: Visiting ReturnStmt: ";
  RetExp->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";

  TypeBuilderVisitor TBVR(BR, Ctx, Mgr, AC, OS, SymT, Def, RetExp);
  const ASaPType *LHSType = SymT.getType(Def);
  if (LHSType)
    LHSType = LHSType->getReturnType(); // FIXME memory leak
  assert(LHSType);
  ASaPType *RHSType = TBVR.getType();
  if (! typecheck(LHSType, RHSType)) {
    OS << "DEBUG:: invalid assignment: gonna emit an error\n";
    helperEmitInvalidReturnTypeWarning(Ret, LHSType, RHSType);
    FatalError = true;
  }
  delete LHSType;
  delete Type;
  Type = 0;
}

void AssignmentCheckerVisitor::helperTypecheckDeclWithInit(
                                                           const VarDecl *VD,
                                                           Expr *Init) {
  TypeBuilderVisitor TBVR(BR, Ctx, Mgr, AC, OS, SymT, Def, Init);
  const ASaPType *LHSType = SymT.getType(VD);
  ASaPType *RHSType = TBVR.getType();
  if (! typecheck(LHSType, RHSType)) {
    OS << "DEBUG:: invalid assignment: gonna emit an error\n";
    //  Fixme pass VS as arg instead of Init
    helperEmitInvalidInitializationWarning(Init, LHSType, RHSType);
    FatalError = true;
  }
}

bool AssignmentCheckerVisitor::typecheckParamAssignment(ParmVarDecl *Param,
                                                        Expr *Arg) {
    TypeBuilderVisitor TBVR(BR, Ctx, Mgr, AC, OS, SymT, Def, Arg);
    const ASaPType *LHSType = SymT.getType(Param);
    ASaPType *RHSType = TBVR.getType();
    if (! typecheck(LHSType, RHSType)) {
      OS << "DEBUG:: invalid argument to parameter assignment: "
        << "gonna emit an error\n";
      //  Fixme pass VS as arg instead of Init
      helperEmitInvalidArgToFunctionWarning(Arg, LHSType, RHSType);
      FatalError = true;
      return false;
    }
    return true;
}

void AssignmentCheckerVisitor::VisitCXXMemberCallExpr(CXXMemberCallExpr *Exp) {
  CXXMethodDecl *CalleeDecl = Exp->getMethodDecl();
  assert(CalleeDecl);
  ExprIterator ArgI, ArgE;
  FunctionDecl::param_iterator ParamI, ParamE;
  assert(CalleeDecl->getNumParams() == Exp->getNumArgs());

  for(ArgI = Exp->arg_begin(), ArgE = Exp->arg_end(),
      ParamI = CalleeDecl->param_begin(), ParamE = CalleeDecl->param_end();
      ArgI != ArgE && ParamI != ParamE; ++ArgI, ++ParamI) {
    OS << "DEBUG:: " << "\n";
    Expr *ArgExpr = *ArgI;
    ParmVarDecl *ParamDecl = *ParamI;
    typecheckParamAssignment(ParamDecl, ArgExpr);
  }
}

/*  void VisitCallExpr(CallExpr *Exp) {
    //TODO
  }
  void VisitCXXOperatorCallExpr(CXXOperatorCallExpr *Exp) {
    //TODO
  }
*/
