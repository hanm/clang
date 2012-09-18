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
  bool isSpecialRplElement(const StringRef& s) {
    if (!s.compare("*")) 
      return true;
    else 
      return false;
  }
  
  bool isValidRegionName(const StringRef& s) {
    // true if it is one of the Special Rpl Elements
    if (isSpecialRplElement(s)) return true;
    
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

class ASPSemanticCheckerTraverser : 
  public RecursiveASTVisitor<ASPSemanticCheckerTraverser> {

private:
  // Private Members
  ento::BugReporter& BR;
  ASTContext& Ctx;
  AnalysisDeclContext* AC;
  raw_ostream& os;


  // Private Methods
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
  
  bool checkRegionDeclarations(Decl* D) {
    os << "DEBUG:: well what have we here?!\n";
    int i = 0;
    bool result = true;
    const RegionAttr* myAttr = D->getAttr<RegionAttr>(i++);
    while (myAttr) {
      os << "DEBUG:: well well...!\n";
      const StringRef str = myAttr->getRegion_name();
      if (!isValidRegionName(str)) {
        // Emit bug report!
        std::string descr = "Invalid Region or Parameter name: '";
        descr.append(str);
        descr.append("'");
              
        PathDiagnosticLocation VDLoc =
           PathDiagnosticLocation::createBegin(D, BR.getSourceManager());

        BR.EmitBasicReport(D, descr.c_str(), "Safe Parallelism", descr.c_str(), VDLoc);
        result = false; 
      }
      myAttr = D->getAttr<RegionAttr>(i++);
    }
    return result; 
  }

  #define GET_NAME_FUNCTION(ATTR_TYPE)  GET_NAME_FUNCTION_##ATTR_TYPE()
  
  #define GET_NAME_FUNCTION_RegionAttr()       getRegion_name()
  #define GET_NAME_FUNCTION_RegionParamAttr()  getParam_name()

  #define SCAN_ATTRIBUTES(ATTR_TYPE) \
  { \
    int i = 0; \
    const ATTR_TYPE* myAttr = D->getAttr<ATTR_TYPE>(i++); \
    while (myAttr) { \
      StringRef rName = myAttr->GET_NAME_FUNCTION(ATTR_TYPE); \
      if (rName==name) return true; \
      myAttr = D->getAttr<ATTR_TYPE>(i++); \
    } \
  }
  
  /* The template approach below doesn't work because templates are 
  initialized after the preprocessor is run, so GET_NAME_FUNCTION_ATTR_TYPE \
  is not macro-expanded to the correct function call.
  
  template<typename AttrType>
  bool helperFindRegionName(Decl* D, const StringRef& name)
  { 
    int i = 0; 
    const AttrType* myAttr = D->getAttr<AttrType>(i++); 
    while (myAttr) { 
      StringRef rName = myAttr->GET_NAME_FUNCTION(AttrType);
      if (rName==name) return true; 
      myAttr = D->getAttr<AttrType>(i++); 
    } 
  }*/
  
  /**
   *  Looks for 'name' in the declaration 'D' and its parent scopes 
   */    
  bool findRegionName(Decl* D, StringRef name) {
    if (!D) return false;
    /// 1. try to find among regions or region parameters of function
    /// a. regions
    SCAN_ATTRIBUTES(RegionAttr);
    //if (helperFindRegionName<RegionAttr>(D, name)) return true;

    /// b. parameters
    SCAN_ATTRIBUTES(RegionParamAttr);
    //if (helperFindRegionName<RegionParamAttr>(D, name)) return true;

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
        std::string description_std = "RPL element '";
        description_std.append(head);
        description_std.append("' was not declared");
              
        PathDiagnosticLocation VDLoc =
           PathDiagnosticLocation::createBegin(D, BR.getSourceManager());

        const char* descr = description_std.c_str();
        BR.EmitBasicReport(D,descr, "Safe Parallelism", descr, VDLoc);
        result = false;
      }
      rpl = pair.second;
    }
    return result;
  }
  /// AttrType must implement getRpl
  template<typename AttrType>
  bool checkRpls(Decl* D) {
    const AttrType* myAttr;
    int i = 0;
    bool result = true;
    do { /// for all attributes of type AttrType
      myAttr = D->getAttr<AttrType>(i++);
      if (myAttr) {
        checkRpl(D, myAttr->getRpl());
      }
    } while (myAttr);

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
    /// 1. Detect Region Declarations
    helperPrintAttributes<RegionAttr>(D);
    checkRegionDeclarations(D);

    /// 2. Detect Region Parameter Declarations
    helperPrintAttributes<RegionParamAttr>(D);

    /// 3. Effects
    helperPrintAttributes<PureEffectAttr>(D); /// pure
    helperPrintAttributes<ReadsEffectAttr>(D); /// reads
    helperPrintAttributes<WritesEffectAttr>(D); /// writes
    helperPrintAttributes<AtomicReadsEffectAttr>(D); /// atomic reads
    helperPrintAttributes<AtomicWritesEffectAttr>(D); /// atomic writes

    /// B. Check Effects
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
    /// 1. Detect Region Declarations
    helperPrintAttributes<RegionAttr>(D);
    checkRegionDeclarations(D);

    /// 2. Detect Region Parameter Declarations
    helperPrintAttributes<RegionParamAttr>(D);

    return true;
  }

  bool VisitFieldDecl(FieldDecl *D) {
    os << "DEBUG:: VisitFieldDecl\n";
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
