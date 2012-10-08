//=== SafeParallelismChecker.cpp - Safe Parallelism checker -----*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This files defines the Safe Parallelism checker, which tries to prove the
// safety of parallelism given region and effect annotations
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

using namespace clang;
using namespace ento;


namespace {
  /**
   *  Return true when the input string is a special RPL element
   *  (e.g., '*', '?', 'Root'.
   */
  // TODO (?): '?', 'Root'
  bool isSpecialRplElement(const StringRef& s) {
    if (!s.compare("*"))
      return true;
    else
      return false;
  }

  /**
   *  Return true when the input string is a valid region
   *  name or region parameter declaration
   */
  bool isValidRegionName(const StringRef& s) {
    // false if it is one of the Special Rpl Elements
    // => it is not allowed to redeclare them
    if (isSpecialRplElement(s)) return false;

    // must start with [_a-zA-Z]
    const char c = s.front();
    if (c != '_' &&
        !( c >= 'a' && c <= 'z') &&
        !( c >= 'A' && c <= 'Z'))
      return false;
    // all remaining characters must be in [_a-zA-Z0-9]
    for (size_t i=0; i < s.size(); i++) {
      const char c = s[i];
      if (c != '_' &&
          !( c >= 'a' && c <= 'z') &&
          !( c >= 'A' && c <= 'Z') &&
          !( c >= '0' && c <= '9'))
        return false;
    }

    return true;
  }

  /**
   * Return the string name of the region or region parameter declaration
   * based on the Kind of the Attribute (RegionAttr or RegionParamAttr)
   */
  inline
  StringRef getRegionOrParamName(const Attr* attr) {
    StringRef result = "";
    attr::Kind kind = attr->getKind();
    switch(kind) {
    case attr::Region:
      result = dyn_cast<RegionAttr>(attr)->getRegion_name(); break;
    case attr::RegionParam:
      result = dyn_cast<RegionParamAttr>(attr)->getParam_name(); break;
    default:
      result = "";
    }
    return result;
  }

  template<typename AttrType>
  bool scanAttributes(Decl* D, const StringRef& name)
  {
    int i = 0;
    const AttrType* myAttr = D->getAttr<AttrType>(i++);
    while (myAttr) {
      StringRef rName = getRegionOrParamName(myAttr);
      if (rName==name) return true;
      myAttr = D->getAttr<AttrType>(i++);
    }
    return false;
  }

  /**
   *  Looks for 'name' in the declaration 'D' and its parent scopes
   */
  bool findRegionName(Decl* D, StringRef name) {
    if (!D) return false;
    /// 1. try to find among regions or region parameters of function
    if (scanAttributes<RegionAttr>(D,name) ||
        scanAttributes<RegionParamAttr>(D,name))
      return true;

    /// if not found, search parent DeclContexts
    DeclContext *dc = D->getDeclContext();
    while (dc) {
      if (dc->isFunctionOrMethod()) {
        FunctionDecl* fd = dyn_cast<FunctionDecl>(dc);
        assert(fd);
        return findRegionName(fd, name);
      } else if (dc->isRecord()) {
        RecordDecl* rd = dyn_cast<RecordDecl>(dc);
        assert(rd);
        return findRegionName(rd, name);
      } else {
        dc = dc->getParent();
      }
    }
    return false;
  }

///-///////////////////////////////////////////////////////////////////////////
/// Rpl Class
class Rpl {
  friend class Rpl;
private:
  /// Fields
  StringRef rpl;
  Decl* decl;
  typedef llvm::SmallVector<StringRef,8> ElementVector;
  ElementVector rplElements;

  /// RplRef class
  class RplRef {
    long firstIdx; 
    long lastIdx;
    Rpl& rpl;
  
  public:
    /// Constructor
    RplRef(Rpl& r) : rpl(r) {
      firstIdx = 0;
      lastIdx = rpl.rplElements.size()-1;
    }
    /// Printing
    void printElements(raw_ostream& os) {
      int i = firstIdx;
      for (; i<lastIdx; i++) {
        os << rpl.rplElements[i] << ":";
      } 
      // print last element
      if (i==lastIdx)
        os << rpl.rplElements[i];
  }
  std::string toString() {
    std::string sbuf;
    llvm::raw_string_ostream os(sbuf);
    printElements(os);
    return std::string(os.str());
  }

