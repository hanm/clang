//=== StaticVariableAccessChecker.cpp - Static variable checker -----*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This files defines StaticVariableAccess checker, a checker that checks
// access (read, write) of static variables.
//
//===----------------------------------------------------------------------===//

#include "ClangSACheckers.h"
#include "clang/AST/StmtVisitor.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/CheckerManager.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace ento;

namespace clang {
  /// ExprLValueBaseVarEvaluator - This class visits an expression
  /// to determine the base variable of an lvalue expression.
  ///
  /// The base variable is defined as the variable the points to the
  /// block of memory in which the the lvalue has its expression.
  ///
  /// For example:
  ///    - the base variable of c->d is c
  ///    - the base variable of a[i] is a
  ///    - the base variable of *(p + 3) is p
  ///    - the base variables of (b ? x : y) are {x,y}
  ///    - etc.
  class ExprLValueBaseVarEvaluator
    : public StmtVisitor<ExprLValueBaseVarEvaluator, void> {

  public:
    typedef llvm::SmallPtrSet<ValueDecl*,4> ResultType;
    ResultType BaseVariables;

    // Constructor
    ExprLValueBaseVarEvaluator(Expr* LValueExpr) {
       Visit(LValueExpr);
    }

    ResultType getBaseVariables() const { return BaseVariables; }

    void VisitDeclRefExpr(DeclRefExpr* E) {
      BaseVariables.insert(E->getDecl());
    }

    // Visit Base.Member or Base->Member expression
    void VisitMemberExpr(MemberExpr* E) {
      // We need to check if the member is a static variable; in which
      // case, it is the member expression that form the base
      // variable of the lvalue expression.
      ValueDecl* MD = E->getMemberDecl();

      if (FunctionDecl* FD = dyn_cast<FunctionDecl>(MD)) {
        // This is a member function, i.e. taking the address of the
        // member function, not calling it. It can only be a static
        // member function.
        CXXMethodDecl* Method = dyn_cast<CXXMethodDecl>(FD);
        assert(Method->isStatic());
      }
      else {
        if (VarDecl* VD = dyn_cast<VarDecl>(MD)) {
          // We have to anaylyze variables separately because static local
          // variables does not have any linkage.
          if (!VD->hasLocalStorage()) {
            BaseVariables.insert(MD);
          }
        }
        else if (isa<FieldDecl>(MD)) {
          // FieldDecl represents non-static data members of classes.
          Visit(E->getBase());
        }
        else {
          llvm_unreachable("Unknown ValueDecl type.");
        }
      }
    }

    void VisitArraySubscriptExpr(ArraySubscriptExpr* E)  {
      Visit(E->getBase());
    }

    void VisitCallExpr(CallExpr* E) {}

    void VisitCXXOperatorCallExpr(CXXOperatorCallExpr *E) {
      switch (E->getOperator()) {
      case OO_New:                  break;
      case OO_Delete:               break;
      case OO_Array_New:            break;
      case OO_Array_Delete:         break;
      case OO_Plus:                 break;
      case OO_Minus:                break;
      case OO_Star:                 if (E->getNumArgs() == 1) Visit(E->getArg(0)); break;
      case OO_Slash:                break;
      case OO_Percent:              break;
      case OO_Caret:                break;
      case OO_Amp:                  Visit(E->getArg(0)); break;
      case OO_Pipe:                 break;
      case OO_Tilde:                break;
      case OO_Exclaim:              break;
      case OO_Equal:                break;
      case OO_Less:                 break;
      case OO_Greater:              break;
      case OO_PlusEqual:            break;
      case OO_MinusEqual:           break;
      case OO_StarEqual:            break;
      case OO_SlashEqual:           break;
      case OO_PercentEqual:         break;
      case OO_CaretEqual:           break;
      case OO_AmpEqual:             break;
      case OO_PipeEqual:            break;
      case OO_LessLess:             break;
      case OO_GreaterGreater:       break;
      case OO_LessLessEqual:        break;
      case OO_GreaterGreaterEqual:  break;
      case OO_EqualEqual:           break;
      case OO_ExclaimEqual:         break;
      case OO_LessEqual:            break;
      case OO_GreaterEqual:         break;
      case OO_AmpAmp:               break;
      case OO_PipePipe:             break;
      case OO_PlusPlus:             Visit(E->getArg(0)); break;
      case OO_MinusMinus:           Visit(E->getArg(0)); break;
      case OO_Comma:                Visit(E->getArg(1)); break;
      case OO_ArrowStar:            Visit(E->getArg(0)); break;
      case OO_Arrow:                Visit(E->getArg(0)); break;
      case OO_Call:                 break;
      case OO_Subscript:            Visit(E->getArg(0)); break;

      default: assert(0 && "Unknown operator!"); break;
      }
    }

    void VisitCXXMemberCallExpr(CXXMemberCallExpr *E) {}
    void VisitCXXConstructExpr(CXXConstructExpr *E) {}

    void VisitBinAdd(BinaryOperator* E)  {
      if (E->getLHS()->getType()->isPointerType()) {
        Visit(E->getLHS());
      }
      if (E->getRHS()->getType()->isPointerType()) {
        Visit(E->getRHS());
      }
    }

    void VisitBinSub(BinaryOperator* E) {
      if (E->getLHS()->getType()->isPointerType()) {
        Visit(E->getLHS());
      }
    }

    void VisitBinComma(BinaryOperator* E) {
      Visit(E->getRHS());
    }

    void VisitCastExpr(CastExpr* E) {
      Visit(E->getSubExpr());
    }

    void VisitParenExpr(ParenExpr* E) {
      Visit(E->getSubExpr());
    }

    void VisitUnaryPostInc(UnaryOperator* E) {
      Visit(E->getSubExpr());
    }

    void VisitUnaryPostDec(UnaryOperator* E)  {
      Visit(E->getSubExpr());
    }

    void VisitUnaryPreInc(UnaryOperator* E) {
      Visit(E->getSubExpr());
    }

    void VisitUnaryPreDec(UnaryOperator* E)  {
      Visit(E->getSubExpr());
    }

    void VisitUnaryPlus(UnaryOperator* E) {
      Visit(E->getSubExpr());
    }

    void VisitUnaryAddrOf(UnaryOperator* E)  {
      Visit(E->getSubExpr());
    }

    void VisitUnaryDeref(UnaryOperator* E) {
      Visit(E->getSubExpr());
    }

    void VisitBinAssign(BinaryOperator* E) {}
    void VisitBinMulAssign(CompoundAssignOperator* E) {}
    void VisitBinDivAssign(CompoundAssignOperator* E) {}
    void VisitBinRemAssign(CompoundAssignOperator* E) {}
    void VisitBinAddAssign(CompoundAssignOperator* E) {}
    void VisitBinSubAssign(CompoundAssignOperator* E) {}
    void VisitBinShlAssign(CompoundAssignOperator* E) {}
    void VisitBinShrAssign(CompoundAssignOperator* E) {}
    void VisitBinAndAssign(CompoundAssignOperator* E) {}
    void VisitBinOrAssign (CompoundAssignOperator* E) {}
    void VisitBinXorAssign(CompoundAssignOperator* E) {}

    void VisitBinPtrMemD(BinaryOperator* E) {
      Visit(E->getLHS());
    }

    void VisitBinPtrMemI(BinaryOperator* E) {
      Visit(E->getLHS());
    }

    void VisitConditionalOperator(ConditionalOperator* E)   {
      Visit(E->getLHS());
      Visit(E->getRHS());
    }

    void VisitExpr(Expr* E) {}
    void VisitStmt(Stmt *S) {
      S->dump();
      llvm::errs() << "\n";
      llvm_unreachable("Unexpected expression type in ExprLValueBaseVarEvaluator");
    }
  };
}

