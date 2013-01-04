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
#include "clang/AST/Attr.h"
#include "clang/AST/StmtVisitor.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/CheckerManager.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

#include <typeinfo>

using namespace clang;
using namespace ento;

// The following definition selects code that uses the 
// specific_attr_reverse_iterator functionality. This option is kept around
// because I suspect *not* using it may actually perform better... The
// alternative being to build an llvm::SmallVector using the fwd iterator,
// then traversing it through the reverse iterator. The reason this might be
// preferable is because after building it, the SmallVector is 'reverse 
// iterated' multiple times.
#define ATTR_REVERSE_ITERATOR_SUPPORTED

#define ASAP_DEBUG
#define ASAP_DEBUG_VERBOSE2

#ifdef ASAP_DEBUG
static raw_ostream& os = llvm::errs();
#else
static raw_ostream& os = llvm::nulls();
#endif

#ifdef ASAP_DEBUG_VERBOSE2
static raw_ostream& osv2 = llvm::errs();
#else
static raw_ostream& osv2 = llvm::nulls();
#endif

/// FIXME temporarily just using pre-processor to concatenate code here... UGLY 
#include "asap/asap-common.cpp"

namespace {
  RegionParamAttr *BuiltinDefaulrRegionParam;

  // FIXME
  inline bool isValidTypeForArg(const QualType qt, const RegionArgAttr* ra) {
    bool result = true;
    // TODO what about function pointers, incomplete types, ...
    if (qt->isAggregateType()) {
      // TODO is the number of args the same as that of the params on the decl.
    }
    return result;
  }
  
  // TODO pass arg attr as parameter
  /*
  inline bool hasValidRegionParamAttr(const Type* T) { 
    bool result = false;
    if (const TagType* tt = dyn_cast<TagType>(T)) {
      const TagDecl* td = tt->getDecl();
      const RegionParamAttr* rpa = td->getAttr<RegionParamAttr>();
      // TODO check the number of params is equal to arg attr.
      if (rpa) result = true; 
    } else if (T->isBuiltinType() || T->isPointerType()) {
      // TODO check the number of parameters of the arg attr to be 1
      result = true;
    } 
    return result;
  }*/

  // FIXME
  inline const RegionParamAttr* getRegionParamAttr(const Type* T) { 
    const RegionParamAttr* result = 0; // null
    if (const TagType* tt = dyn_cast<TagType>(T)) {
      const TagDecl* td = tt->getDecl();
      result = td->getAttr<RegionParamAttr>();
      // TODO check the number of params is equal to arg attr.
    } else if (T->isBuiltinType() || T->isPointerType()) {
      // TODO check the number of parameters of the arg attr to be 1
      result = BuiltinDefaulrRegionParam;
    } /// else result = NULL;
    return result;
  }

  /**
   * Return the string name of the region or region parameter declaration
   * based on the Kind of the Attribute (RegionAttr or RegionParamAttr)
   */
  // FIXME
  inline
  StringRef getRegionOrParamName(const Attr* attr) {
    StringRef result = "";
    attr::Kind kind = attr->getKind();
    switch(kind) {
    case attr::Region:
      result = dyn_cast<RegionAttr>(attr)->getName(); break;
    case attr::RegionParam:
      result = dyn_cast<RegionParamAttr>(attr)->getName(); break;
    default:
      result = "";
    }
    return result;
  }

  inline
  RplElement* getRegionOrParamElement(const Attr* attr) {
    RplElement* result = 0;
    attr::Kind kind = attr->getKind();
    switch(kind) {
    case attr::Region:
      result = new NamedRplElement(dyn_cast<RegionAttr>(attr)->getName()); 
      break;
    case attr::RegionParam:
      result = new ParamRplElement(dyn_cast<RegionParamAttr>(attr)->getName());
      break;
    default:
      result = 0;
    }
    return result;
  }

