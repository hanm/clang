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

//#include "stdio.h"
#include <iostream>
#include <iterator>

using namespace clang;
using namespace ento;

namespace {
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
    const AttrType* attr;
    int i = 1;
    do {
      attr = D->getAttr<AttrType>(i++);
      if (attr) {
        attr->printPretty(os, Ctx.getPrintingPolicy());
        os << "\n";
      }
    } while (attr);
  }

  bool helperFindRegionName(Decl* D, StringRef name) {
    if (!D) return false;
    /*os << "DEBUG:: helperFindRegionName called on Decl: ";
    D->print(os, Ctx.getPrintingPolicy());
    os << "\n";*/
    /// 1. try to find among regions or region parameters of function
    /// a. regions
    //const AttrType* myAttr;
    {
      const RegionAttr* myAttr;
      int i = 1;
      do {
        myAttr = D->getAttr<RegionAttr>(i++);
        if (myAttr) {
          StringRef rName = myAttr->getRegion_name();
          if (rName==name) return true;
        }
      } while (myAttr);
    }

    /// b. parameters
    {
      const RegionParamAttr* myAttr;
      int i = 1;
      do {
        myAttr = D->getAttr<RegionParamAttr>(i++);
        if (myAttr) {
          StringRef pName = myAttr->getParam_name();
          if (pName==name) return true;
        }
      } while (myAttr);
    }

    /// if not found, search parent DeclContexts
    DeclContext *dc = D->getDeclContext();
    while (dc) {
      if (dc->isFunctionOrMethod()) {
        FunctionDecl* fd = dyn_cast<FunctionDecl>(dc);
        assert(fd);
        return helperFindRegionName(fd, name);
      } else if (dc->isRecord()) {
        RecordDecl* rd = dyn_cast<RecordDecl>(dc);
        assert(rd);
        return helperFindRegionName(rd, name);
      } else {
        dc = dc->getParent();
      }
    }
    return false;
  }

  /// AttrType must implement getRpl
  template<typename AttrType>
  bool helperCheckRpl(Decl* D) {
    const AttrType* myAttr;
    int i = 1;
    bool result = true;
    do { /// for all attributes of type AttrType
      myAttr = D->getAttr<AttrType>(i++);
      if (myAttr) {
        StringRef rpl = myAttr->getRpl();
        while(rpl.size() > 0) { /// for all RPL elements of the RPL
            std::pair<StringRef,StringRef> pair = rpl.split(':');
            /// pair.first: is it declared?
            if (helperFindRegionName(D, pair.first))
              os << "DEBUG:: found declaration of region '" << pair.first << "'\n";
            else {
              os << "DEBUG:: didn't find declaration of region '" << pair.first << "'\n";
              result = false;
            }
            ////~~~
            rpl = pair.second;
        }
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

    /// 2. Detect Region Parameter Declarations
    helperPrintAttributes<RegionParamAttr>(D);

    /// 3. Effects
    helperPrintAttributes<PureEffectAttr>(D); /// pure
    helperPrintAttributes<ReadsEffectAttr>(D); /// reads
    helperPrintAttributes<WritesEffectAttr>(D); /// writes
    helperPrintAttributes<AtomicReadsEffectAttr>(D); /// atomic reads
    helperPrintAttributes<AtomicWritesEffectAttr>(D); /// atomic writes

    /// B. Check Effects
    helperCheckRpl<ReadsEffectAttr>(D);
    helperCheckRpl<WritesEffectAttr>(D);
    helperCheckRpl<AtomicReadsEffectAttr>(D);
    helperCheckRpl<AtomicWritesEffectAttr>(D);

    return true;
  }

  bool VisitRecordDecl (RecordDecl* D) {
    os << "DEBUG:: printing ASP attributes for class or struct '";
    D->getDeclName().printName(os);
    os << "':\n";
    /// 1. Detect Region Declarations
    helperPrintAttributes<RegionAttr>(D);

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