namespace {
  enum StaticVariableAccessErrorKind {
    /// Declare a static variable
    StaticVarDecl,
    /// Reference a static variable
    StaticVarRef,
    /// Write to a static variable
    StaticVarWrite,
    /// Create a non-const alias static variable
    StaticVarEscape
  };

  std::string GenerateDiagnosticDescriptionFromErrorKind(
    StaticVariableAccessErrorKind ErrorKind,
    std::string Name) {
      std::string sbuf;
      llvm::raw_string_ostream os(sbuf);

      switch (ErrorKind) {
      case StaticVarDecl:
        os << "Declare the variable '"
          << Name << "' with static duration";
        break;
      case StaticVarRef:
        os << "Reference the variable '"
          << Name << "' with static duration";
        break;
      case StaticVarWrite:
        os << "Write to the variable '"
          << Name << "' with static duration";
        break;
      case StaticVarEscape:
        os << "Create a non-const alias to the variable '"
          << Name << "' with static duration";
        break;
          llvm_unreachable("unexpected autodesk attributes error kind!");
          return "";
      }

      return std::string(os.str());
  }

  /// All compound assign operators.
#define CAO_LIST()                                                      \
  OPERATOR(Mul) OPERATOR(Div) OPERATOR(Rem) OPERATOR(Add) OPERATOR(Sub) \
  OPERATOR(Shl) OPERATOR(Shr) OPERATOR(And) OPERATOR(Or)  OPERATOR(Xor)

  /// VariableGuard - Reverts the given variable to its orginal value when the guard goes
  /// out-of-scope.
  template <typename T>
  class VariableGuard {
  public:
    VariableGuard(T& var, T newValue)
      : Var(var), OldValue(var)
    {
      var = newValue;
    }

    ~VariableGuard()
    {
      Var = OldValue;
    }

  private:
    T& Var;
    const T OldValue;
  };

