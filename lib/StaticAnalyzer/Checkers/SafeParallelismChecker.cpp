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

#define ASP_DEBUG

#ifdef ASP_DEBUG
static raw_ostream& os = llvm::errs();
#else
static raw_ostream& os = llvm::nulls();
#endif

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

  inline bool isValidTypeForIn(const Type* t) {
    if (t->isScalarType()) return true;
    else return false;
  }

  inline bool isValidTypeForArg(const Type* t) {
    if (t->isAggregateType() 
        || (t->isAnyPointerType() 
            && t->getPointeeType().getTypePtr()->isAggregateType())) 
      return true;
    else 
      return false;
  }
  
  // TODO pass arg attr as parameter
  inline bool hasValidRegionParamAttr(const Type* T) { 
    bool result = false;
    if (const TagType* tt = dyn_cast<TagType>(T)) {
      const TagDecl* td = tt->getDecl();
      const RegionParamAttr* rpa = td->getAttr<RegionParamAttr>();
      // TODO check the number of params is equal to arg attr.
      if (rpa) result = true; 
    } else if (const BuiltinType* tt = dyn_cast<BuiltinType>(T)) {
      // TODO check the number of parameters of the arg attr to be 1
      result = true;
    } 
    return result;
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
    for (specific_attr_iterator<AttrType>
         i = D->specific_attr_begin<AttrType>(),
         e = D->specific_attr_end<AttrType>();
         i != e; ++i) {
      if (getRegionOrParamName(*i)==name) return true;
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
  // Idea Have an RplRef class, friends with Rpl to efficiently perform 
  // isIncluded and isUnder tests
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
      //os  << "DEBUG:: isEmpty[RplRef] returned " 
      //    << (lastIdx<firstIdx ? "true" : "false") << "\n";
      return (lastIdx<firstIdx) ? true : false;
    }
    
    bool isUnder(RplRef& rhs) {
      os  << "DEBUG:: ~~~~~~~~isUnder[RplRef](" 
          << this->toString() << ", " << rhs.toString() << ")\n";
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
      return stripLast().isUnder(rhs);
      // TODO z-regions 
    }
    
    /// Inclusion: this c= rhs
    bool isIncludedIn(RplRef& rhs) { 
      os  << "DEBUG:: ~~~~~~~~isIncludedIn[RplRef](" 
          << this->toString() << ", " << rhs.toString() << ")\n";
      if (rhs.isEmpty()) {
        /// Root c= Root    
        if (isEmpty()) return true;
        /// RPL c=? Root and RPL!=Root ==> not included      
        else /*!isEmpty()*/ return false;
      } else { /// rhs is not empty
        /// R c= R':* <==  R <= R'
        if (!rhs.getLastElement().compare("*")) {
          os <<"DEBUG:: isIncludedIn[RplRef] last elmt of RHS is '*'\n";
          return isUnder(rhs.stripLast());
        }
        ///   R:r c= R':r    <==  R <= R'
        /// R:[i] c= R':[i]  <==  R <= R'
        if (!isEmpty() && !getLastElement().compare(rhs.getLastElement()))
          return stripLast().isIncludedIn(rhs.stripLast());
        // TODO : more rules (include Param, ...)
        return false;
      }
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
        /// Don't produce an error, this should have been checked already.
        /// Instead try to proceed...
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
    os << "DEBUG:: ~~~~~ isIncludedIn[RPL](" << this->toString() << ", "
        << rhsRpl.toString() << ")=" << (result ? "true" : "false") << "\n";
    return result;
  }
  /// Substitution
  bool substitute(StringRef from, Rpl& to) {
    os << "DEBUG:: before substitution: ";
    printElements(os);
    os << "\n";
    /// 1. find all occurences of 'from'
    for (ElementVector::iterator
            it = rplElements.begin();
         it != rplElements.end(); it++) {
      if (!((*it).compare(from))) { 
        os << "DEBUG:: found '" << from 
          << "' replaced with '" ;
        to.printElements(os);
        //size_t len = to.length();
        it = rplElements.erase(it);
        it = rplElements.insert(it, to.rplElements.begin(), to.rplElements.end());
        os << "' == '";
        printElements(os);
        os << "'\n";
      }
    }
    os << "DEBUG:: after substitution: ";
    printElements(os);
    os << "\n";
    return false;
  }  
  
  /// Iterator
}; // end class Rpl


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
  inline bool isSubEffectKindOf(const Effect& e) const {
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
  inline bool printEffectKind(raw_ostream& os) const {
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

  void print(raw_ostream& os) const {
    bool hasRpl = printEffectKind(os);
    if (hasRpl) {
      os << " on ";
      assert(rpl && "NULL RPL in non-pure effect");
      rpl->printElements(os);
    }
  }
  /// Various
  inline bool isPureEffect() const {
    return (effectKind == PureEffect) ? true : false;
  }
  
  inline bool hasRplArgument() const { return !isPureEffect(); }

  std::string toString() const {
    std::string sbuf;
    llvm::raw_string_ostream os(sbuf);
    print(os);
    return std::string(os.str());
  }
  
  /// Getters
  EffectKind getEffectKind() const { return effectKind; }
  Rpl* getRpl() { return rpl; }

  inline bool isAtomic() const { 
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
  bool isSubEffectOf(const Effect& e) const {
    bool result = (isPureEffect() ||
            (isSubEffectKindOf(e) && rpl->isIncludedIn(*(e.rpl))));
    os  << "DEBUG:: ~~~isSubEffect(" << this->toString() << ", "
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
    stmt->printPretty(os, 0, Ctx.getPrintingPolicy());
    Visit(stmt);
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
    for (Stmt::child_iterator I = S->child_begin(), E = S->child_end(); I!=E; ++I)
      if (Stmt *child = *I)
        Visit(child);
  }

  void VisitStmt(Stmt *S) {
    VisitChildren(S);
  }

  void VisitMemberExpr(MemberExpr* E) {
    os << "DEBUG:: VisitMemberExpr: ";
    E->printPretty(os, 0, Ctx.getPrintingPolicy());
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

    ValueDecl* vd = E->getMemberDecl();
    vd->print(os, Ctx.getPrintingPolicy());
    os << "\n";

    /// 1. vd is a FunctionDecl
    if (dyn_cast<FunctionDecl>(vd)) {
      // TODO
    }
    /// 2. vd is a FieldDecl
    /// Type_vd <args> vd in RPL
    const FieldDecl* fd  = dyn_cast<FieldDecl>(vd);    
    if (fd) {
      /// 2.1. apply substitutions
      const RegionArgAttr* arg = fd->getAttr<RegionArgAttr>();
      if (arg) {
        const RecordDecl* rd = fd->getParent();
        const RegionParamAttr* rpa = rd->getAttr<RegionParamAttr>();
        assert(rpa);
        StringRef from = rpa->getParam_name();
        // apply substitution to temp effects
        StringRef s = arg->getRpl();
        Rpl* rpl = new Rpl(s, vd);
        for (TmpEffectVector::const_iterator
              it = effectsTmp.begin(),
              end = effectsTmp.end();
              it != end; it++) {
          (*it)->substitute(from, *rpl);
        }
      }
      /// 2.2. Push partially built effect onto temp stack
      StringRef s;
      Effect* e = 0;
      // TODO if basic type or pointer or reference use 'in' clause
      const InRegionAttr* at = fd->getAttr<InRegionAttr>();
      if (at) {
        s = at->getRpl();
        // TODO else use arg clause 
        /// TODO is this atomic or not? just ignore atomic for now
        Rpl* rpl = new Rpl(s, vd);
        EffectKind ec = (hasWriteSemantics) ? WritesEffect : ReadsEffect;
        e = new Effect(ec, rpl);
        // push effects on temp-stack for possible substitution
      }
      if (e) effectsTmp.push_back(e);
      
      /// 2.3. Visit Base with read semantics, then restore write semantics
      bool hws = hasWriteSemantics;
      hasWriteSemantics = false;
      Visit(E->getBase());
      hasWriteSemantics = hws;
 
      /// Post-Visit Actions: check that effects (after substitution) 
      /// are covered by effect summary
      if (e) {
        e = effectsTmp.pop_back_val();
        os << "### "; e->print(os); os << "\n";
        if (!e->isCoveredBy(effectSummary)) {
          std::string str = e->toString();
          helperEmitEffectNotCoveredWarning(E, vd, str);
          isCoveredBySummary = false;
        }
      }
      /// Add effect to this function's effect set (why?)
      //effects.push_back(e);
    }
  }

  void VisitDeclRefExpr(DeclRefExpr* E) {
    os << "DEBUG:: VisitDeclRefExpr --- whatever that is!: ";
    E->printPretty(os, 0, Ctx.getPrintingPolicy());
    os << "\n";
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
  }
  
  // TODO ++ etc operators
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
   *
   */
  void helperEmitInvalidTypeForAttr(const Decl* D, 
                                  const Attr* attr, const StringRef& str) {
    std::string attrType = "";
    if (isa<InRegionAttr>(attr)) attrType = "in";
    else if (isa<RegionArgAttr>(attr)) attrType = "arg";
    else {
      llvm::errs() << "Safe Parallelism Checker Internal Error: "
          << "called 'helperEmitInvalidTypeForAttr' on invalid attribute\n";
      return;
    }
    std::string bugName = "invalid type for '";
    bugName.append(attrType);
    bugName.append("' attribute");
    
    std::string description_std = "'";
    description_std.append(str);
    description_std.append("' ");
    description_std.append(bugName);
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
    for (specific_attr_iterator<AttrType>
         i = D->specific_attr_begin<AttrType>(),
         e = D->specific_attr_end<AttrType>();
         i != e; ++i) {
      (*i)->printPretty(os, Ctx.getPrintingPolicy());
      os << "\n";      
    }
  }

  /**
   *  Check that the region and region parameter declarations
   *  of Declaration D are valid.
   */
  template<typename AttrType>
  bool checkRegionAndParamDecls(Decl* D) {
    bool result = true;
    for (specific_attr_iterator<AttrType>
         i = D->specific_attr_begin<AttrType>(),
         e = D->specific_attr_end<AttrType>();
         i != e; ++i) {
      const StringRef str = getRegionOrParamName(*i);
      if (!isValidRegionName(str)) {
        // Emit bug report!
        helperEmitInvalidRegionOrParamWarning(D, str);
        result = false;
      }
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
    bool result = true;
    for (specific_attr_iterator<AttrType>
         i = D->specific_attr_begin<AttrType>(),
         e = D->specific_attr_end<AttrType>();
         i != e; ++i) {
      if (!checkRpl(D, (*i)->getRpl()))
        result = false;
    }
    return result;
  }

  inline EffectKind getEffectKind(const PureEffectAttr* attr) {
    return PureEffect;
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
    for (specific_attr_iterator<AttrType>
         i = D->specific_attr_begin<AttrType>(),
         e = D->specific_attr_end<AttrType>();
         i != e; ++i) {
      EffectKind ec = getEffectKind(*i); 
      Rpl* rpl = 0; // TODO: I would like to be able to call this on 
                    // PureEffectAttr as well, but the compiler complains
                    // that such attributes don't have a getRpl method...
      if (!dyn_cast<PureEffectAttr>(*i)) rpl = new Rpl((*i)->getRpl(), D);
      ev.push_back(new Effect(ec, rpl));
    }
  }

  void buildEffectSummary(Decl* D, Effect::EffectVector& ev) {   
    //buildPartialEffectSummary<PureEffectAttr>(D, ev);
    buildPartialEffectSummary<ReadsEffectAttr>(D, ev);
    buildPartialEffectSummary<WritesEffectAttr>(D, ev);
    buildPartialEffectSummary<AtomicReadsEffectAttr>(D, ev);
    buildPartialEffectSummary<AtomicWritesEffectAttr>(D, ev);
    if (D->getAttr<PureEffectAttr>()) {
      Effect* e = new Effect(PureEffect, 0);
      ev.push_back(e);      
    }
  }
  
  void checkEffectSummary(Decl* D, Effect::EffectVector& ev) {
    Effect::EffectVector::iterator I = ev.begin(); // not a const iterator
    while (I != ev.end()) { // ev.end() is not loop invariant
      const Effect* e = *I;
      bool found = false;
      for (Effect::EffectVector::iterator 
            J = ev.begin(); J != ev.end(); J++) {
        if (I != J && e->isSubEffectOf(*(*J))) {
          // warning: e is covered by *J
          StringRef bugName = "effect summary is not minimal";
          std::string sbuf;
          llvm::raw_string_ostream strbuf(sbuf);
          strbuf << "'"; e->print(strbuf);
          strbuf << "' covered by '";
          (*J)->print(strbuf); strbuf << "': ";
          strbuf << bugName;

          StringRef bugCategory = "Safe Parallelism";
          StringRef bugStr = strbuf.str();

          PathDiagnosticLocation VDLoc =
            PathDiagnosticLocation::create(D, BR.getSourceManager());
          
          BR.EmitBasicReport(D, bugName, bugCategory,
                   bugStr, VDLoc, D->getSourceRange());
          found = true;
          break;
        } // end if
        //if (found) break; // found one effect that covers this, so stop looking.
      } // end inner for loop
      /// optimization: remove e from effect Summary
      if (found) I = ev.erase(I);
      else       I++;
    } // end while loop
  } 
  
  void printEffectSummary(Effect::EffectVector& ev, raw_ostream& os) {
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
    AnalysisDeclContext* AC, raw_ostream& os
    ) : BR(BR),
        Ctx(ctx),
        AC(AC),
        os(os)
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

    /// C. Check effect summary
    const FunctionDecl* Definition;
    if (D->hasBody(Definition)) {
      Stmt* st = Definition->getBody(Definition);
      assert(st);
      //os << "DEBUG:: calling Stmt Visitor\n";
      /// C.1. Build Effect Summary
      Effect::EffectVector effectSummary;
      buildEffectSummary(D, effectSummary);
      os << "Effect Summary from annotation:\n";
      printEffectSummary(effectSummary, os);

      /// C.2. Check Effect Summary is consistent and minimal
      checkEffectSummary(D, effectSummary);
      os << "Minimal Effect Summary:\n";
      printEffectSummary(effectSummary, os);
      
      /// C.3. Check Effect Summary covers method effects
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
    
    /// C. Check validity of annotations
    /// C.1. 'in' clause only applies to basic types.
    if (const InRegionAttr* attr = D->getAttr<InRegionAttr>()) {
      const Type *t = D->getType().getTypePtr();
      if (!isValidTypeForIn(t)) {
        std::string sbuf;
        llvm::raw_string_ostream strbuf(sbuf);
        D->getType().print(strbuf, Ctx.getPrintingPolicy());
        helperEmitInvalidTypeForAttr(D, attr, strbuf.str());
      }
    }
    /// C.2. 'arg' clause only applies to aggregate types (class, struct,
    ///      union) and must have the same number of region parameters.
    if (const RegionArgAttr* attr = D->getAttr<RegionArgAttr>()) {
      const Type *t = D->getType().getTypePtr();
      if (t->isPointerType()) t->getPointeeType(); 
            // FIXME also support pointer to pointer to pointer...
            // Note right now we support the syntax of single pointer with 
            // an 'in' annotation for the pointer and an 'arg' one for the 
            // base type.
      if (!isValidTypeForArg(t) // FIXME
          && hasValidRegionParamAttr(t)) { //FIXME
        std::string sbuf;
        llvm::raw_string_ostream strbuf(sbuf);
        D->getType().print(strbuf, Ctx.getPrintingPolicy());
        helperEmitInvalidTypeForAttr(D, attr, strbuf.str());
      }
    }
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
    os << "DEBUG:: starting ASP Semantic Checker\n";
    /** initialize traverser */
    ASPSemanticCheckerTraverser aspTraverser(BR, D->getASTContext(),
                                             mgr.getAnalysisDeclContext(D), os);
    /** run checker */
    aspTraverser.TraverseDecl(const_cast<TranslationUnitDecl*>(D));
    os << "DEBUG:: done running ASP Semantic Checker\n\n";
  }
};
} // end unnamed namespace

void ento::registerSafeParallelismChecker(CheckerManager &mgr) {
  mgr.registerChecker<SafeParallelismChecker>();
}