  /**
   *  Return true if any of the attributes of type AttrType of the 
   *  declaration Decl* D have a region name or a param name that is 
   *  the same as the 'name' provided as the second argument
   */
  template<typename AttrType>
  bool scanAttributesForRegionDecl(Decl* D, const StringRef& name)
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
   *  Return an RplElement if any of the attributes of type AttrType of the 
   *  declaration Decl* D have a region name or a param name that is 
   *  the same as the 'name' provided as the 2nd argument
   */
  template<typename AttrType>
  RplElement* scanAttributes(Decl* D, const StringRef& name)
  {
    for (specific_attr_iterator<AttrType>
         i = D->specific_attr_begin<AttrType>(),
         e = D->specific_attr_end<AttrType>();
         i != e; ++i) {
      if (getRegionOrParamName(*i)==name) {
        return getRegionOrParamElement(*i);
      }
    }
    return 0;
  }

  /**
   *  Looks for 'name' in the declaration 'D' and its parent scopes
   */
  RplElement* findRegionName(Decl* D, StringRef name) {
    if (!D) return 0;
    /// 1. try to find among regions or region parameters of function
    RplElement* result = scanAttributes<RegionAttr>(D,name);
    if (!result) result = scanAttributes<RegionParamAttr>(D,name);
    if (result) return result;

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
      } else if (dc->isNamespace()) {
        NamespaceDecl *nd = dyn_cast<NamespaceDecl>(dc);
        assert(nd);
        return findRegionName(nd, name);
      } else {
        /// no ASaP annotations on other types of declarations
        dc = dc->getParent();
      }
    }
    return 0;
  }