  /// Classify overloaed C++ operator with respect to whether they have a write
  /// semantic. Note, that we can't be absolutely certain, that the given
  /// operator actually modifies the object. But, in practice, every C++
  /// programmer would expect calls like "x += 3" or "++x" to modify
  /// x. We therefore assume the worst case, i.e. that the variable is
  /// being modified even we can't prove it for sure.
  bool isAWriteCXXOperator(OverloadedOperatorKind overloadedOp) {
    switch (overloadedOp) {
    case OO_None:                 return false;
    case OO_New:                  return false;
    case OO_Delete:               return true;
    case OO_Array_New:            return false;
    case OO_Array_Delete:         return true;
    case OO_Plus:                 return false;
    case OO_Minus:                return false;
    case OO_Star:                 return false;
    case OO_Slash:                return false;
    case OO_Percent:              return false;
    case OO_Caret:                return false;
    case OO_Amp:                  return false;
    case OO_Pipe:                 return false;
    case OO_Tilde:                return false;
    case OO_Exclaim:              return false;
    case OO_Equal:                return true;
    case OO_Less:                 return false;
    case OO_Greater:              return false;
    case OO_PlusEqual:            return true;
    case OO_MinusEqual:           return true;
    case OO_StarEqual:            return true;
    case OO_SlashEqual:           return true;
    case OO_PercentEqual:         return true;
    case OO_CaretEqual:           return true;
    case OO_AmpEqual:             return true;
    case OO_PipeEqual:            return true;
    case OO_LessLess:             return false;
    case OO_GreaterGreater:       return false;
    case OO_LessLessEqual:        return true;
    case OO_GreaterGreaterEqual:  return true;
    case OO_EqualEqual:           return true;
    case OO_ExclaimEqual:         return true;
    case OO_LessEqual:            return true;
    case OO_GreaterEqual:         return true;
    case OO_AmpAmp:               return false;
    case OO_PipePipe:             return false;
    case OO_PlusPlus:             return true;
    case OO_MinusMinus:           return true;
    case OO_Comma:                return false;
    case OO_ArrowStar:            return false;
    case OO_Arrow:                return false;
    case OO_Call:                 return false;
    case OO_Subscript:            return false;

    default: assert(0 && "Unknown operator!"); return true;
    }
  }

  class ASTTraverser : public RecursiveASTVisitor<ASTTraverser> {

  typedef RecursiveASTVisitor<ASTTraverser> BaseClass;

  const CheckerBase *Checker;
  ento::BugReporter& BR;
  ASTContext& Ctx;
  AnalysisDeclContext* AC;

  const bool IsTracingEnabled;
  int indentation;

  QualType CurrentFunctionReturnType;
  SourceRange SourceRangeOverride;
  SourceRange CurrentCallSourceRange;
	std::string CurrentBugDescription;

  ExprLValueBaseVarEvaluator::ResultType CurrentLValueBases;
  ExprLValueBaseVarEvaluator::ResultType CurrentParamBases;

public:
  enum TracingType {
     WITHOUT_TRACING,
     WITH_TRACING
  };

  explicit ASTTraverser(
    const CheckerBase *Checker,
    ento::BugReporter& BR, ASTContext& ctx,
    AnalysisDeclContext* AC,
    TracingType tracingType
  ) : Checker(Checker),
      BR(BR),
      Ctx(ctx),
      AC(AC),
      IsTracingEnabled(tracingType==WITH_TRACING),
      indentation(0) {}

  bool shouldVisitTemplateInstantiations() const { return true; }

  static const FunctionType* getFunctionType(QualType CalleeType,
      ASTContext& Context) {
    QualType TY = CalleeType.getDesugaredType(Context);

    /// Handle the pointer to function cases.
    while (true) {
      if (const PointerType* PT = TY->getAs<PointerType>()) {
        TY = PT->getPointeeType();
      }
      else if (const BlockPointerType* BPT = TY->getAs<BlockPointerType>()) {
        TY = BPT->getPointeeType();
      }
      else if (const MemberPointerType* MPT = TY->getAs<MemberPointerType>()) {
        TY = MPT->getPointeeType();
      }
      else {
        break;
      }
    }

    assert(TY->isFunctionType() && "Type must be a function or a pointer to function!");
    return TY->getAs<FunctionType>();
  }

  template <class ASTNodeType>
  void checkValueDeclForStaticDuration(ASTNodeType* Node, ValueDecl* D) {
    if (VarDecl* VD = dyn_cast<VarDecl>(D)) {
      // We have to anaylyze variables separately because static local
      // variables does not have any linkage.
      if (VD->hasLocalStorage()) {
        return;
      }

      // FIXME
#if 0
      // We only need to check static variable that has function scope
      // (required by Richard Latham)
      if (!VD->isStaticLocal())
        return;
#endif
    }
    else {
      assert(
        (
          // We don't care about function declarations
          isa<FunctionDecl>(D) ||
          // We don't care about enumerants.
          isa<EnumConstantDecl>(D) ||
          // FieldDecl represents non-static data members of classes.
          isa<FieldDecl>(D)
        ) && "Unknown ValueDecl type.");
      return;
    }

    if (CurrentLValueBases.count(D)) {
      // We should not report on static variables in functions that
      // are marked as no effect. This is required by Richard Latham too.
      // FIXME: use a better attribute type.
#if 0
      DeclContext *Ctx = D->getDeclContext();
      if (FunctionDecl *Function = dyn_cast<FunctionDecl>(Ctx)) {
        // FIXME
        /*
        if (Function->hasAttr<clang::NoEffectAttr>())
          return;
        */
      }
#endif

      // report bug
      SourceRange SR =
        SourceRangeOverride.getBegin().isValid() ?
        SourceRangeOverride : Node->getSourceRange();

      // In some cases, the source range is invalid and the
      // BugReporter class does not handle that. We therefore fix
      // this here!
      if (!SR.getEnd().isValid()) SR.setEnd(SR.getBegin());

			CurrentBugDescription = GenerateDiagnosticDescriptionFromErrorKind(
        StaticVarWrite,
        D->getQualifiedNameAsString());
			const char *desc = CurrentBugDescription.c_str();

      PathDiagnosticLocation VDLoc =
        PathDiagnosticLocation::createBegin(Node, BR.getSourceManager(), AC);
      BR.EmitBasicReport(D, Checker, desc, "static variable access", desc,
        VDLoc, SR);

    }
  }

