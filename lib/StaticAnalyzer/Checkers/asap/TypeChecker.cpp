//=== TypeChecker.cpp - Safe Parallelism checker -----*- C++ -*------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This files defines the Type Checker pass of the Safe Parallelism
// checker, which tries to prove the safety of parallelism given region
// and effect annotations.
//
//===----------------------------------------------------------------===//

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
    Def->print(OS, Ctx.getPrintingPolicy());
    //S->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    if (const CXXConstructorDecl *D = dyn_cast<CXXConstructorDecl>(Def)) {
      // Also visit initialization lists
      helperVisitCXXConstructorDecl(D);
    }
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
  //void VisitCallExpr(CallExpr *Exp);

  void VisitCXXMemberCallExpr(CXXMemberCallExpr *Exp) {
    typecheckCallExpr(Exp);
    VisitChildren(Exp);
  }

  void VisitCXXOperatorCallExpr(CXXOperatorCallExpr *Exp) {
    typecheckCallExpr(Exp);
    VisitChildren(Exp);
  }

  void VisitMemberExpr(MemberExpr *Exp) {
    OS << "DEBUG:: VisitMemberExpr: ";
    Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    VisitChildren(Exp);
  }

  void VisitDesignatedInitExpr(DesignatedInitExpr *Exp) {
    OS << "Designated INIT Expr!!\n";
    // TODO?
  }

  void VisitCXXScalarValueInitExpr(CXXScalarValueInitExpr *Exp) {
    OS << "CXX Scalar Value INIT Expr!!\n";
    // TODO?
  }

  void VisitInitListExpr(InitListExpr *Exp) {
    OS << "DEBUG:: VisitInitListExpr: ";
    Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    // TODO?
  }

  void VisitDeclStmt(DeclStmt *S) {
    OS << "Decl Stmt INIT ?? (";
    S->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << ")\n";
    for(DeclGroupRef::const_iterator I = S->decl_begin(), E = S->decl_end();
        I != E; ++I) {
      if (VarDecl *VD = dyn_cast<VarDecl>(*I)) {
        if (VD->hasInit()) {
          Expr *Init = VD->getInit();

          OS << "DEBUG:: TypecheckDeclWithInit: Decl = ";
          VD->print(OS,  Ctx.getPrintingPolicy());
          OS << "\n Init Expr = ";
          Init->printPretty(OS, 0, Ctx.getPrintingPolicy());
          OS << "\n";
          Init->dump(OS, BR.getSourceManager());

          OS << "DEBUG:: IsDirectInit = "
             << (VD->isDirectInit()?"true":"false")
             << "\n";
          OS << "DEBUG:: Init Style: ";
          switch(VD->getInitStyle()) {
          case VarDecl::CInit:
            OS << "CInit\n";
            helperTypecheckDeclWithInit(VD, VD->getInit());
            break;
          case VarDecl::ListInit:
            OS << "ListInit\n";
            // Intentonally falling through (i.e., no break stmt).
          case VarDecl::CallInit:
            OS << "CallInit\n";
            CXXConstructExpr *Exp = dyn_cast<CXXConstructExpr>(VD->getInit());
            assert(Exp);
            typecheckCXXConstructExpr(VD, Exp);
            break;
          }
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

  // Implemented out-of-line because it is calling TypeBuilder which is
  // defined later.
  bool typecheckParamAssignment(ParmVarDecl *Param, Expr *Arg,
                                SubstitutionVector *SubV = 0);

  void typecheckCall(FunctionDecl *CalleeDecl,
                     ExprIterator ArgI,
                     ExprIterator ArgE,
                     SubstitutionVector *SubV = 0) {
    assert(CalleeDecl);
    FunctionDecl::param_iterator ParamI, ParamE;
    for(ParamI = CalleeDecl->param_begin(), ParamE = CalleeDecl->param_end();
        ArgI != ArgE && ParamI != ParamE; ++ArgI, ++ParamI) {
      OS << "DEBUG:: " << "\n";
      Expr *ArgExpr = *ArgI;
      ParmVarDecl *ParamDecl = *ParamI;
      typecheckParamAssignment(ParamDecl, ArgExpr, SubV);
    }

  }

  void typecheckCallExpr(CallExpr *Exp);

  void typecheckCXXConstructExpr(VarDecl *D, CXXConstructExpr *Exp) {
    CXXConstructorDecl *ConstrDecl =  Exp->getConstructor();
    DeclContext *ClassDeclContext = ConstrDecl->getDeclContext();
    assert(ClassDeclContext);
    RecordDecl *ClassDecl = dyn_cast<RecordDecl>(ClassDeclContext);
    assert(ClassDecl);
    // Set up Substitution Vector
    SubstitutionVector SubV, *SubPtr = 0;
    Substitution S;
    const ParameterVector *PV = SymT.getParameterVector(ClassDecl);
    if (PV) {
      assert(PV->size() == 1); // until we support multiple region params
      const ParamRplElement *ParamEl = PV->getParamAt(0);
      const ASaPType *T = SymT.getType(D);
      if (T) {
        const Rpl *R = T->getSubstArg();
        S.set(ParamEl, R);
        OS << "DEBUG:: ConstructExpr Substitution = " << S.toString() << "\n";
        SubV.push_back(&S);
        SubPtr = &SubV;
      }
    }
    typecheckCall(ConstrDecl, Exp->arg_begin(), Exp->arg_end(), SubPtr);
  }

  void helperTypecheckDeclWithInit(const ValueDecl *VD, Expr *Init);

  /// \brief Issues Warning: '<str>' <bugName> on Declaration
  void helperEmitDeclarationWarning(const Decl *D,
                                    const StringRef &Str,
                                    std::string BugName,
                                    bool AddQuotes = true) {

    std::string Description = "";
    if (AddQuotes)
      Description.append("'");
    Description.append(Str);
    if (AddQuotes)
      Description.append("' ");
    else
      Description.append(" ");
    Description.append(BugName);
    StringRef BugCategory = "Safe Parallelism";
    StringRef BugStr = Description;

    PathDiagnosticLocation VDLoc(D->getLocation(), BR.getSourceManager());
    BR.EmitBasicReport(D, BugName, BugCategory,
                       BugStr, VDLoc, D->getSourceRange());

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

  void helperEmitUnsupportedConstructorInitializer(const CXXConstructorDecl *D) {
    StringRef BugName = "unsupported constructor initializer."
      " Please file feature support request.";
    helperEmitDeclarationWarning(D, "", BugName, false);
  }
  /// \brief Typecheck Constructor initialization lists
  void helperVisitCXXConstructorDecl(const CXXConstructorDecl *D) {
    CXXConstructorDecl::init_const_iterator
      I = D->init_begin(),
      E = D->init_end();
    for(; I != E; ++I) {
      CXXCtorInitializer *Init = *I;
      if (Init->isMemberInitializer()) {
        helperTypecheckDeclWithInit(Init->getMember(), Init->getInit());
      } else {
        helperEmitUnsupportedConstructorInitializer(D);
      }
    }
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

  // Private Functions
  /// \brief substitute region parameters in Type with arguments.
  void memberSubstitute(const ValueDecl *D) {
    OS << "DEBUG:: in TypeBuilder::memberSubstitute:";
    D->print(OS, Ctx.getPrintingPolicy());
    OS << "\nDEBUG:: isBase = " << (IsBase ? "true" : "false") << "\n";
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
      Substitution S(FromEl, ToRpl);
      Type->substitute(S);
    }
    OS << "   DONE\n";
    if (NeedsCleanup) {
      //delete T;
    }
  }

  /// \brief collect the region arguments for a field
  void setType(const ValueDecl *D) {
    OS << "DEBUG:: in TypeBuilder::setType: ";
    D->print(OS, Ctx.getPrintingPolicy());
    OS << "\n Decl pointer address:" << D;
    OS << "\n";
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
    if (IsBase)
      memberSubstitute(VD);
    else
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
    //VD->print(OS, Ctx.getPrintingPolicy());
    //OS << "\n";
    if (IsBase)
      memberSubstitute(VD);
    else
      setType(VD);

    // Visit Base with read semantics, then restore write semantics
    bool SavedIsBase = IsBase;
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
    OS << "DEBUG:: VisitCallExpr:";
    Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\nNOTHING TODO HERE!\n";
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
/////////////////////////////////////////////////////////////////////////////
//// BaseTypeBuilderVisitor
class BaseTypeBuilderVisitor
    : public StmtVisitor<BaseTypeBuilderVisitor, void> {

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
  QualType RefQT;

public:
  /// Constructor
  BaseTypeBuilderVisitor (
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
        Type(0) {

    OS << "DEBUG:: ******** INVOKING BaseTypeBuilderVisitor...\n";
    E->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";

    Visit(E);
    OS << "DEBUG:: ******** DONE WITH BaseTypeBuilderVisitor...\n";
  }

  /// Destructor
  virtual ~BaseTypeBuilderVisitor() {
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

  void VisitMemberExpr(MemberExpr *Exp) {
    OS << "DEBUG:: VisitMemberExpr: ";
    Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
    OS << "\n";
    TypeBuilderVisitor TBV(BR, Ctx, Mgr, AC, OS, SymT, Def, Exp->getBase());
    Type = TBV.stealType();
  }
}; // end class BaseTypeBuilderVisitor
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
                                                    const ValueDecl *VD,
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

bool AssignmentCheckerVisitor::
      typecheckParamAssignment(ParmVarDecl *Param,
                               Expr *Arg, SubstitutionVector *SubV) {
    bool Result = true;
    OS << "DEBUG:: typeckeckParamAssignment\n";
    if (SubV) {
      OS << "SubstitutionVector Size = " << SubV->size() << "\n";
      OS << "SubVec: " << SubV->toString();
    }
    TypeBuilderVisitor TBVR(BR, Ctx, Mgr, AC, OS, SymT, Def, Arg);
    const ASaPType *LHSType = SymT.getType(Param);
    ASaPType *LHSTypeMod = 0;
    if (SubV && LHSType) {
      OS << "DEBUG:: gonna perform substitution\n";
      LHSTypeMod = new ASaPType(*LHSType);
      LHSTypeMod->substitute(*SubV);
      LHSType = LHSTypeMod;
      OS << "DEBUG:: DONE perform substitution\n";
    }
    ASaPType *RHSType = TBVR.getType();
    if (! typecheck(LHSType, RHSType)) {
      OS << "DEBUG:: invalid argument to parameter assignment: "
        << "gonna emit an error\n";
      //  Fixme pass VS as arg instead of Init
      helperEmitInvalidArgToFunctionWarning(Arg, LHSType, RHSType);
      FatalError = true;
      Result = false;
    }
    delete LHSTypeMod;
    OS << "DEBUG:: DONE with typeckeckParamAssignment\n";
    return Result;
}
void AssignmentCheckerVisitor::typecheckCallExpr(CallExpr *Exp) {
  Decl *D = Exp->getCalleeDecl();
  assert(D);
  FunctionDecl *FD = dyn_cast<FunctionDecl>(D);
  assert(FD);
  // Set up Substitution Vector
  DeclContext *DC = FD->getDeclContext();
  assert(DC);
  RecordDecl *ClassDecl = dyn_cast<RecordDecl>(DC);
  assert(ClassDecl);
  const ParameterVector *PV = SymT.getParameterVector(ClassDecl);

  SubstitutionVector SubV, *SubPtr = 0;
  Substitution S;
  BaseTypeBuilderVisitor  TBV(BR, Ctx, Mgr, AC, OS,
                              SymT, Def, Exp->getCallee());
  if (PV) {
    assert(PV->size() == 1); // until we support multiple region params
    const ParamRplElement *ParamEl = PV->getParamAt(0);

    ASaPType *T = TBV.getType();
    if (T) {
      const Rpl *R = T->getSubstArg();
      S.set(ParamEl, R);
      OS << "DEBUG:: typecheckCallExpr Substitution = "
         << S.toString() << "\n";
      SubV.push_back(&S);
      SubPtr = &SubV;
    }
  }
  typecheckCall(FD, Exp->arg_begin(), Exp->arg_end(), SubPtr);
}