    /// Getters
    StringRef getFirstElement() {
      return rpl.rplElements[firstIdx];
    }
    StringRef getLastElement() {
      return rpl.rplElements[lastIdx];
    }
    
    RplRef& stripLast() {
      lastIdx--;
      return *this;
    }
    inline bool isEmpty() {
      //llvm::errs() << "DEBUG:: isEmpty[RplRef] returned " 
      //    << (lastIdx<firstIdx ? "true" : "false") << "\n";
      return (lastIdx<firstIdx) ? true : false;
    }
    
    bool isUnder(RplRef& rhs) {
      /// R <= Root
      if (rhs.isEmpty())
        return true;
      if (isEmpty()) /// and rhs is not Empty
        return false;
      /// R <= R' <== R c= R'
      if (isIncludedIn(rhs)) return true;
      /// R:* <= R' <== R <= R'
      if (!getLastElement().compare("*"))
        return stripLast().isUnder(rhs);
      /// R:r <= R' <==  R <= R'
      /// R:[i] <= R' <==  R <= R'
      return stripLast().isUnder(rhs.stripLast());
      /// TODO z-regions 
    }
    
    /// Inclusion: this c= rhs
    bool isIncludedIn(RplRef& rhs) { 
      //llvm::errs() << "DEBUG:: ~~~~~~~~isIncludedIn[RplRef](" 
      //    << this->toString() << ", " << rhs.toString() << ")\n";
      /// Root c= Root    
      if (isEmpty() && rhs.isEmpty())
        return true;
      /// R c= R':* <==  R <= R'
      if (!rhs.getLastElement().compare("*")) {
        llvm::errs() <<"DEBUG:: isIncludedIn[RplRef] compared *==0\n";
        return isUnder(rhs.stripLast());
      }
      /// Both cannot be empty[==Root] (see 1st case above)
      /// Case1 rhs.isEmpty (e.g., R1 <=? Root) ==> not included
      /// Case2 lhs.isEmpty (e.g., Root <= R1[!='*']) ==> not included
      if (isEmpty() || rhs.isEmpty())
        return false;
      /// R:r c= R':r  <== R <= R'
      /// R:[i] c= R':[i]  <== R <= R'
      if (!rhs.getLastElement().compare(getLastElement()))
        return this->stripLast().isIncludedIn(rhs.stripLast());
      /// TODO : more rules
      return false;
    }
  }; // end class RplRef
  
public:
  /// Constructors
  Rpl(StringRef rpl, Decl* D):rpl(rpl), decl(D) {
    //bool result = true;
    while(rpl.size() > 0) { /// for all RPL elements of the RPL
      // FIXME: '::' can appear as part of an RPL element. Splitting must
      // be done differently to account for that.
      std::pair<StringRef,StringRef> pair = rpl.split(':');
      const StringRef& head = pair.first;
      /// head: is it a special RPL element? if not, is it declared?
      if (!isSpecialRplElement(head) && !findRegionName(D, head)) {
        /// TODO
        // Emit bug report!
        //helperEmitUndeclaredRplElementWarning(D, head);
        //result = false;
      }
      rplElements.push_back(head);
      rpl = pair.second;
    }
    //return result;
  }
  /// Printing
  void printElements(raw_ostream& os) {
    ElementVector::const_iterator I = rplElements.begin();
    ElementVector::const_iterator E = rplElements.end();
    for (; I < E-1; I++) {
      os << (*I) << ":";
    }
    // print last element
    if (I==E-1)
      os << (*I);
  }
  std::string toString() {
    std::string sbuf;
    llvm::raw_string_ostream os(sbuf);
    printElements(os);
    return std::string(os.str());
  }
  /// Getters
  inline const StringRef getLastElement() {
    return rplElements.back(); 
  }
  
  inline size_t length() {
    return rplElements.size();
  }
  