  template <typename FunctionDeclType>
  bool TraverseFunctionDeclHelper(
    FunctionDeclType* D,
    bool (BaseClass::*BaseClassTraverseFunctionDecl)(FunctionDeclType* )
    ) {

      VariableGuard<QualType> CurrentFunctionReturnTypeGuard(
        CurrentFunctionReturnType,
        D->getReturnType() );

      return (this->*BaseClassTraverseFunctionDecl)(D);
  }

  bool TraverseFunctionDecl(FunctionDecl* D) {
    return TraverseFunctionDeclHelper(D, &BaseClass::TraverseFunctionDecl);
  }

  bool TraverseCXXMethodDecl(CXXMethodDecl* D) {
    return TraverseFunctionDeclHelper(D, &BaseClass::TraverseCXXMethodDecl);
  }

  bool TraverseCXXConstructorDecl(CXXConstructorDecl* D) {
    return TraverseFunctionDeclHelper(D, &BaseClass::TraverseCXXConstructorDecl);
  }

  bool TraverseCXXDestructorDecl(CXXDestructorDecl* D) {
    return TraverseFunctionDeclHelper(D, &BaseClass::TraverseCXXDestructorDecl);
  }

  bool TraverseCXXConversionDecl(CXXConversionDecl* D) {
    return TraverseFunctionDeclHelper(D, &BaseClass::TraverseCXXConversionDecl);
  }

  bool TraverseBlockExpr(BlockExpr* S) {
    VariableGuard<QualType> CurrentFunctionReturnTypeGuard(
      CurrentFunctionReturnType,
      S->getFunctionType()->getReturnType());

    return BaseClass::TraverseBlockExpr(S);
  }

  //============================================================================
  // Visitors for tracing the traversal process itself.
  //============================================================================

  /// Trace the traversing of a node in the AST.
  class TraceTraversalGuard {
  public:
    TraceTraversalGuard(
      ASTTraverser& checker,
      std::string Type,
      std::string Name,
      void* Addr,
      std::string OtherName = std::string()
    )
      : Checker(checker) {
      if ( Checker.IsTracingEnabled ) {
        llvm::errs().indent(checker.indentation);
        llvm::errs() << "Traversing " << Type << " "
                     << Name << " @" << Addr << " (";
        if (!OtherName.empty()) {
          llvm::errs() << OtherName;
        }
        llvm::errs() << ") { \n";
        Checker.indentation += 2;
      }
    }

    ~TraceTraversalGuard() {
      if ( Checker.IsTracingEnabled ) {
        Checker.indentation -= 2;
        llvm::errs().indent(Checker.indentation);
        llvm::errs() << "}\n";
      }
    }

  private:
    ASTTraverser& Checker;
  };

  void Dump(
    const ExprLValueBaseVarEvaluator::ResultType& variables,
    const char* varSetName
  ) {
    if ( IsTracingEnabled ) {
      if (variables.empty()) {
        llvm::errs().indent(indentation);
        llvm::errs() << varSetName << " base variable: None\n";
      }
      else {
        for (ExprLValueBaseVarEvaluator::ResultType::const_iterator
               it  = variables.begin(),
               end = variables.end();
             it != end ; ++it) {
          llvm::errs().indent(indentation);
          llvm::errs() << varSetName << " base variable @"
                       << (void*)(*it) << ": ";
          (*it)->dump();
          llvm::errs() << "\n";
        }
      }
    }
  }

  bool TraverseDecl(Decl* D) {
    if (!D)
      return true;

    NamedDecl* ND = dyn_cast<NamedDecl>(D);
    TraceTraversalGuard traceGuard(
      *this, "Decl", D->getDeclKindName(), D,
      ND ? ND->getQualifiedNameAsString() : "");

    if (IsTracingEnabled) {
      // BaseClass::TraverseDecl() ignores implicit declarations because
      // as a syntax visitor, one should ignore declarations for
      // implicitly-defined declarations (ones not typed explicitly by
      // the user). For our purpose, we also do not need to analyze
      // implicitly generated functions because we correctly compute
      // their attributes, but for debugging purposes it is still
      // helpful to trace them.
      if (D->isImplicit()) {
        llvm::errs().indent(indentation);
        llvm::errs() << "\"***Implicit declaration***\"\n";
      }
    }

    return BaseClass::TraverseDecl(D);
  }

