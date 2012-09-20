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
  /// AttrType must implement getRpl
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

public:

  typedef RecursiveASTVisitor<ASPSemanticCheckerTraverser> BaseClass;

  // Constructor
  explicit ASPSemanticCheckerTraverser (
    ento::BugReporter& BR, ASTContext& ctx,
    AnalysisDeclContext* AC
    ) : BR(BR),
        Ctx(ctx),
        AC(AC),
        os(llvm::errs())
  {}

  bool VisitFunctionDecl(FunctionDecl* D) {
    os << "DEBUG:: printing ASP attributes for method or function '";
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
    return true;
  }
  bool VisitCXXConstructorDecl(CXXConstructorDecl *D) {
    // ATTENTION This is called after VisitFunctionDecl
    os << "DEBUG:: VisitCXXConstructorDecl\n";
    return true;
  }
  bool VisitCXXDestructorDecl(CXXDestructorDecl *D) {
    // ATTENTION This is called after VisitFunctionDecl
    os << "DEBUG:: VisitCXXDestructorDecl\n";
    return true;
  }
  bool VisitCXXConversionDecl(CXXConversionDecl *D) {
    // ATTENTION This is called after VisitFunctionDecl
    os << "DEBUG:: VisitCXXConversionDecl\n";
    return true;
  }

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