  /// Nesting (Under)
  bool isUnder(Rpl& rhsRpl) {
    RplRef* lhs = new RplRef(*this);
    RplRef* rhs = new RplRef(rhsRpl);
    bool result = lhs->isIncludedIn(*rhs);
    delete lhs; delete rhs;    
    return result;
  }
  /// Inclusion
  bool isIncludedIn(Rpl& rhsRpl) { 
    RplRef* lhs = new RplRef(*this);
    RplRef* rhs = new RplRef(rhsRpl);
    bool result = lhs->isIncludedIn(*rhs);
    delete lhs; delete rhs;
    llvm::errs() << "DEBUG:: ~~~~~ isIncludedIn[RPL](" << this->toString() << ", "
        << rhsRpl.toString() << ")=" << (result ? "true" : "false") << "\n";
    return result;
  }
  /// Substitution
  bool substitute(StringRef from, Rpl& to) {
    llvm::errs() << "DEBUG:: before substitution: ";
    printElements(llvm::errs());
    llvm::errs() << "\n";
    /// 1. find all occurences of 'from'
    for (ElementVector::iterator
            it = rplElements.begin();
         it != rplElements.end(); it++) {
      if (!((*it).compare(from))) { 
        llvm::errs() << "DEBUG:: found '" << from 
          << "' replaced with '" ;
        to.printElements(llvm::errs());
        //size_t len = to.length();
        it = rplElements.erase(it);
        it = rplElements.insert(it, to.rplElements.begin(), to.rplElements.end());
        llvm::errs() << "' == '";
        printElements(llvm::errs());
        llvm::errs() << "'\n";
      }
    }
    llvm::errs() << "DEBUG:: after substitution: ";
    printElements(llvm::errs());
    llvm::errs() << "\n";
    return false;
  }  
  
  /// Iterator
}; // end class Rpl


// Idea Have an RplRef class, friends with Rpl to efficiently perform 
// isIncluded and isUnder tests

///-///////////////////////////////////////////////////////////////////////////
/// Effect Class

enum EffectKind {
  /// pure = no effect
  PureEffect,
  /// reads effect
  ReadsEffect,
  /// atomic reads effect
  AtomicReadsEffect,
  /// writes effect
  WritesEffect,
  /// atomic writes effect
  AtomicWritesEffect
};
#ifndef EFFECT_VECTOR_SIZE
#define EFFECT_VECTOR_SIZE 16
#endif
class Effect {
private:
  /// Fields
  EffectKind effectKind;
  Rpl* rpl;

  /// Sub-Effect Kind
  inline bool isSubEffectKindOf(Effect& e) {
    bool result = false;
    if (effectKind == PureEffect) return true; // optimization
    
    if (!e.isAtomic() || this->isAtomic()) { 
      /// if e.isAtomic ==> this->isAtomic() [[else return false]]
      switch(e.getEffectKind()) {
      case WritesEffect:
        if (effectKind == WritesEffect) result = true;
        // intentional fall through (lack of 'break')
      case AtomicWritesEffect:
        if (effectKind == AtomicWritesEffect) result = true;
        // intentional fall through (lack of 'break')
      case ReadsEffect:
        if (effectKind == ReadsEffect) result = true;
        // intentional fall through (lack of 'break')
      case AtomicReadsEffect:
        if (effectKind == AtomicReadsEffect) result = true;
        // intentional fall through (lack of 'break')
      case PureEffect:
        if (effectKind == PureEffect) result = true;
      }
    }
    return result;
  }

public:
  /// Types
  typedef llvm::SmallVector<Effect*, EFFECT_VECTOR_SIZE> EffectVector;

  /// Constructors
  Effect(EffectKind ec, Rpl* r) : effectKind(ec), rpl(r) {}
  /// Destructors
  virtual ~Effect() { 
    delete rpl;
  }
  /// Printing
  inline bool printEffectKind(raw_ostream& os) {
    bool hasRpl = true;
    switch(effectKind) {
    case PureEffect: os << "Pure Effect"; hasRpl = false; break;
    case ReadsEffect: os << "Reads Effect"; break;
    case WritesEffect: os << "Writes Effect"; break;
    case AtomicReadsEffect: os << "Atomic Reads Effect"; break;
    case AtomicWritesEffect: os << "Atomic Writes Effect"; break;
    }
    return hasRpl;
  }

  void print(raw_ostream& os) {
    bool hasRpl = printEffectKind(os);
    if (hasRpl) {
      os << " on ";
      assert(rpl && "NULL RPL in non-pure effect");
      rpl->printElements(os);
    }
  }
  /// Various
  inline bool isPureEffect() {
    return (effectKind == PureEffect) ? true : false;
  }
  
  inline bool hasRplArgument() { return !isPureEffect(); }

  std::string toString() {
    std::string sbuf;
    llvm::raw_string_ostream os(sbuf);
    print(os);
    return std::string(os.str());
  }
  