  bool TraverseStmt(Stmt* S) {
    if (!S)
      return true;

    TraceTraversalGuard traceGuard(
      *this, "Stmt", S->getStmtClassName(), S);

    Expr* E = dyn_cast<Expr>(S);
    if (IsTracingEnabled) {
      if (E) {
        llvm::errs().indent(indentation);
        llvm::errs() << "isTypeDependent = " << E->isTypeDependent() << "\n";
      }
    }

    // Skip template dependent expressions since we can't reliably
    // analyse them. These template dependent expressions will be
    // checked once they are instanted or specialized.
    if (E && E->isTypeDependent()) return true;

    return BaseClass::TraverseStmt(S);
  }

  bool VisitFunctionDecl(FunctionDecl* D) {
    if ( IsTracingEnabled ) {
      llvm::errs().indent(indentation);
      D->dump();
      llvm::errs() << "\n";
    }

    return BaseClass::VisitFunctionDecl(D);
  }

  bool VisitBinaryOperator(BinaryOperator* S) {
    if ( IsTracingEnabled ) {
      llvm::errs().indent(indentation);
      llvm::errs() << "Operator " << S->getOpcodeStr() << "\n";
    }

    return BaseClass::VisitBinaryOperator(S);
  }

  bool VisitUnaryOperator(UnaryOperator* S) {
    if ( IsTracingEnabled ) {
      llvm::errs().indent(indentation);
      llvm::errs() << "Operator "
                   << UnaryOperator::getOpcodeStr(S->getOpcode()) << "\n";
    }

    return BaseClass::VisitUnaryOperator(S);
  }

  //============================================================================
  // Visitors for different types of call expressions.
  //============================================================================

  bool VisitCallExpr(CallExpr* S) {
    return true;
  }

  bool VisitCXXConstructExpr(CXXConstructExpr* S) {
    return true;
  }

  bool VisitCXXDestructorDecl(CXXDestructorDecl* D) {
    return true;
  }

  bool VisitCXXNewExpr(CXXNewExpr* S) {
    return true;
  }

  bool VisitCXXDeleteExpr(CXXDeleteExpr* S) {
    return true;
  }

  bool VisitVarDecl(VarDecl* D) {
    QualType ElementType = D->getType();

    if (const ArrayType* AT = Ctx.getAsArrayType(ElementType)) {
      ElementType = Ctx.getBaseElementType(AT);
    }

    if (!D->hasLocalStorage()) {
      /// FIXME - check declaration of local static
      /// variable inside a function if requires.
    }

    return true;
  }

  bool VisitCXXBindTemporaryExpr(CXXBindTemporaryExpr* S) {
    assert(S->getType()->isStructureOrClassType() &&
           "Temporary objects should only be created for instances of a class");

    return true;
  }

  //============================================================================
  // Visitors for references to static variables.
  //============================================================================

  bool VisitDeclRefExpr(DeclRefExpr* E) {
    if ( IsTracingEnabled ) {
      llvm::errs().indent(indentation);
      E->dump();
      llvm::errs() << "\n";
    }

    checkValueDeclForStaticDuration(E,E->getDecl());
    return true;
  }

  bool VisitMemberExpr(MemberExpr* E) {
    checkValueDeclForStaticDuration(E,E->getMemberDecl());
    return true;
  }

  //==========================================================================
  // Traversals helpers for checking binding for static variables
  // references and writes.
  //==========================================================================

  bool TraverseObjectCallHelper(Expr* Object, bool isConst,
                                OverloadedOperatorKind overloadedOp) {
    if (isConst) {
      if (!TraverseStmt(Object)) return false;
    }
    else {
      if (isAWriteCXXOperator(overloadedOp)) {
        // Always assume that C++ operators with write semantic are
        // always writting to the this pointer!
        ExprLValueBaseVarEvaluator lvalueBaseVarEvaluator(Object);
        VariableGuard<ExprLValueBaseVarEvaluator::ResultType> LValueBasesGuard(
          CurrentLValueBases, lvalueBaseVarEvaluator.getBaseVariables());
        Dump(CurrentLValueBases, "write op \"this\" pointer");
        if (!TraverseStmt(Object)) return false;
      }
      else {
        ExprLValueBaseVarEvaluator paramBaseVarEvaluator(Object);
        VariableGuard<ExprLValueBaseVarEvaluator::ResultType> ParamBasesGuard(
          CurrentParamBases, paramBaseVarEvaluator.getBaseVariables());
        Dump(CurrentParamBases, "\"this\" pointer");
        if (!TraverseStmt(Object)) return false;
      }
    }

    return true;
  }

  bool TraverseThisPointerCallHelper( Expr* Object, QualType CalleeType,
                                      OverloadedOperatorKind overloadedOp) {
    TraceTraversalGuard traceGuard(*this, "CallHelper", "ThisPointer", Object);

    const FunctionType* FT = getFunctionType(CalleeType, Ctx);

    assert(FT->isFunctionProtoType());

    return TraverseObjectCallHelper(
      Object,
      FT->getAs<FunctionProtoType>()->getTypeQuals() & Qualifiers::Const,
      overloadedOp);
  }

