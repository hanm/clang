//=== CollectRegionNamesAndParameters.cpp - Safe Parallelism checker ===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This file defines the pass that collects the region names and
// parameters for the Safe Parallelism checker, which tries to prove
// the safety of parallelism given region and effect annotations.
//
//===----------------------------------------------------------------===//
#include "clang/AST/ASTContext.h"

#include "clang/AST/Type.h"
#include "clang/AST/Decl.h"

#include "ASaPUtil.h"
#include "ASaPSymbolTable.h"
#include "CollectRegionNamesAndParameters.h"
#include "Rpl.h"

namespace clang {
namespace asap {

// Constructor
CollectRegionNamesAndParametersTraverser::
CollectRegionNamesAndParametersTraverser(VisitorBundle &_VB)
  : VB(_VB),
  BR(VB.BR),
  Ctx(VB.Ctx),
  OS(VB.OS),
  SymT(VB.SymT),
  FatalError(false) {}

// Private Functions
inline StringRef CollectRegionNamesAndParametersTraverser::
getRegionOrParamName(const Attr *Attribute) {
  StringRef Result = "";
  switch(Attribute->getKind()) {
  case attr::Region:
    Result = dyn_cast<RegionAttr>(Attribute)->getName(); break;
  case attr::RegionParam:
    Result = dyn_cast<RegionParamAttr>(Attribute)->getName(); break;
  default:
    Result = "";
  }
  return Result;
}

template<typename AttrType>
void CollectRegionNamesAndParametersTraverser::
helperPrintAttributes(Decl *D) {
  for (specific_attr_iterator<AttrType>
       I = D->specific_attr_begin<AttrType>(),
       E = D->specific_attr_end<AttrType>();
       I != E; ++I) {
    (*I)->printPretty(OS, Ctx.getPrintingPolicy());
    OS << "\n";
  }
}

template<typename AttrType>
bool CollectRegionNamesAndParametersTraverser::
checkRegionOrParamDecls(Decl *D) {
  bool Result = true;
  specific_attr_iterator<AttrType>
      I = D->specific_attr_begin<AttrType>(),
      E = D->specific_attr_end<AttrType>();
  for ( ; I != E; ++I) {
    assert(isa<RegionAttr>(*I) || isa<RegionParamAttr>(*I));
    const llvm::StringRef ElmtNames = getRegionOrParamName(*I);

    llvm::SmallVector<StringRef, 8> RplElmtVec;
    ElmtNames.split(RplElmtVec, Rpl::RPL_LIST_SEPARATOR);
    for (size_t Idx = 0 ; Idx != RplElmtVec.size(); ++Idx) {
      llvm::StringRef Name = RplElmtVec[Idx].trim();
      if (Rpl::isValidRegionName(Name)) {
        /// Add it to the vector.
        OS << "DEBUG:: creating RPL Element called " << Name << "\n";
        if (isa<RegionAttr>(*I)) {
         const Decl *ScopeDecl = D;
          if (isa<EmptyDecl>(D)) {
            // An empty declaration is typically at global scope
            // E.g., [[asap::name("X")]];
            ScopeDecl = getDeclFromContext(D->getDeclContext());
            assert(ScopeDecl);
          }
          if (!SymT.addRegionName(ScopeDecl, Name)) {
            // Region name already declared at this scope.
            emitRedeclaredRegionName(D, Name);
            Result = false;
          }
        } else if (isa<RegionParamAttr>(*I)) {
          if (!SymT.addParameterName(D, Name)) {
            // Region parameter already declared at this scope.
            emitRedeclaredRegionParameter(D, Name);
            Result = false;
          }
        }
      } else {
        /// Emit bug report: ill formed region or parameter name.
        emitIllFormedRegionNameOrParameter(D, *I, Name);
        Result = false;
      }
    } // End for each Element of Attribute.
  } // End for each Attribute of type AttrType.
  return Result;
}

inline void CollectRegionNamesAndParametersTraverser::
emitRedeclaredRegionName(const Decl *D, const StringRef &Str) {
  StringRef BugName = "region name already declared at this scope";
  helperEmitDeclarationWarning(BR, D, Str, BugName);
  // Not a Fatal Error
}

inline void CollectRegionNamesAndParametersTraverser::
emitRedeclaredRegionParameter(const Decl *D, const StringRef &Str) {
  FatalError = true;
  StringRef BugName = "region parameter already declared at this scope";
  helperEmitDeclarationWarning(BR, D, Str, BugName);
}

inline void CollectRegionNamesAndParametersTraverser::
emitIllFormedRegionNameOrParameter(const Decl *D,
                                   const Attr *A, StringRef Name) {
  // Not a fatal error (e.g., if region name is not actually used)
  std::string AttrTypeStr = "";
  assert(A);
  if (isa<RegionAttr>(A))
    AttrTypeStr = "region";
  else if (isa<RegionParamAttr>(A))
    AttrTypeStr = "region parameter";
  std::string BugName = "invalid ";
  BugName.append(AttrTypeStr);
  BugName.append(" name");
  helperEmitAttributeWarning(BR, D, A, Name, BugName);
}

//////////////////////////////////////////////////////////////////////////
// Visitors

bool CollectRegionNamesAndParametersTraverser::
VisitFunctionDecl(FunctionDecl *D) {
  OS << "DEBUG:: VisitFunctionDecl (" << D << ") "
     << D->getDeclName() << "\n";
  D->dump(OS);
  OS << "':\n";


  /// A. Detect Annotations
  /// A.1. Detect Region and Parameter Declarations
  helperPrintAttributes<RegionAttr>(D);

  /// A.2. Detect Region Parameter Declarations
  helperPrintAttributes<RegionParamAttr>(D);

  /// B Check Region Name & Param Declarations
  checkRegionOrParamDecls<RegionAttr>(D);
  checkRegionOrParamDecls<RegionParamAttr>(D);

  return true;
}

bool CollectRegionNamesAndParametersTraverser::
VisitRecordDecl (RecordDecl *D) {

  OS << "DEBUG:: VisitRecordDecl (" << D << ") : ";
  D->print(OS, Ctx.getPrintingPolicy());
  OS << "\n";
  D->dump(OS);
  OS << "\n";

  OS << "DEBUG:: printing ASaP attributes for class or struct '";
  OS << D->getDeclName();
  OS << "':\n";

  /// A. Detect Region & Param Annotations
  helperPrintAttributes<RegionAttr>(D);
  helperPrintAttributes<RegionParamAttr>(D);

  /// B Check Region Name & Param Declarations
  checkRegionOrParamDecls<RegionAttr>(D);
  // An empty param vector means the class (was visited and) takes
  // zero region arguments.
  SymT.initParameterVector(D);
  checkRegionOrParamDecls<RegionParamAttr>(D);

  return true;
}

bool CollectRegionNamesAndParametersTraverser::
VisitEmptyDecl(EmptyDecl *D) {
  OS << "DEBUG:: VisitEmptyDecl\n'";
  /// A. Detect Region & Param Annotations
  helperPrintAttributes<RegionAttr>(D);

  /// B. Check Region & Param Names
  checkRegionOrParamDecls<RegionAttr>(D);

  return true;
}

bool CollectRegionNamesAndParametersTraverser::
VisitNamespaceDecl (NamespaceDecl *D) {
  OS << "DEBUG:: VisitNamespaceDecl (" << D << ") "
     << D->getDeclName();
  OS << "':\n";
  /// A. Detect Region & Param Annotations
  helperPrintAttributes<RegionAttr>(D);

  /// B. Check Region & Param Names
  checkRegionOrParamDecls<RegionAttr>(D);

  return true;
}

} // end namespace asap
} // end namespace clang