  /// Getters
  EffectKind getEffectKind() { return effectKind; }
  Rpl* getRpl() { return rpl; }

  inline bool isAtomic() { 
    return (effectKind==AtomicReadsEffect ||
            effectKind==AtomicWritesEffect) ? true : false;
  }
  /// Substitution
  inline bool substitute(StringRef from, Rpl& to) {
    if (rpl)
      return rpl->substitute(from, to);
    else 
      return true;
  }  
  
  /// SubEffect: true if this <= e
  /** 
   *  rpl1 c= rpl2   E1 c= E2
   * ~~~~~~~~~~~~~~~~~~~~~~~~~
   *    E1(rpl1) <= E2(rpl2) 
   */
  bool isSubEffectOf(Effect& e) {
    bool result = (isPureEffect() ||
            (isSubEffectKindOf(e) && rpl->isIncludedIn(*(e.rpl))));
    llvm::errs() << "DEBUG:: ~~~isSubEffect(" << this->toString() << ", "
        << e.toString() << ")=" << (result ? "true" : "false") << "\n";
    return result;
  }
  /// isCoveredBy
  bool isCoveredBy(EffectVector effectSummary) {
    for (EffectVector::const_iterator
            I = effectSummary.begin(),
            E = effectSummary.end();
            I != E; I++) {
      if (isSubEffectOf(*(*I))) return true;
    }
    return false;
  }

}; // end class Effect

///-///////////////////////////////////////////////////////////////////////////
/// Stmt Visitor Class

void destroyEffectVector(Effect::EffectVector& ev) {
  for (Effect::EffectVector::const_iterator
          it = ev.begin(),
          end = ev.end();
        it != end; it++) {
    delete (*it);
  }
}

class EffectCollectorVisitor
    : public StmtVisitor<EffectCollectorVisitor, void> {

private:
  /// Types
  typedef llvm::SmallVector<Effect*,8> TmpEffectVector;

  /// Fields
  ento::BugReporter& BR;
  ASTContext& Ctx;
  AnalysisDeclContext* AC;
  raw_ostream& os;
  bool hasWriteSemantics;
  TmpEffectVector effectsTmp;
  Effect::EffectVector effects;
  Effect::EffectVector& effectSummary;
  bool isCoveredBySummary;
  
  /// Private Methods
  void helperEmitEffectNotCoveredWarning(Stmt* S, Decl* D, const StringRef& str) {
    std::string description_std = "'";
    description_std.append(str);
    description_std.append("' effect not covered by effect summary");
    StringRef bugName = "effect not covered by effect summary";
    StringRef bugCategory = "Safe Parallelism";
    StringRef bugStr = description_std;

    //TODO get ProgramPoint for Stmt S
    PathDiagnosticLocation VDLoc =
       PathDiagnosticLocation::createBegin(S, BR.getSourceManager(), AC);

    BR.EmitBasicReport(D, bugName, bugCategory,
                       bugStr, VDLoc, S->getSourceRange());
  }

public:
  /// Constructor
  EffectCollectorVisitor (
    ento::BugReporter& BR, 
    ASTContext& Ctx, 
    AnalysisDeclContext* AC,
    raw_ostream& os, 
    Effect::EffectVector& effectsummary, 
    Stmt* stmt
    ) : BR(BR),
        Ctx(Ctx), 
        AC(AC),
        os(os), 
        hasWriteSemantics(false), 
        effectSummary(effectsummary), 
        isCoveredBySummary(true) 
  {
    //os << "DEBUG::  Starting Stmt visitor~~\n";
    stmt->printPretty(os, 0, Ctx.getPrintingPolicy());
    Visit(stmt);
    //os << "DEBUG::  Finito!\n";
  }
  
  /// Destructor
  virtual ~EffectCollectorVisitor() {
    /// free effectsTmp
    for(TmpEffectVector::const_iterator
            it = effectsTmp.begin(),
            end = effectsTmp.end();
            it != end; it++) {
      delete (*it);
    }
  }
  
  /// Getters
  inline bool getIsCoveredBySummary() { return isCoveredBySummary; }
  
  /// Visitors
  void VisitChildren(Stmt *S) {
    //os << "DEBUG:: VisitChildren\n";
    for (Stmt::child_iterator I = S->child_begin(), E = S->child_end(); I!=E; ++I)
      if (Stmt *child = *I)
        Visit(child);
  }

  void VisitStmt(Stmt *S) {
    //os << "DEBUG:: VisitStmt\n";
    VisitChildren(S);
  }

  //bool VisitMemberExpr(MemberExpr* E) {
  void VisitMemberExpr(MemberExpr* E) {
    os << "DEBUG:: VisitMemberExpr: ";
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

    ValueDecl* vd = E->getMemberDecl();
    vd->print(os, Ctx.getPrintingPolicy());
    os << "\n";

    /// 1. vd is a FunctionDecl
    if (dyn_cast<FunctionDecl>(vd)) {
      // TODO
    }
    /// 2. vd is a FieldDecl
    if (dyn_cast<FieldDecl>(vd)) {
      /// add effect reads vd
      InRegionAttr* at = vd->getAttr<InRegionAttr>();
      if (!at) {
        // 
        os << "DEBUG:: didn't find 'in' annotation for field (perhaps use default?)\n";
      }
      RegionArgAttr* arg = vd->getAttr<RegionArgAttr>();
      if (arg) {
        // apply substitution to temp effects
        StringRef s = arg->getRpl();
        Rpl* rpl = new Rpl(s, vd);
        for (TmpEffectVector::const_iterator
              it = effectsTmp.begin(),
              end = effectsTmp.end();
              it != end; it++) {
          // TODO find proper from to substitute hard-coded "P1"
          (*it)->substitute("P1", *rpl);
        }
      }
      /// TODO is this atomic or not? just ignore atomic for now
      StringRef s = at->getRpl();
      Rpl* rpl = new Rpl(s, vd);
      //r->printElements(os);
      EffectKind ec = (hasWriteSemantics) ? WritesEffect : ReadsEffect;
      Effect* e = new Effect(ec, rpl);
      bool hws = hasWriteSemantics;
      //TODO push effects on temp-stack for possible substitution
      effectsTmp.push_back(e);
      hasWriteSemantics = false;
      Visit(E->getBase());
      hasWriteSemantics = hws;
      e = effectsTmp.pop_back_val();
      os << "### "; e->print(os); os << "\n";
      //Check that effects are covered by effect summary
      if (!e->isCoveredBy(effectSummary)) {
        ///TODO produce warning
        os << "DEBUG:: effects not covered error\n";
        std::string str = e->toString();
        helperEmitEffectNotCoveredWarning(E, vd, str);
        isCoveredBySummary = false;
      }
      effects.push_back(e);
    }
  }

  //bool VisitDeclRefExpr(DeclRefExpr* E) {
  void VisitDeclRefExpr(DeclRefExpr* E) {
    os << "DEBUG:: VisitDeclRefExpr --- whatever that is!: ";
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
    ValueDecl* vd = E->getDecl();
    vd->print(os, Ctx.getPrintingPolicy());
    os << "\n";
    
    //return true;
  }
  
  /*void VisitCastExpr(CastExpr* E) {
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

    Visit(E->getSubExpr());
    //return true;
  }*/

  void VisitCompoundAssignOperator(CompoundAssignOperator* E) {
    os << "DEBUG:: !!!!!!!!!!! Mother of compound Assign!!!!!!!!!!!!!\n";
    E->printPretty(os, 0, Ctx.getPrintingPolicy());
    os << "\n";
    //Expr* lhs = E->getLHS();
    //Expr* rhs = E->getRHS();
    bool hws = this->hasWriteSemantics;
    hasWriteSemantics = true;
    Visit(E->getLHS());
    hasWriteSemantics = hws;
    Visit(E->getRHS());
    //return true;
  }

  void VisitBinAssign(BinaryOperator* E) {
    os << "DEBUG:: >>>>>>>>>>VisitBinAssign<<<<<<<<<<<<<<<<<\n";
    E->printPretty(os, 0, Ctx.getPrintingPolicy());
    os << "\n";
    bool hws = this->hasWriteSemantics;
    hasWriteSemantics = true;
    Visit(E->getLHS());
    hasWriteSemantics = hws;
    Visit(E->getRHS());
    
    //return true;
  }
}; // end class StmtVisitor
///-///////////////////////////////////////////////////////////////////////////
/// AST Traverser Class
class ASPSemanticCheckerTraverser :
  public RecursiveASTVisitor<ASPSemanticCheckerTraverser> {

private:
  // Private Members
  ento::BugReporter& BR;
  ASTContext& Ctx;
  AnalysisDeclContext* AC;
  raw_ostream& os;

  // Private Methods
  /**
   *  Emit a Warning when the input string is not a valid region name
   *  or region parameter.
   */
  void helperEmitInvalidRegionOrParamWarning(Decl* D, const StringRef& str) {
    std::string descr = "'";
    descr.append(str);
    descr.append("' invalid region or parameter name");

    StringRef bugName = "invalid region or parameter name";
    StringRef bugCategory = "Safe Parallelism";
    StringRef bugStr = descr;

    PathDiagnosticLocation VDLoc =
       PathDiagnosticLocation::create(D, BR.getSourceManager());

    BR.EmitBasicReport(D, bugName, bugCategory,
                       bugStr, VDLoc, D->getSourceRange());
  }

  /**
   *  Emit a Warning when the input string (which is assumed to be an RPL
   *  element) is not declared.
   */
  void helperEmitUndeclaredRplElementWarning(Decl* D, const StringRef& str) {
    std::string description_std = "'";
    description_std.append(str);
    description_std.append("' RPL element was not declared");
    StringRef bugName = "RPL element was not declared";
    StringRef bugCategory = "Safe Parallelism";
    StringRef bugStr = description_std;

    PathDiagnosticLocation VDLoc =
       PathDiagnosticLocation::create(D, BR.getSourceManager());

    BR.EmitBasicReport(D, bugName, bugCategory,
                       bugStr, VDLoc, D->getSourceRange());
  }

  /**
   *  Print to the debug output stream (os) the attribute
   */
  template<typename AttrType>
  inline void helperPrintAttributes(Decl* D) {
    int i = 0;
    const AttrType* attr = D->getAttr<AttrType>(i++);
     while (attr) {
      attr->printPretty(os, Ctx.getPrintingPolicy());
      os << "\n";
      attr = D->getAttr<AttrType>(i++);
    }
  }

  /**
   *  Check that the region and region parameter declarations
   *  of Declaration D are valid.
   */
  template<typename AttrType>
  bool checkRegionAndParamDecls(Decl* D) {
    int i = 0;
    bool result = true;
    const AttrType* myAttr = D->getAttr<AttrType>(i++);
    while (myAttr) {
      const StringRef str = getRegionOrParamName(myAttr);
      if (!isValidRegionName(str)) {
        // Emit bug report!
        helperEmitInvalidRegionOrParamWarning(D, str);
        result = false;
      }
      myAttr = D->getAttr<AttrType>(i++);
    }
    return result;
  }

  /**
   *  Check that the annotations of type AttrType of declaration D
   *  have RPLs whose elements have been declared
   */
  bool checkRpl(Decl*D, StringRef rpl) {
    bool result = true;
    while(rpl.size() > 0) { /// for all RPL elements of the RPL
      // FIXME: '::' can appear as part of an RPL element. Splitting must
      // be done differently to account for that.
      std::pair<StringRef,StringRef> pair = rpl.split(':');
      const StringRef& head = pair.first;
      /// head: is it a special RPL element? if not, is it declared?
      if (!isSpecialRplElement(head) && !findRegionName(D, head)) {
        // Emit bug report!
        helperEmitUndeclaredRplElementWarning(D, head);
        result = false;
      }
      rpl = pair.second;
    }
    return result;
  }


  /// AttrType must implement getRpl (i.e., InRegionAttr,
  ///                                 RegionArgAttr, & Effect Attributes)
  template<typename AttrType>
  bool checkRpls(Decl* D) {
    int i = 0;
    bool result = true;
    const AttrType* myAttr = D->getAttr<AttrType>(i++);
    while (myAttr) { /// for all attributes of type AttrType
      if (checkRpl(D, myAttr->getRpl()))
        result = false;
      myAttr = D->getAttr<AttrType>(i++);
    }
    return result;
  }

  inline EffectKind getEffectKind(const ReadsEffectAttr* attr) {
    return ReadsEffect;
  }
  inline EffectKind getEffectKind(const WritesEffectAttr* attr) {
    return WritesEffect;
  }
  inline EffectKind getEffectKind(const AtomicReadsEffectAttr* attr) {
    return AtomicReadsEffect;
  }
  inline EffectKind getEffectKind(const AtomicWritesEffectAttr* attr) {
    return AtomicWritesEffect;
  }
  
  template<typename AttrType>
  void buildPartialEffectSummary(Decl* D, Effect::EffectVector& ev) {
    int i = 0;
    //bool result = true;
    const AttrType* myAttr = D->getAttr<AttrType>(i++);
    while (myAttr) { /// for all attributes of type AttrType
      StringRef s = myAttr->getRpl();
      Rpl* rpl = new Rpl(s,D);
      EffectKind ec = getEffectKind(myAttr); // TODO
      
      Effect* e = new Effect(ec, rpl);
      ev.push_back(e);
      myAttr = D->getAttr<AttrType>(i++);
    }
    //return result;
  }

  void buildEffectSummary(Decl* D, Effect::EffectVector& ev) {   
    //bulidPartialEffectSummary<PureEffectAttr>(D, ev);
    buildPartialEffectSummary<ReadsEffectAttr>(D, ev);
    buildPartialEffectSummary<WritesEffectAttr>(D, ev);
    buildPartialEffectSummary<AtomicReadsEffectAttr>(D, ev);
    buildPartialEffectSummary<AtomicWritesEffectAttr>(D, ev);
    const PureEffectAttr* myAttr = D->getAttr<PureEffectAttr>();
    if (ev.size()==0 && myAttr) {
      Effect* e = new Effect(PureEffect, 0);
      ev.push_back(e);      
    }
    // print effect summary
    os << "DEBUG:: Effect summary: \n";
    for (Effect::EffectVector::const_iterator 
            I = ev.begin(),
            E = ev.end(); 
            I != E; I++) {
      (*I)->print(os);
      os << "\n";
    }
  }
  
public:

  typedef RecursiveASTVisitor<ASPSemanticCheckerTraverser> BaseClass;

  /// Constructor
  explicit ASPSemanticCheckerTraverser (
    ento::BugReporter& BR, ASTContext& ctx,
    AnalysisDeclContext* AC
    ) : BR(BR),
        Ctx(ctx),
        AC(AC),
        os(llvm::errs())
  {}

  bool VisitValueDecl(ValueDecl* E) {
    os << "DEBUG:: VisitValueDecl : ";
    E->print(os, Ctx.getPrintingPolicy());
    os << "\n";
    return true;
  }

  bool VisitFunctionDecl(FunctionDecl* D) {
    os << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"
       << "DEBUG:: printing ASP attributes for method or function '";
    D->getDeclName().printName(os);
    os << "':\n";
    /// A. Detect Annotations
    /// A.1. Detect Region and Parameter Declarations
    helperPrintAttributes<RegionAttr>(D);

    /// A.2. Detect Region Parameter Declarations
    helperPrintAttributes<RegionParamAttr>(D);

    /// A.3. Detect Effects
    helperPrintAttributes<PureEffectAttr>(D); /// pure
    helperPrintAttributes<ReadsEffectAttr>(D); /// reads
    helperPrintAttributes<WritesEffectAttr>(D); /// writes
    helperPrintAttributes<AtomicReadsEffectAttr>(D); /// atomic reads
    helperPrintAttributes<AtomicWritesEffectAttr>(D); /// atomic writes

    /// B. Check Annotations
    /// B.1 Check Regions & Params
    checkRegionAndParamDecls<RegionAttr>(D);
    checkRegionAndParamDecls<RegionParamAttr>(D);
    /// B.2 Check Effect RPLs
    checkRpls<ReadsEffectAttr>(D);
    checkRpls<WritesEffectAttr>(D);
    checkRpls<AtomicReadsEffectAttr>(D);
    checkRpls<AtomicWritesEffectAttr>(D);

    const FunctionDecl* Definition;
    if (D->hasBody(Definition)) {
      Stmt* st = Definition->getBody(Definition);
      assert(st);
      //os << "DEBUG:: calling Stmt Visitor\n";
      Effect::EffectVector effectSummary;
      buildEffectSummary(D, effectSummary);
      EffectCollectorVisitor ecv(BR, Ctx, AC, os, effectSummary, st);
      destroyEffectVector(effectSummary);
      //os << "DEBUG:: DONE!! \n";
    }
    
    return true;
  }

  bool VisitRecordDecl (RecordDecl* D) {
    os << "DEBUG:: printing ASP attributes for class or struct '";
    D->getDeclName().printName(os);
    os << "':\n";
    /// A. Detect Region & Param Annotations
    helperPrintAttributes<RegionAttr>(D);
    helperPrintAttributes<RegionParamAttr>(D);

    /// B. Check Region & Param Names
    checkRegionAndParamDecls<RegionAttr>(D);
    checkRegionAndParamDecls<RegionParamAttr>(D);

    return true;
  }

  bool VisitFieldDecl(FieldDecl *D) {
    os << "DEBUG:: VisitFieldDecl\n";
    /// A. Detect Region In & Arg annotations
    helperPrintAttributes<InRegionAttr>(D); /// in region
    helperPrintAttributes<RegionArgAttr>(D); /// in region

    /// B. Check RPLs
    checkRpls<InRegionAttr>(D);
    checkRpls<RegionArgAttr>(D);
    return true;
  }
  bool VisitVarDecl(VarDecl *) {
    os << "DEBUG:: VisitVarDecl\n";
    /// TODO check that any region args match any region params
    return true;
  }

  bool VisitCXXMethodDecl(clang::CXXMethodDecl *) {
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

  /*bool VisitBinAssign(BinaryOperator* E) {
    os << "DEBUG:: >>>>>>>>>>VisitBinAssign<<<<<<<<<<<<<<<<<\n";
    return true;
  }

  bool VisitBinAddAssign(const CompoundAssignOperator* E) {
    os << "DEBUG:: >>>>>>>>>>VisitBinAddAssign<<<<<<<<<<<<<<<<<\n";
    return true;
  }

  bool VisitCompoundAssignOperator(CompoundAssignOperator* E) {
    os << "DEBUG:: !!!!!!!!!!! Mother of compound Assign!!!!!!!!!!!!!\n";
    //Expr* lhs = E->getLHS();
    //Expr* rhs = E->getRHS();

    return true;
  }

  bool VisitCXXThisExpr(CXXThisExpr* E) {
    os << "DEBUG:: VisitCXXThisExpr: ";
    E->printPretty(os, 0, Ctx.getPrintingPolicy());
    os << "\n";
    return true;
  }

  bool VisitUnaryOperator(UnaryOperator* E) {
    os << "DEBUG:: VisitUnaryOperator :) :) \n";
    return true;
  }

  bool VisitUnaryExprOrTypeTraitExpr(UnaryExprOrTypeTraitExpr* E) {
    os << "DEBUG:: VisitUnaryExprOrTypeTraitExpr\n";
    return true;
  }*/

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

  bool VisitMemberExpr(MemberExpr* E) {
    os << "DEBUG:: VisitMemberExpr: ";
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

    ValueDecl* vd = E->getMemberDecl();
    vd->print(os, Ctx.getPrintingPolicy());
    os << "\n";

    /// add effect reads vd
    InRegionAttr* at = vd->getAttr<InRegionAttr>();
    /// TODO is this a read or a write? atomic or not? just go with write for now
    StringRef s = at->getRpl();
    Rpl* rpl = new Rpl(s, vd);
    //r->printElements(os);
    EffectKind ec = WritesEffect;
    Effect* e = new Effect(ec, *rpl);
    os << "### "; e->print(os); os << "\n";
    return true;
  }

  bool VisitDeclRefExpr(DeclRefExpr* E) {
    os << "DEBUG:: VisitDeclRefExpr --- whatever that is!: ";
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
    ValueDecl* vd = E->getDecl();
    vd->print(os, Ctx.getPrintingPolicy());
    os << "\n";

    return true;
  }*/

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

  /*bool VisitAssignmentExpression() {
    os << "DEBUG:: VisitAssignmentExpression\n"
    return true;
  }*/

};

class  SafeParallelismChecker
  : public Checker<check::ASTDecl<TranslationUnitDecl> > {

public:
  void checkASTDecl(const TranslationUnitDecl* D, AnalysisManager& mgr,
                    BugReporter &BR) const {
    llvm::errs() << "DEBUG:: starting ASP Semantic Checker\n";
    /** initialize traverser */
    ASPSemanticCheckerTraverser aspTraverser(BR, D->getASTContext(),
                                             mgr.getAnalysisDeclContext(D));
    /** run checker */
    aspTraverser.TraverseDecl(const_cast<TranslationUnitDecl*>(D));
    llvm::errs() << "DEBUG:: done running ASP Semantic Checker\n\n";
  }
};
} // end unnamed namespace

void ento::registerSafeParallelismChecker(CheckerManager &mgr) {
  mgr.registerChecker<SafeParallelismChecker>();
}