  bool TraverseBindingHelper(QualType LHSType, Expr* RHS) {
    TraceTraversalGuard traceGuard(*this, "BindingHelper", "RHS", RHS);

    if (!RHS) return true;

    bool isAliasingPossible = false;
    if (LHSType->isPointerType() || LHSType->isReferenceType()) {
      QualType pointeeType =
        LHSType->isPointerType() ? LHSType->getPointeeType() : LHSType.getNonReferenceType();

      while (true) {
        if (!pointeeType.isConstQualified()) {
          isAliasingPossible = true;
          break;
        }
        if (!pointeeType->isPointerType()) {
          break;
        }
        pointeeType = pointeeType->getPointeeType();
      }
    }

    if (isAliasingPossible) {
      ExprLValueBaseVarEvaluator paramBaseVarEvaluator(RHS);
      VariableGuard<ExprLValueBaseVarEvaluator::ResultType> ParamBasesGuard(
        CurrentParamBases, paramBaseVarEvaluator.getBaseVariables());
      Dump(CurrentParamBases, "Binding");
      return TraverseStmt(RHS);
    }
    else {
      return TraverseStmt(RHS);
    }
  }

  bool TraverseCallExprHelper(CallExpr *S, OverloadedOperatorKind overloadedOp) {
    TraceTraversalGuard traceGuard(*this, "Helper", "CallExpr", S);

    VariableGuard<SourceRange> SourceRangeGuard(
      CurrentCallSourceRange, S->getSourceRange());

    Expr* Callee =  S->getCallee();
    if (!TraverseStmt(Callee)) return false;

    QualType CalleeType = Callee->getType();

    if (Callee->getType()->isSpecificPlaceholderType(BuiltinType::BoundMember)) {
      CalleeType = clang::Expr::findBoundMemberType(Callee);
    }

    const FunctionType* FT = getFunctionType(CalleeType, Ctx);

    const unsigned numArgs = S->getNumArgs();
    unsigned argIdx = 0;

    if (const FunctionProtoType* FTProto = FT->getAs<FunctionProtoType>()) {
      const unsigned numParams = FTProto->getNumParams();

      unsigned paramIdx = 0;

      if (overloadedOp == OO_Equal || overloadedOp == OO_Arrow ||
          overloadedOp == OO_Call  || overloadedOp == OO_Subscript) {
        // These overloaded operators can only be declared as member
        // functions and therefore we have to traverse the first
        // operator argument as a "this" pointer. Note that this first
        // argument will not be part of the function
        // prototype. Note also to some of these overloaded operators
        // accept variable length argument list.
        TraverseThisPointerCallHelper(
          S->getArg(argIdx), CalleeType, overloadedOp);
        ++argIdx;
      }
      else if (overloadedOp != OO_None) {
        // Always traverse the first operator argument as if it is a
        // "this" pointer whether the operator is a free or a member
        // function.
        if (numParams + 1 == numArgs) {
          // This is a C++ member function operator. The first
          // parameter is for the this pointer and is not part of the
          // function prototype. We therefore need to handle this case
          // as a special case.
          TraverseThisPointerCallHelper(
            S->getArg(argIdx), CalleeType, overloadedOp);
        }
        else {
          Expr* Object = S->getArg(argIdx);
          QualType ObjectType = Object->getType();
          TraverseObjectCallHelper(Object, ObjectType.isConstQualified(), overloadedOp);

          assert(numParams == numArgs);
          ++paramIdx;
        }

        ++argIdx;
      }

      for (; paramIdx<numParams; ++paramIdx, ++argIdx) {
        Expr* Arg = S->getArg(argIdx);
        QualType ParamType = FTProto->getParamType(paramIdx);
        if (!TraverseBindingHelper(ParamType, Arg)) return false;
      }
    }

    // Argument with no matching parameter prototype.
    for (; argIdx<numArgs; ++argIdx) {
      Expr* Arg = S->getArg(argIdx);
      QualType ArgType = Arg->getType();
      if (!TraverseBindingHelper(ArgType, Arg)) return false;
    }

    return true;
  }

  bool TraverseCXXConstructExprHelper(CXXConstructExpr *S) {
    TraceTraversalGuard traceGuard(*this, "Helper", "CXXConstructExpr", S);

    const QualType CalleeType = S->getConstructor()->getType();
    const FunctionType* FT = getFunctionType(CalleeType, Ctx);

    const unsigned numArgs = S->getNumArgs();
    unsigned argIdx = 0;

    if (const FunctionProtoType* FTProto = FT->getAs<FunctionProtoType>()) {
      const unsigned numParams = FTProto->getNumParams();
      for (unsigned paramIdx = 0; paramIdx<numParams; ++paramIdx, ++argIdx) {
        Expr* Arg = S->getArg(argIdx);
        QualType ParamType = FTProto->getParamType(paramIdx);
        if (!TraverseBindingHelper(ParamType, Arg)) return false;
      }
    }

    // Argument with no matching parameter prototype.
    for (; argIdx<numArgs; ++argIdx) {
      Expr* Arg = S->getArg(argIdx);
      QualType ArgType = Arg->getType();
      if (!TraverseBindingHelper(ArgType, Arg)) return false;
    }

    return true;
  }