typedef std::map<FunctionDecl*, Effect::EffectVector*> EffectSummaryMapTy;
typedef std::map<Attr*, Rpl*> RplAttrMapTy;
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

  bool typecheckAssignment;
  bool hasWriteSemantics;
  bool isBase;
  int nDerefs;
  
  Effect::EffectVector effectsTmp;
  Effect::EffectVector effects;
  Effect::EffectVector& effectSummary;
  EffectSummaryMapTy& effectSummaryMap;
  RplAttrMapTy& rplAttrMap;
  Rpl::RplVector *lhsRegions, *rhsRegions, *tmpRegions;
  bool isCoveredBySummary;
  
  /// Private Methods
  void helperEmitEffectNotCoveredWarning(Stmt* S, Decl* D, const StringRef& str) {
    StringRef bugName = "effect not covered by effect summary";
    std::string description_std = "'";
    description_std.append(str);
    description_std.append("' ");
    description_std.append(bugName);
    
    StringRef bugCategory = "Safe Parallelism";
    StringRef bugStr = description_std;

    PathDiagnosticLocation VDLoc =
       PathDiagnosticLocation::createBegin(S, BR.getSourceManager(), AC);

    BR.EmitBasicReport(D, bugName, bugCategory,
                       bugStr, VDLoc, S->getSourceRange());
  }

  void helperEmitInvalidAliasingModificationWarning(Stmt* S, Decl* D, const StringRef& str) {
    StringRef bugName = "cannot modify aliasing through pointer to partly specified region";
    std::string description_std = "'";
    description_std.append(str);
    description_std.append("' ");
    description_std.append(bugName);
    
    StringRef bugCategory = "Safe Parallelism";
    StringRef bugStr = description_std;

    PathDiagnosticLocation VDLoc =
       PathDiagnosticLocation::createBegin(S, BR.getSourceManager(), AC);

    BR.EmitBasicReport(D, bugName, bugCategory,
                       bugStr, VDLoc, S->getSourceRange());
  }

  void helperEmitInvalidAssignmentWarning(Stmt* S, Rpl* lhs, Rpl* rhs) {
    StringRef bugName = "invalid assignment";

    std::string description_std = "RHS region '";
    description_std.append(rhs?rhs->toString():"");
    description_std.append("' is not included in LHS region '");
    description_std.append(lhs?lhs->toString():"");
    description_std.append("' ");
    description_std.append(bugName);

    StringRef bugCategory = "Safe Parallelism";
    StringRef bugStr = description_std;

    PathDiagnosticLocation VDLoc =
       PathDiagnosticLocation::createBegin(S, BR.getSourceManager(), AC);
    
    BugType *BT = new BugType(bugName, bugCategory);
    BugReport *R = new BugReport(*BT, bugStr, VDLoc);
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
        isCoveredBySummary(true) 
  {
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
    const FunctionDecl* fund = dyn_cast<FunctionDecl>(vd);
    if (fund) {
      // TODO
      Effect* e = 0; // TODO here we may have a long list of effects

      /// find declaration -> find parameter(s) -> 
      /// find argument(s) -> substitute
      const RegionParamAttr* param = fund->getAttr<RegionParamAttr>();
      if (param) { 
        /// if function has region params, find the region args on 
        /// the invokation
        os << "DEBUG:: found function param";
        param->printPretty(os, Ctx.getPrintingPolicy());
        os << "\n";
        
      } else {
        /// no params
        os << "DEBUG:: didn't find function param\n";
        
      }
      
      /// parameters read after substitution, invoke effects after substitution
      /// 
      /// return type
      
      /// TODO Merge with FieldDecl (Duplicate code)
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

    }
    /// 2. vd is a FieldDecl
    /// Type_vd <args> vd 
    const FieldDecl* fd  = dyn_cast<FieldDecl>(vd);    
    if (fd) {
      StringRef s;
      int effectNr = 0;
#ifdef ATTR_REVERSE_ITERATOR_SUPPORTED
      specific_attr_reverse_iterator<RegionArgAttr>
        argit = fd->specific_attr_rbegin<RegionArgAttr>(),
        endit = fd->specific_attr_rend<RegionArgAttr>();
#else
      /// Build a reverse iterator over RegionArgAttr
      llvm::SmallVector<RegionArgAttr*, 8> argv;
      for (specific_attr_iterator<RegionArgAttr> 
              i = fd->specific_attr_begin<RegionArgAttr>(),
              e = fd->specific_attr_end<RegionArgAttr>();
          i!=e; i++){
          argv.push_back(*i);
      }    
      llvm::SmallVector<RegionArgAttr*, 8>::reverse_iterator 
        argit = argv.rbegin(),
        endit = argv.rend();
      // Done preparing reverse iterator
#endif
      // TODO if (!arg) arg = some default
      assert(argit!=endit);
      if (isBase) {
        /// 2.1 Substitution
        RegionArgAttr* substarg;
        QualType substqt = fd->getType();
        
        os << "DEBUG:: nDerefs = " << nDerefs << "\n";
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
        ParamRplElement* fromElmt = new ParamRplElement(from);
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
            (*it)->substitute(*fromElmt, *toRpl);
          }
          /// 2.1.2 Substitution of Regions (for typechecking)
          if (typecheckAssignment) {
            for (Rpl::RplVector::const_iterator
                    it = tmpRegions->begin(),
                    end = tmpRegions->end();
                  it != end; it++) {
              (*it)->substitute(*fromElmt, *toRpl);
            }
          }
        }
      } else if (typecheckAssignment) { // isBase == false ==> init regions
#ifdef ATTR_REVERSE_ITERATOR_SUPPORTED
        specific_attr_reverse_iterator<RegionArgAttr>
            i = fd->specific_attr_rbegin<RegionArgAttr>(),
            e = fd->specific_attr_rend<RegionArgAttr>();
#else
        llvm::SmallVector<RegionArgAttr*, 8>::reverse_iterator 
            i = argv.rbegin(),
            e = argv.rend();
#endif

        /// 2.1.2.1 drop region args that are sidestepped by dereferrences.
        int ndrfs = nDerefs; 
        while (ndrfs>0 && i != e) {
          i++;
          ndrfs--;
        }

        assert(ndrfs==0 || ndrfs==-1);

        if (ndrfs==0) { /// skip the 1st arg, then add the rest
          assert(i!=e);
          Rpl *tmp = rplAttrMap[(*i)];
          assert(tmp);
          if (hasWriteSemantics && nDerefs > 0
              && tmp->isFullySpecified() == false
              /* FIXME: && (*i)->/isaPointerType/ )*/) {
            /// emit capture error
            std::string str = tmp->toString();
            helperEmitInvalidAliasingModificationWarning(E,vd,str);
          }
          
          i++;
        }
        
        /// add rest of region types
        while(i!=e) {
          RegionArgAttr *arg = *i;
          Rpl *rpl = rplAttrMap[arg];
          assert(rpl);
          tmpRegions->push_back(new Rpl(rpl)); // make a copy because rpl may
                                               // be modified by substitution.
          os << "DEBUG:: adding RPL for typechecking ~~~~~~~~~~ " 
             << rpl->toString() << "\n";
          i++; 
        }
      }
      
      /// 2.2 Collect Effects
      os << "DEBUG:: isBase = " << (isBase ? "true" : "false") << "\n";
      if (nDerefs<0) { // nDeref<0 ==> AddrOf Taken
        // Do nothing. Aliasing captured by type-checker
      } else { // nDeref >=0
        /// 2.2.1. Take care of dereferences first
#ifdef ATTR_REVERSE_ITERATOR_SUPPORTED
        specific_attr_reverse_iterator<RegionArgAttr>
            argit = fd->specific_attr_rbegin<RegionArgAttr>(),
            endit = fd->specific_attr_rend<RegionArgAttr>();
#else
        llvm::SmallVector<RegionArgAttr*, 8>::reverse_iterator 
                argit = argv.rbegin(),
                endit = argv.rend();
#endif
        QualType qt = fd->getType();
        
        for (int i = nDerefs; i>0; i--) {
          assert(qt->isPointerType());
          assert(argit!=endit);
          /// TODO is this atomic or not? ignore atomic for now
          effectsTmp.push_back(
                  new Effect(ReadsEffect, 
                             new Rpl(rplAttrMap[(*argit)]), 
                             *argit));
                  
          effectNr++;
           
          qt = qt->getPointeeType();
          argit++;
        }
        /// 2.2.2 Take care of the destination of dereferrences, unless this 
        /// is the base of a member expression. In that case, the '.' operator 
        /// describes the offset from the base, and the substitution performed 
        /// in earlier in 2.1 takes care of that offset.
        assert(argit!=endit);
        if (!isBase) {
          /// TODO is this atomic or not? just ignore atomic for now
          EffectKind ec = (hasWriteSemantics) ? WritesEffect : ReadsEffect;
          // if it is an aggregate type we have to capture all the copy effects
          // at most one of isAddrOf and isDeref can be true
          // last type to work on
          if (fd->getType()->isStructureOrClassType()) {
            // TODO for each field add effect & i++
            /// Actually this translates into an implicit call to an 
            /// implicit copy function... treat it as a function call.
            /// i.e., not here!
          } else {
            effectsTmp.push_back(
                      new Effect(ec, new Rpl(rplAttrMap[(*argit)]), *argit));
            effectNr++;
          }
        }
      }
      
      /// 2.3. Visit Base with read semantics, then restore write semantics
      bool saved_hws = hasWriteSemantics;
      bool saved_isBase = isBase; // probably not needed to save
      
      nDerefs = E->isArrow() ? 1 : 0;
      hasWriteSemantics = false;
      isBase = true;
      Visit(E->getBase());
      
      /// Post visitation checking
      hasWriteSemantics = saved_hws;
      isBase = saved_isBase; 
           
      /// Post-Visit Actions: check that effects (after substitutions) 
      /// are covered by effect summary
      while (effectNr) {
        Effect* e = effectsTmp.pop_back_val();
        os << "### "; e->print(os); os << "\n";
        if (!e->isCoveredBy(effectSummary)) {
          std::string str = e->toString();
          helperEmitEffectNotCoveredWarning(E, vd, str);
          isCoveredBySummary = false;
        }
        effectNr--;
      }
    } // end if FieldDecl
    
  } // end VisitMemberExpr

  void VisitUnaryAddrOf(UnaryOperator* E)  {
    assert(nDerefs>=0);
    nDerefs--;
    os << "DEBUG:: Visit Unary: AddrOf (nDerefs=" << nDerefs << ")\n";
    Visit(E->getSubExpr());
  }

  void VisitUnaryDeref(UnaryOperator* E) {
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
  void VisitDeclRefExpr(DeclRefExpr* E) {
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
  
  void VisitCXXThisExpr(CXXThisExpr* E) {
    os << "DEBUG:: visiting 'this' expression\n";
    nDerefs = 0;
    if (tmpRegions && tmpRegions->empty() && !E->isImplicit()) {
      // Add parameter as implicit argument
      CXXRecordDecl* rd = const_cast<CXXRecordDecl*>(E->getBestDynamicClassType());
      assert(rd);
      /// If the declaration does not yet have an implicit region argument
      /// add it to the Declaration
      if (!rd->getAttr<RegionArgAttr>()) {
        const RegionParamAttr *param = rd->getAttr<RegionParamAttr>();
        assert(param);
        RegionArgAttr *arg = 
                  ::new (rd->getASTContext()) 
                        RegionArgAttr(param->getRange(), 
                                      rd->getASTContext(), param->getName());
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
  
  void VisitCompoundAssignOperator(CompoundAssignOperator* E) {
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
  
  void VisitCallExpr(CallExpr* E) { 
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
  
};

class ASaPSemanticCheckerTraverser :
  public RecursiveASTVisitor<ASaPSemanticCheckerTraverser> {
  
private:
  /// Private Fields
  ento::BugReporter& BR;
  ASTContext& Ctx;
  AnalysisDeclContext* AC;
  raw_ostream& os;
  bool fatalError;
  EffectSummaryMapTy& effectSummaryMap;
  RplAttrMapTy& rplAttrMap;

  /// Private Methods
  /**
   *  Issues Warning: '<str>' <bugName> on Declaration
   */
  void helperEmitDeclarationWarning(const Decl* D, 
                                  const StringRef& str, std::string bugName) { 

    std::string description_std = "'";
    description_std.append(str);
    description_std.append("' ");
    description_std.append(bugName);
    StringRef bugCategory = "Safe Parallelism";
    StringRef bugStr = description_std;

    PathDiagnosticLocation VDLoc(D->getLocation(), BR.getSourceManager());
    BR.EmitBasicReport(D, bugName, bugCategory,
                       bugStr, VDLoc, D->getSourceRange());    

  }

  /**
   *  Issues Warning: '<str>' <bugName> on Attribute
   */
  void helperEmitAttributeWarning(const Decl* D, 
                                  const Attr* attr, 
                                  const StringRef& str, std::string bugName) { 

    std::string description_std = "'";
    description_std.append(str);
    description_std.append("' ");
    description_std.append(bugName);
    StringRef bugCategory = "Safe Parallelism";
    StringRef bugStr = description_std;

    PathDiagnosticLocation VDLoc(attr->getLocation(), BR.getSourceManager());
    BR.EmitBasicReport(D, bugName, bugCategory,
                       bugStr, VDLoc, attr->getRange());    

  }

  /**
   *  Emit a Warning when the input string (which is assumed to be an RPL
   *  element) is not declared.
   */
  void helperEmitUndeclaredRplElementWarning(
                        const Decl* D, 
                        const Attr* attr, 
                        const StringRef& str) {
    StringRef bugName = "RPL element was not declared";
    helperEmitAttributeWarning(D,attr,str, bugName);
  }

  /** 
   *  Declaration D is missing region argument(s)
   */
  void helperEmitMissingRegionArgs(Decl* D) {
    std::string bugName = "missing region argument(s)";

    std::string sbuf;
    llvm::raw_string_ostream strbuf(sbuf);
    D->print(strbuf, Ctx.getPrintingPolicy());
      
    helperEmitDeclarationWarning(D, strbuf.str(), bugName);    
  }
  /**
   *  Region argument A on declaration D is superfluous for type of D
   */
  void helperEmitSuperfluousRegionArg(Decl* D, RegionArgAttr* A) {
    std::string bugName = "superfluous region argument";

    std::string sbuf;
    llvm::raw_string_ostream strbuf(sbuf);
    A->printPretty(strbuf, Ctx.getPrintingPolicy());
          
    helperEmitAttributeWarning(D, A, strbuf.str(), bugName);
  }

  void checkIsValidTypeForArg(Decl* D, QualType qt, RegionArgAttr* attr) {
    if (!isValidTypeForArg(qt, attr)) {
      // Emit Warning: Incompatible arg annotation TODO
      std::string bugName = "wrong number of region parameter arguments";

      std::string sbuf;
      llvm::raw_string_ostream strbuf(sbuf);
      attr->printPretty(strbuf, Ctx.getPrintingPolicy());
          
      helperEmitAttributeWarning(D, attr, strbuf.str(), bugName);
    }
  }

  /**
   *  check that the Type T of Decl D has the proper region arg annotations
   *  Types of errors to find
   *  1. Too many arg annotations for type
   *  2. Incompatible arg annotation for type (invalid #of RPLs)
   *  3. Not enough arg annotations for type
   */
  void checkTypeRegionArgs(ValueDecl* D) {
    QualType qt = D->getType();
    // here we need a reverse iterator over RegionArgAttr
#ifdef ATTR_REVERSE_ITERATOR_SUPPORTED
    specific_attr_reverse_iterator<RegionArgAttr>
      i = D->specific_attr_rbegin<RegionArgAttr>(),
      e = D->specific_attr_rend<RegionArgAttr>();
#else
    llvm::SmallVector<RegionArgAttr*, 8> argv;
    for (specific_attr_iterator<RegionArgAttr> 
            i = D->specific_attr_begin<RegionArgAttr>(),
            e = D->specific_attr_end<RegionArgAttr>();
         i!=e; i++){
        argv.push_back(*i);
    }    
    llvm::SmallVector<RegionArgAttr*, 8>::reverse_iterator 
      i = argv.rbegin(),
      e = argv.rend();
#endif    
    while (i!=e && qt->isPointerType()) {
      checkIsValidTypeForArg(D, qt, *i);      
      i++;
      qt = qt->getPointeeType(); 
    }
    if (i==e) {
      // TODO attach default annotations
      //helperEmitMissingRegionArgs(D);
    } else {
      checkIsValidTypeForArg(D, qt, *i);
      i++;
    }
    
    // check if there are annotations left
    while (i!=e) {
      helperEmitSuperfluousRegionArg(D, *i);
      fatalError = true;
      i++;
    }
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
  bool checkRegionOrParamDecls(Decl* D) {
    bool result = true;
    for (specific_attr_iterator<AttrType>
         i = D->specific_attr_begin<AttrType>(),
         e = D->specific_attr_end<AttrType>();
         i != e; ++i) {
      //const Attr* attr = *i;
      const StringRef str = getRegionOrParamName(*i);
      if (!isValidRegionName(str)) {
        // Emit bug report!
        std::string attrType = "";
        if (isa<RegionAttr>(*i)) attrType = "region";
        else if (isa<RegionParamAttr>(*i)) attrType = "region parameter";
        std::string bugName = "invalid ";
        bugName.append(attrType);
        bugName.append(" name");
        
        helperEmitAttributeWarning(D, *i, str, bugName);
        //SourceLocation loc = attr->getLocation();
        result = false;
      }
    }
    return result;
  }

  /**
   *  Check that the annotations of type AttrType of declaration D
   *  have RPLs whose elements have been declared
   */
  Rpl* checkRpl(Decl*D, Attr* attr, StringRef rpl_str) {
    if (rplAttrMap[attr]) return rplAttrMap[attr];
    /// else
    bool result = true;
    Rpl *rpl = new Rpl();
    
    while(rpl_str.size() > 0) { /// for all RPL elements of the RPL
      // FIXME: '::' can appear as part of an RPL element. Splitting must
      // be done differently to account for that.
      std::pair<StringRef,StringRef> pair = rpl_str.split(Rpl::RPL_SPLIT_CHARACTER);
      const StringRef& head = pair.first;
      /// head: is it a special RPL element? if not, is it declared?
      const RplElement *rplEl = getSpecialRplElement(head);
      if (!rplEl) rplEl = findRegionName(D,head);
      if (!rplEl) {
        // Emit bug report!
        helperEmitUndeclaredRplElementWarning(D, attr, head);
        result = false;
      } else {
        rpl->appendElement(rplEl);
      }
      rpl_str = pair.second;
    }
    if (result == false) {
      delete(rpl);
      return 0;
    } else {
      rplAttrMap[attr] = rpl;
      return rpl;
    }
  }


  /// AttrType must implement getRpl (i.e., RegionArgAttr, & Effect Attributes)
  template<typename AttrType>
  bool checkRpls(Decl* D) {
    bool result = true;
    
    for (specific_attr_iterator<AttrType>
         i = D->specific_attr_begin<AttrType>(),
         e = D->specific_attr_end<AttrType>();
         i != e; ++i) {
           
      if (!checkRpl(D, *i, (*i)->getRpl()))
        result = false;
    }
    return result;
  }

  /// Get Effect Kind from Attr type
  inline EffectKind getEffectKind(const NoEffectAttr* attr) {
    return NoEffect;
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
                    // NoEffectAttr as well, but the compiler complains
                    // that such attributes don't have a getRpl method...
      if (Rpl* tmp = rplAttrMap[*i]) {        
        rpl = new Rpl(tmp); // FIXME: Do we need to copy here?
        ev.push_back(new Effect(ec, rpl, *i));
      }
    }
  }

  void buildEffectSummary(Decl* D, Effect::EffectVector& ev) {   
    buildPartialEffectSummary<ReadsEffectAttr>(D, ev);
    buildPartialEffectSummary<WritesEffectAttr>(D, ev);
    buildPartialEffectSummary<AtomicReadsEffectAttr>(D, ev);
    buildPartialEffectSummary<AtomicWritesEffectAttr>(D, ev);
    if (const NoEffectAttr* attr = D->getAttr<NoEffectAttr>()) {
      Effect* e = new Effect(NoEffect, 0, attr);
      ev.push_back(e);      
    }
  }
  
  /**
   *  Check that an effect summary is minimal and, if not, remove 
   *  superluous effects
   */
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

          StringRef bugStr = strbuf.str();
          helperEmitAttributeWarning(D, (*I)->getAttr(), bugStr, bugName);
          
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
  
public:

  typedef RecursiveASTVisitor<ASaPSemanticCheckerTraverser> BaseClass;

  /// Constructor
  explicit ASaPSemanticCheckerTraverser (
    ento::BugReporter& BR, ASTContext& ctx,
    AnalysisDeclContext* AC, raw_ostream& os,
    EffectSummaryMapTy& esm, RplAttrMapTy& rplAttrMap
    ) : BR(BR),
        Ctx(ctx),
        AC(AC),
        os(os),
        fatalError(false),
        effectSummaryMap(esm),
        rplAttrMap(rplAttrMap)
  {}

  /// Getters & Setters
  inline bool encounteredFatalError() { return fatalError; }
  
  /// Visitors
  bool VisitValueDecl(ValueDecl* E) {
    os << "DEBUG:: VisitValueDecl : ";
    E->print(os, Ctx.getPrintingPolicy());
    os << "\n";
    return true;
  }

  bool VisitFunctionDecl(FunctionDecl* D) {
    os << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"
       << "DEBUG:: printing ASaP attributes for method or function '";
    D->getDeclName().printName(os);
    os << "':\n";
    /// A. Detect Annotations
    /// A.1. Detect Region and Parameter Declarations
    helperPrintAttributes<RegionAttr>(D);

    /// A.2. Detect Region Parameter Declarations
    helperPrintAttributes<RegionParamAttr>(D);

    /// A.3. Detect Effects
    helperPrintAttributes<NoEffectAttr>(D); /// pure
    helperPrintAttributes<ReadsEffectAttr>(D); /// reads
    helperPrintAttributes<WritesEffectAttr>(D); /// writes
    helperPrintAttributes<AtomicReadsEffectAttr>(D); /// atomic reads
    helperPrintAttributes<AtomicWritesEffectAttr>(D); /// atomic writes

    /// B. Check Annotations
    /// B.1 Check Regions & Params
    checkRegionOrParamDecls<RegionAttr>(D);
    checkRegionOrParamDecls<RegionParamAttr>(D);
    /// B.2 Check Effect RPLs
    checkRpls<ReadsEffectAttr>(D);
    checkRpls<WritesEffectAttr>(D);
    checkRpls<AtomicReadsEffectAttr>(D);
    checkRpls<AtomicWritesEffectAttr>(D);

    /// C. Check effect summary
    /// C.1. Build Effect Summary
    Effect::EffectVector *ev = new Effect::EffectVector();
    Effect::EffectVector &effectSummary = *ev;
    buildEffectSummary(D, effectSummary);
    os << "Effect Summary from annotation:\n";
    Effect::printEffectSummary(effectSummary, os);

    /// C.2. Check Effect Summary is consistent and minimal
    checkEffectSummary(D, effectSummary);
    os << "Minimal Effect Summary:\n";
    Effect::printEffectSummary(effectSummary, os);
    effectSummaryMap[D] = ev;    
    return true;
  }

  bool VisitRecordDecl (RecordDecl* D) {
    os << "DEBUG:: printing ASaP attributes for class or struct '";
    D->getDeclName().printName(os);
    os << "':\n";
    /// A. Detect Region & Param Annotations
    helperPrintAttributes<RegionAttr>(D);
    helperPrintAttributes<RegionParamAttr>(D);

    /// B. Check Region & Param Names
    checkRegionOrParamDecls<RegionAttr>(D);
    checkRegionOrParamDecls<RegionParamAttr>(D);

    return true;
  }

  bool VisitFieldDecl(FieldDecl *D) {
    os << "DEBUG:: VisitFieldDecl\n";
    /// A. Detect Region In & Arg annotations
    helperPrintAttributes<RegionArgAttr>(D); /// in region

    /// B. Check RPLs
    checkRpls<RegionArgAttr>(D);
    
    /// C. Check validity of annotations
    checkTypeRegionArgs(D);
    return true;
  }

  bool VisitVarDecl(VarDecl *D) {
    os << "DEBUG:: VisitVarDecl\n";
    checkTypeRegionArgs(D);
    return true;
  }

  bool VisitCXXMethodDecl(clang::CXXMethodDecl *D) {
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
*/
  /*bool VisitAssignmentExpression() {
    os << "DEBUG:: VisitAssignmentExpression\n"
    return true;
  }*/

}; // end class ASaPSemanticCheckerTraverser

class  SafeParallelismChecker
  : public Checker<check::ASTDecl<TranslationUnitDecl> > {

public:
  void checkASTDecl(const TranslationUnitDecl* D, AnalysisManager& mgr,
                    BugReporter &BR) const {
    os << "DEBUG:: starting ASaP Semantic Checker\n";
    BuiltinDefaulrRegionParam = ::new(D->getASTContext()) 
        RegionParamAttr(D->getSourceRange(), D->getASTContext(), "P");
    /** initialize traverser */
    EffectSummaryMapTy esm;
    RplAttrMapTy ram;
    ASaPSemanticCheckerTraverser 
      asapSemaChecker(BR, D->getASTContext(), 
                    mgr.getAnalysisDeclContext(D), os, esm, ram);
    /** run checker */
    asapSemaChecker.TraverseDecl(const_cast<TranslationUnitDecl*>(D));
    os << "##############################################\n";
    os << "DEBUG:: done running ASaP Semantic Checker\n\n";
    
    if (!asapSemaChecker.encounteredFatalError()) {
      /// Check that Effect Summaries cover effects
      ASaPEffectsCheckerTraverser asapEffectChecker(BR, D->getASTContext(),
                    mgr, mgr.getAnalysisDeclContext(D), os, esm, ram);
      asapEffectChecker.TraverseDecl(const_cast<TranslationUnitDecl*>(D));
    }
    
    // FIXME: deleting BuiltinDefaulrRegionParam below creates dangling 
    // pointers (i.e. there's some memory leak somewhere).
    //::delete BuiltinDefaulrRegionParam;
  }
};
} // end unnamed namespace

void ento::registerSafeParallelismChecker(CheckerManager &mgr) {
  mgr.registerChecker<SafeParallelismChecker>();
}