  //==========================================================================
  // Traversals for checking the bindings of various types of function
  // calls for static variables references and writes.
  //==========================================================================

  bool TraverseCallExpr(CallExpr *S) {
    if (!WalkUpFromCallExpr(S)) return false;
    return TraverseCallExprHelper(S, OO_None);
  }

  bool TraverseCXXMemberCallExpr(CXXMemberCallExpr *S) {
    if (!WalkUpFromCXXMemberCallExpr(S)) return false;
    return TraverseCallExprHelper(S, OO_None);
  }

  bool TraverseCXXConstructExpr(CXXConstructExpr *S) {
    if (!WalkUpFromCXXConstructExpr(S)) return false;
    return TraverseCXXConstructExprHelper(S);
  }

  bool TraverseCXXTemporaryObjectExpr(CXXTemporaryObjectExpr *S) {
    if (!WalkUpFromCXXTemporaryObjectExpr(S)) return false;
    if (!TraverseType(S->getType())) return false;
    return TraverseCXXConstructExprHelper(S);
  }

  bool TraverseCXXOperatorCallExpr(CXXOperatorCallExpr *S) {
    if (!WalkUpFromCXXOperatorCallExpr(S)) return false;
    return TraverseCallExprHelper(S, S->getOperator());
  }

  bool TraverseCXXDeleteExpr(CXXDeleteExpr *S) {
    Expr* Arg = S->getArgument();

    // Skip processing the delete expression if the argument is
    // dependent of template type arguments. The delete expression
    // will be analyzed when the template is instantiated.
    if (Arg->isTypeDependent()) return true;

    if (!WalkUpFromCXXDeleteExpr(S)) return false;

    // Always assume that invoking delete on static variable (of
    // pointer type) writes to the variable.
    ExprLValueBaseVarEvaluator lvalueBaseVarEvaluator(Arg);
    VariableGuard<ExprLValueBaseVarEvaluator::ResultType> LValueBasesGuard(
      CurrentLValueBases, lvalueBaseVarEvaluator.getBaseVariables());
    Dump(CurrentLValueBases, "deleting \"this\" pointer");
    if (!TraverseStmt(Arg)) return false;

    return true;
  }

  bool TraverseConstructorInitializer(CXXCtorInitializer *Init) {
    TraceTraversalGuard traceGuard(
      *this, "Init", " CXXCtorInitializer", Init);

    if (Init->isMemberInitializer()) {
      if (!TraverseBindingHelper(
            Init->getMember()->getType(), Init->getInit()))
        return false;
    }
    else {
      if (!TraverseStmt(Init->getInit())) return false;
    }
    return true;
  }

  bool TraverseMemberExpr(MemberExpr* S) {
    if (!WalkUpFromMemberExpr(S)) return false;
    if (!TraverseTemplateArgumentLocsHelper(
          S->getTemplateArgs(), S->getNumTemplateArgs())) return false;
    // FIXME: Should we be recursing on the qualifier?
    if (!TraverseNestedNameSpecifier(S->getQualifier())) return false;

    if (isa<CXXMethodDecl>(S->getMemberDecl())) {
      return TraverseThisPointerCallHelper(
        S->getBase(),
        S->getMemberDecl()->getType(),
        S->isArrow() ? OO_Arrow : OO_None);
    }
    else {
      return TraverseStmt(S->getBase());
    }
  }

#define TRAVERSE_BINOP_PTR_MEM(PTR_MEM_TYPE, OO_TYPE)                   \
  bool TraverseBinPtrMem##PTR_MEM_TYPE(BinaryOperator* S)               \
  {                                                                     \
    if (!WalkUpFromBinPtrMem##PTR_MEM_TYPE(S)) return false;            \
                                                                        \
    const QualType RhsType = S->getRHS()->getType();                    \
                                                                        \
    if (RhsType->isMemberFunctionPointerType()) {                       \
      if (!TraverseThisPointerCallHelper(S->getLHS(), RhsType, OO_TYPE)) \
        return false;                                                   \
    }                                                                   \
    else {                                                              \
      if (!TraverseStmt(S->getLHS())) return false;                     \
    }                                                                   \
                                                                        \
    return TraverseStmt(S->getRHS());                                   \
  }

  TRAVERSE_BINOP_PTR_MEM(D, OO_None);
  TRAVERSE_BINOP_PTR_MEM(I, OO_ArrowStar);
#undef TRAVERSE_BINOP_PTR_MEM


  //==========================================================================
  // Traversals for checking the bindings of various types of variable
  // declarations for static variables references and writes.
  // ==========================================================================

  bool TraverseVarDecl(VarDecl* D) {
    if (!WalkUpFromVarDecl(D)) return false;
    if (!TraverseDeclaratorHelper(D)) return false;
    if (!TraverseBindingHelper(D->getType(), D->getInit())) return false;
    if (!TraverseDeclContextHelper(dyn_cast<DeclContext>(D))) return false;
    return true;
  }


  bool TraverseParmVarDecl(ParmVarDecl* D) {
    if (!WalkUpFromVarDecl(D)) return false;
    if (!TraverseDeclaratorHelper(D)) return false;

    if (D->hasDefaultArg() &&
        !D->hasUnparsedDefaultArg()) {
      if (D->hasUninstantiatedDefaultArg()) {
        if (!TraverseStmt(D->getUninstantiatedDefaultArg())) return false;
      }
      else {
        if (!TraverseBindingHelper(D->getType(), D->getDefaultArg())) return false;
      }
    }

    if (!TraverseDeclContextHelper(dyn_cast<DeclContext>(D))) return false;
    return true;
  }


  bool TraverseCXXDefaultArgExpr(CXXDefaultArgExpr *S) {
    if (!WalkUpFromCXXDefaultArgExpr(S)) return false;

    VariableGuard<SourceRange> SourceRangeGuard(
      SourceRangeOverride, CurrentCallSourceRange);

    return TraverseBindingHelper(S->getParam()->getType(), S->getExpr());
  }

  bool TraverseReturnStmt(ReturnStmt* S) {
    if (!WalkUpFromReturnStmt(S)) return false;

    return TraverseBindingHelper(CurrentFunctionReturnType, S->getRetValue());
  }

  //==========================================================================
  // Traversals for checking the bindings of various types of
  // assignment operators and unary operators with side-effects for
  // static variables references and writes.
  // ==========================================================================

#define TRAVERSE_BINOP(NAME, BINOP_TYPE)                            \
  bool TraverseBin##NAME(BINOP_TYPE* S)                             \
  {                                                                 \
    if (!WalkUpFromBin##NAME(S)) return false;                      \
                                                                    \
    Expr* LHS = S->getLHS();                                        \
    Expr* RHS = S->getRHS();                                        \
    {                                                               \
      ExprLValueBaseVarEvaluator lValueBaseVarEvaluator(LHS);   \
      VariableGuard<ExprLValueBaseVarEvaluator::ResultType>     \
        LValueBaseGuard(CurrentLValueBases,                         \
                        lValueBaseVarEvaluator.getBaseVariables()); \
      Dump(CurrentLValueBases, "Bin" #NAME);                        \
      if (!TraverseStmt(LHS)) return false;                         \
    }                                                               \
    if (!TraverseBindingHelper(LHS->getType(), RHS)) return false;  \
    return true;                                                    \
  }

  TRAVERSE_BINOP(Assign, BinaryOperator);

#define OPERATOR(NAME)                                  \
  TRAVERSE_BINOP(NAME##Assign, CompoundAssignOperator)

  CAO_LIST()
#undef OPERATOR
#undef TRAVERSE_BINOP

#define TRAVERSE_UNARY_OP(NAME)                                         \
  bool TraverseUnary##NAME(UnaryOperator* S)                            \
  {                                                                     \
    if (!WalkUpFromUnary##NAME(S)) return false;                        \
    {                                                                   \
      ExprLValueBaseVarEvaluator lValueBaseVarEvaluator(S->getSubExpr()); \
      VariableGuard<ExprLValueBaseVarEvaluator::ResultType>         \
        LValueBaseGuard(CurrentLValueBases,                             \
                        lValueBaseVarEvaluator.getBaseVariables());     \
      Dump(CurrentLValueBases, "Unary" #NAME);                          \
      if (!TraverseStmt(S->getSubExpr())) return false;                 \
    }                                                                   \
    return true;                                                        \
  }

  TRAVERSE_UNARY_OP(PostInc);
  TRAVERSE_UNARY_OP(PostDec);
  TRAVERSE_UNARY_OP(PreInc);
  TRAVERSE_UNARY_OP(PreDec);

#undef TRAVERSE_UNARY_OP

};

}

//===----------------------------------------------------------------------===//
// Entry point for checks.
//===----------------------------------------------------------------------===//
static void TraverseAST(TranslationUnitDecl &TU,
  const CheckerBase *Checker,
  AnalysisDeclContext* AC,
  ento::BugReporter &BR)
{
#if 1
  const char* const EnableTracingString =
    std::getenv("ENABLE_TRACE_CHECKER") ;
  const bool EnableTracing = (EnableTracingString &&
    std::atoi(EnableTracingString) != 0);
#else
  const bool EnableTracing = true;
#endif

  ASTTraverser traverser(
    Checker, BR, TU.getASTContext(), AC,
    EnableTracing ? ASTTraverser::WITH_TRACING
                  : ASTTraverser::WITHOUT_TRACING);
  traverser.TraverseDecl(&TU);
}

namespace {
class  StaticVariableAccessChecker
  : public Checker<check::ASTDecl<TranslationUnitDecl> > {

public:
  void checkASTDecl(const TranslationUnitDecl* D, AnalysisManager& mgr, BugReporter &BR) const {
    TraverseAST(*const_cast<TranslationUnitDecl*>(D), this, mgr.getAnalysisDeclContext(D), BR);
  }
};
}

void ento::registerStaticVariableAccessChecker(CheckerManager &mgr) {
  mgr.registerChecker<StaticVariableAccessChecker>();
}

