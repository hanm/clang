//=== SemanticChecker.cpp - Safe Parallelism checker -----*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This files defines the Semantic Checker pass of the Safe Parallelism
// checker, which tries to prove the safety of parallelism given region
// and effect annotations.
//
//===----------------------------------------------------------------===//

#include "SemanticChecker.h"
#include "ASaPType.h"
#include "clang/AST/Decl.h"
// FIXME: Include these static analyzer core headers are really unfortunate given
// we only traverse AST without any path sensitive analysis. These header should be
// removed.
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/AnalysisManager.h"
#include "clang/StaticAnalyzer/Core/BugReporter/PathDiagnostic.h"

using namespace clang;
using namespace clang::asap;
using namespace clang::ento;
using namespace llvm;

void ASaPSemanticCheckerTraverser::
addASaPTypeToMap(ValueDecl *D, RplVector *RV, Rpl *InRpl) {
  assert(!SymT.hasType(D));
  ASaPType *T = new ASaPType(D->getType(), RV, InRpl);
  OS << "DEBUG:: D->getType() = ";
  D->getType().print(OS, Ctx.getPrintingPolicy());
  OS << ", isFunction = " << D->getType()->isFunctionType() << "\n";
  OS << "Debug:: RV.size=" << (RV ? RV->size() : 0)

    << ", T.RV.size=" << T->getArgVSize() << "\n";
  OS << "Debug :: adding type: " << T->toString(Ctx) << " to Decl: ";
  D->print(OS, Ctx.getPrintingPolicy());
  OS << "\n";
  bool Result = SymT.setType(D, T);
  assert(Result);
}

void ASaPSemanticCheckerTraverser::
helperEmitDeclarationWarning(const Decl *D, const StringRef &Str,
                             std::string BugName, bool AddQuotes) {
  std::string Description = "";
  if (AddQuotes)
    Description.append("'");
  Description.append(Str);
  if (AddQuotes)
    Description.append("': ");
  else
    Description.append(": ");
  Description.append(BugName);
  StringRef BugCategory = "Safe Parallelism";
  StringRef BugStr = Description;
  PathDiagnosticLocation VDLoc(D->getLocation(), BR.getSourceManager());
  BR.EmitBasicReport(D, BugName, BugCategory,
                     BugStr, VDLoc, D->getSourceRange());
}

void ASaPSemanticCheckerTraverser::
helperEmitAttributeWarning(const Decl *D, const Attr *Attr,
                           const StringRef &Str, std::string BugName,
                           bool AddQuotes) {
  std::string Description = "";
  if (AddQuotes)
    Description.append("'");
  Description.append(Str);
  if (AddQuotes)
    Description.append("': ");
  else
    Description.append(": ");
  Description.append(BugName);
  StringRef BugCategory = "Safe Parallelism";
  StringRef BugStr = Description;
  PathDiagnosticLocation VDLoc(Attr->getLocation(), BR.getSourceManager());
  BR.EmitBasicReport(D, BugName, BugCategory,
                     BugStr, VDLoc, Attr->getRange());
}

void ASaPSemanticCheckerTraverser::
emitRedeclaredRegionName(const Decl *D, const StringRef &Str) {
  StringRef BugName = "region name already declared at this scope";
  helperEmitDeclarationWarning(D, Str, BugName);
  // Not a Fatal Error
}

inline void ASaPSemanticCheckerTraverser::
emitRedeclaredRegionParameter(const Decl *D, const StringRef &Str) {
  FatalError = true;
  StringRef BugName = "region parameter already declared at this scope";
  helperEmitDeclarationWarning(D, Str, BugName);
}

void ASaPSemanticCheckerTraverser::
emitMisplacedRegionParameter(const Decl *D,
                             const Attr* A,
                             const StringRef &S) {
  StringRef BugName = "Misplaced Region Parameter: Region parameters "
                        "may only appear at the head of an RPL.";
  helperEmitAttributeWarning(D, A, S, BugName);
}

void ASaPSemanticCheckerTraverser::
emitUndeclaredRplElement(const Decl *D,
                         const Attr *Attr,
                         const StringRef &Str) {
  StringRef BugName = "RPL element was not declared";
  helperEmitAttributeWarning(D, Attr, Str, BugName);
}

void ASaPSemanticCheckerTraverser::
emitNameSpecifierNotFound(Decl *D, Attr *A, StringRef Name) {
  StringRef BugName = "Name specifier was not found";
  helperEmitAttributeWarning(D, A, Name, BugName);
}

void ASaPSemanticCheckerTraverser::emitMissingRegionArgs(Decl *D) {
  FatalError = true;
  std::string bugName = "missing region argument(s)";

  std::string sbuf;
  llvm::raw_string_ostream strbuf(sbuf);
  D->print(strbuf, Ctx.getPrintingPolicy());

  helperEmitDeclarationWarning(D, strbuf.str(), bugName);
}

void ASaPSemanticCheckerTraverser::
emitUnknownNumberOfRegionParamsForType(Decl *D) {
  FatalError = true;
  std::string bugName = "unknown number of region parameters for type";

  std::string sbuf;
  llvm::raw_string_ostream strbuf(sbuf);
  D->print(strbuf, Ctx.getPrintingPolicy());

  helperEmitDeclarationWarning(D, strbuf.str(), bugName);
}

void ASaPSemanticCheckerTraverser::
emitSuperfluousRegionArg(Decl *D, StringRef Str) {
  FatalError = true;
  std::string bugName = "superfluous region argument(s)";
  helperEmitDeclarationWarning(D, Str, bugName);
}

void ASaPSemanticCheckerTraverser::
emitIllFormedRegionNameOrParameter(Decl *D, Attr *A, StringRef Name) {
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
  helperEmitAttributeWarning(D, A, Name, BugName);
}

void ASaPSemanticCheckerTraverser::
emitCanonicalDeclHasSmallerEffectSummary(const Decl *D, const StringRef &Str) {
  StringRef BugName = "effect summary of canonical declaration does not cover"\
    " the summary of this declaration";
  helperEmitDeclarationWarning(D, Str, BugName);
}

void ASaPSemanticCheckerTraverser::emitEffectCovered(Decl *D, const Effect *E1,
                                                     const Effect *E2) {
  // warning: e1 is covered by e2
  StringRef BugName = "effect summary is not minimal";
  std::string sbuf;
  llvm::raw_string_ostream StrBuf(sbuf);
  StrBuf << "'"; E1->print(StrBuf);
  StrBuf << "' covered by '";
  E2->print(StrBuf); StrBuf << "'";
  //StrBuf << BugName;

  StringRef BugStr = StrBuf.str();
  helperEmitAttributeWarning(D, E1->getAttr(), BugStr, BugName, false);
}

void ASaPSemanticCheckerTraverser::
emitNoEffectInNonEmptyEffectSummary(Decl *D, const Attr *A) {
  StringRef BugName = "no_effect is illegal in non-empty effect summary";
  StringRef BugStr = "";

  helperEmitAttributeWarning(D, A, BugStr, BugName, false);
}

StringRef ASaPSemanticCheckerTraverser::
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

const RplElement *ASaPSemanticCheckerTraverser::
findRegionOrParamName(const Decl *D, StringRef Name) {
  if (!D)
    return 0;
  /// 1. try to find among regions or region parameters
  const RplElement *Result = SymT.lookupParameterName(D, Name);
  if (!Result)
    Result = SymT.lookupRegionName(D, Name);
  return Result;
}

const Decl *ASaPSemanticCheckerTraverser::
getDeclFromContext(const DeclContext *DC) {
  assert(DC);
  const Decl *D = 0;
  if (DC->isFunctionOrMethod())
    D = dyn_cast<FunctionDecl>(DC);
  else if (DC->isRecord())
    D = dyn_cast<RecordDecl>(DC);
  else if (DC->isNamespace())
    D = dyn_cast<NamespaceDecl>(DC);
  else if (DC->isTranslationUnit())
    D = dyn_cast<TranslationUnitDecl>(DC);
  return D;
}

const RplElement *ASaPSemanticCheckerTraverser::
recursiveFindRegionOrParamName(const Decl *D, StringRef Name) {
  /// 1. try to find among regions or region parameters of function
  const RplElement *Result = findRegionOrParamName(D, Name);
  if (Result)
    return Result;

  /// if not found, search parent DeclContexts
  const DeclContext *DC = D->getDeclContext();
  while (DC) {
    const Decl *EnclosingDecl = getDeclFromContext(DC);
    if (EnclosingDecl)
      return recursiveFindRegionOrParamName(EnclosingDecl, Name);
    else
      DC = DC->getParent();
  }

  return 0;
}

void ASaPSemanticCheckerTraverser::
checkTypeRegionArgs(ValueDecl *D, const Rpl *DefaultInRpl) {
  assert(D);
  RegionArgAttr *A = D->getAttr<RegionArgAttr>();
  RplVector *RplVec = (A) ? RplVecAttrMap[A] : 0;
  if (A && !RplVec && FatalError)
    return; // don't check an error already occured

  QualType QT = D->getType();
  // How many In/Arg annotations does the type require?
  OS << "DEBUG:: calling getRegionParamCount on type: ";
  QT.print(OS, Ctx.getPrintingPolicy());
  OS << "\n";
  OS << "DEBUG:: Decl:";
  D->print(OS, Ctx.getPrintingPolicy());
  OS << "\n";

  SymbolTable::ResultTriplet ResTriplet = SymT.getRegionParamCount(QT);
  ResultKind ResKin = ResTriplet.ResKin;
  long ParamCount = ResTriplet.NumArgs;
  OS << "DEBUG:: called 'getRegionParamCount(QT)' : "
     << "(" << stringOf(ResKin)
     << ", " << ParamCount << ") DONE!\n";
  long ArgCount = (RplVec) ? RplVec->size() : 0;
  OS << "ArgCount = " << ArgCount << "\n";

  switch(ResKin) {
  case RK_NOT_VISITED:
    assert(ResTriplet.DeclNotVis);
    OS << "DEBUG:: DeclNotVisited : ";
    ResTriplet.DeclNotVis->print(OS, Ctx.getPrintingPolicy());
    OS << "\n";

    VisitRecordDecl(ResTriplet.DeclNotVis);
    OS << "DEBUG:: done with the recursive visiting\n";
    checkTypeRegionArgs(D, DefaultInRpl);
    break;
  case RK_ERROR:
    emitUnknownNumberOfRegionParamsForType(D);
    break;
  case RK_VAR:
    // Type is TemplateTypeParam -- Any number of region args
    // could be ok. At least ParamCount are needed though.
    if (ParamCount > ArgCount &&
      ParamCount > ArgCount + (DefaultInRpl?1:0)) {
        emitMissingRegionArgs(D);
    }
    break;
  case RK_OK:
    if (ParamCount > ArgCount &&
      ParamCount > ArgCount + (DefaultInRpl?1:0)) {
        emitMissingRegionArgs(D);
    } else if (ParamCount < ArgCount &&
      ParamCount < ArgCount + (DefaultInRpl?1:0)) {
        std::string SBuf;
        llvm::raw_string_ostream BufStream(SBuf);
        int I = ParamCount;
        for (; I < ArgCount-1; ++I) {
          RplVec->getRplAt(I)->print(BufStream);
          BufStream << ", ";
        }
        if (I < ArgCount)
          RplVec->getRplAt(I)->print(BufStream);
        emitSuperfluousRegionArg(D, BufStream.str());
    } else /*if (ParamCount > 0) */{
      assert(ParamCount>=0);
      if (ParamCount > ArgCount) {
        assert(DefaultInRpl);
        if (RplVec) {
          RplVec->push_front(DefaultInRpl);
        } else {
          RplVec = new RplVector(*DefaultInRpl);
        }
      }
      assert(ParamCount == 0 ||
        (size_t)ParamCount == RplVec->size());

      addASaPTypeToMap(D, RplVec, 0);
    }
    break;
    default:
      //TODO emitInternalError();
      break;
  } //  end switch
  OS << "DEBUG:: DONE checkTypeRegionArgs\n";
}

bool ASaPSemanticCheckerTraverser::checkRpls(Decl *D, Attr *A,
                                             StringRef RplsStr) {
  /// First check that we have not already parsed this attribute's RPL
  RplVector *RV = RplVecAttrMap[A];
  if (RV)
    return new RplVector(*RV);
  // else:
  bool Failed = false;

  RV = new RplVector();
  llvm::SmallVector<StringRef, 8> RplVec;
  RplsStr.split(RplVec, ",");

  for (size_t I = 0 ; !Failed && I != RplVec.size(); ++I) {
    Rpl *R = checkRpl(D, A, RplVec[I].trim());
    if (R) {
      RV->push_back(R);
    } else {
      Failed = true;
    }
  }
  if (Failed) {
    delete RV;
    RV = 0;
    return false;
  } else {
    RplVecAttrMap[A] = RV;
    return true;
  }
}

Rpl *ASaPSemanticCheckerTraverser::checkRpl(Decl *D, Attr *A,
                                            StringRef RplStr) {
  bool Result = true;
  int Count = 0;
  Rpl *R = new Rpl();
  while(RplStr.size() > 0) { /// for all RPL elements of the RPL
    const RplElement *RplEl = 0;
    std::pair<StringRef,StringRef> Pair = Rpl::splitRpl(RplStr);
    StringRef Head = Pair.first;
    llvm::SmallVector<StringRef, 8> Vec;
    Head.split(Vec, Rpl::RPL_NAME_SPEC);
    OS << "DEBUG:: Vec.size = " << Vec.size()
       << ", Vec.back() = " << Vec.back() <<"\n";

    if (Vec.size() > 1) {
      // Find the specified declaration
      DeclContext *DC = D->getDeclContext();
      DeclContextLookupResult Res;
      IdentifierInfo &II = Ctx.Idents.get(Vec[0]);
      DeclarationName DN(&II);
      OS << "DEBUG:: IdentifierInfo.getName = " << II.getName() << "\n";
      OS << "DEBUG:: DeclContext: ";
      //DC->printPretty(OS, Ctx.getPrintingPolicy());
      OS << "\n";
      while (DC && Res.size() == 0) {
        Res = DC->lookup(DN);
        OS << "DEBUG:: Lookup Result Size = " << Res.size() << "\n";
        DC = DC->getParent();
      }
      if (Res.size() !=1) {
        emitNameSpecifierNotFound(D, A, Vec[0]);
        return 0;
      }
      DC = Decl::castToDeclContext(Res[0]);
      assert(DC);

      for(size_t I = 1; I < Vec.size() - 1; ++I) {
        IdentifierInfo &II = Ctx.Idents.get(Vec[I]);
        DeclarationName DN(&II);
        OS << "DEBUG:: IdentifierInfo.getName = " << II.getName() << "\n";
        OS << "DEBUG:: DeclContext: ";
        //DC->printPretty(OS, Ctx.getPrintingPolicy());
        OS << "\n";
        Res = DC->lookup(DN);
        OS << "DEBUG:: Lookup Result Size = " << Res.size() << "\n";
        if (Res.size() !=1) {
          emitNameSpecifierNotFound(D, A, Vec[I]);
          return 0;
        }
        DC = Decl::castToDeclContext(Res[0]);
        assert(DC);
      }
      RplEl = findRegionOrParamName(Res[0], Vec.back());
    } else {
      assert(Vec.size() == 1);
      Head = Vec.back();
      /// head: is it a special RPL element? if not, is it declared?
      RplEl = SymbolTable::getSpecialRplElement(Head);
      if (!RplEl)
        RplEl = recursiveFindRegionOrParamName(D, Head);
    }
    if (!RplEl) {
      // Emit bug report!
      emitUndeclaredRplElement(D, A, Head);
      Result = false;
    } else { // RplEl != NULL
      if (Count>0 && (isa<ParamRplElement>(RplEl)
        || isa<CaptureRplElement>(RplEl))) {
          /// Error: region parameter is only allowed at the head of an Rpl
          emitMisplacedRegionParameter(D, A, Head);
      } else
        R->appendElement(RplEl);
    }
    /// Proceed to next iteration
    RplStr = Pair.second;
    ++Count;
  } // end while RplStr
  if (Result == false) {
    delete(R);
    R = 0;
  }
  return R;
}

void ASaPSemanticCheckerTraverser::buildEffectSummary(FunctionDecl *D,
                                                      EffectSummary &ES) {
  buildPartialEffectSummary<ReadsEffectAttr>(D, ES);
  buildPartialEffectSummary<WritesEffectAttr>(D, ES);
  buildPartialEffectSummary<AtomicReadsEffectAttr>(D, ES);
  buildPartialEffectSummary<AtomicWritesEffectAttr>(D, ES);
  if (const NoEffectAttr* Attr = D->getAttr<NoEffectAttr>()) {
    if (ES.size() > 0) {
      // "no effect" not compatible with other effects
      emitNoEffectInNonEmptyEffectSummary(D, Attr);
    } else {
      Effect E(Effect::EK_NoEffect, 0, Attr);
      bool Success = ES.insert(&E);
      assert(Success);
    }
  }
}

ASaPSemanticCheckerTraverser::ASaPSemanticCheckerTraverser(
  ento::BugReporter &BR, ASTContext &Ctx,
  AnalysisDeclContext *AC, raw_ostream &OS,
  SymbolTable &SymT)
  : BR(BR),
  Ctx(Ctx),
  AC(AC),
  OS(OS),
  SymT(SymT),
  FatalError(false),
  NextFunctionIsATemplatePattern(false)
{}

ASaPSemanticCheckerTraverser::~ASaPSemanticCheckerTraverser() {
  for(RplVecAttrMapT::const_iterator
    I = RplVecAttrMap.begin(),
    E = RplVecAttrMap.end();
  I != E; ++I) {
    delete (*I).second;
  }
}

bool ASaPSemanticCheckerTraverser::VisitValueDecl(ValueDecl *D) {
  OS << "DEBUG:: VisitValueDecl : ";
  D->print(OS, Ctx.getPrintingPolicy());
  OS << "\n";
  OS << "DEBUG:: it is " << (D->isTemplateDecl() ? "" : "NOT ")
    << "a template\n";
  OS << "DEBUG:: it is " << (D->isTemplateParameter() ? "" : "NOT ")
    << "a template PARAMETER\n";
  return true;
}

bool ASaPSemanticCheckerTraverser::VisitParmVarDecl(ParmVarDecl *D) {
  OS << "DEBUG:: VisitParmVarDecl : ";
  D->print(OS, Ctx.getPrintingPolicy());
  OS << "\n";
  OS << "DEBUG:: it is " << (D->isTemplateDecl() ? "" : "NOT ")
    << "a template\n";
  OS << "DEBUG:: it is " << (D->isTemplateParameter() ? "" : "NOT ")
    << "a template PARAMETER\n";
  return true;
}

bool ASaPSemanticCheckerTraverser::VisitFunctionDecl(FunctionDecl *D) {
  if (NextFunctionIsATemplatePattern) {
    NextFunctionIsATemplatePattern = false;
    //return true; // skip this function
  }

  OS << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"
    << "DEBUG:: printing ASaP attributes for method or function '";
  //D->getDeclName().printName(OS);
  D->print(OS, Ctx.getPrintingPolicy());
  OS << "':\n";

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
  /// B.2 Check ReturnType
  bool Success = checkRpls<RegionArgAttr>(D); // ReturnType
  if (Success) {
    Rpl Local(*SymbolTable::LOCAL_RplElmt);
    checkTypeRegionArgs(D, &Local); // check return type
  }

  /// B.3 Check Effect RPLs
  Success = true;
  Success &= checkRpls<ReadsEffectAttr>(D);
  Success &= checkRpls<WritesEffectAttr>(D);
  Success &= checkRpls<AtomicReadsEffectAttr>(D);
  Success &= checkRpls<AtomicWritesEffectAttr>(D);

  if (Success) {
    /// C. Check effect summary
    /// C.1. Build Effect Summary
    EffectSummary *ES = new EffectSummary();
    buildEffectSummary(D, *ES);
    OS << "Effect Summary from source file:\n";
    ES->print(OS);

    /// C.2. Check Effects covered by canonical Declaration
    const FunctionDecl *CanFD = D->getCanonicalDecl();
    if (CanFD != D) { // Case 1: We are not visiting the canonical Decl
      const EffectSummary *DeclEffSummary = SymT.getEffectSummary(CanFD);
      assert(DeclEffSummary); // Assume the canonical Decl has been visited.
      if (!DeclEffSummary->covers(ES)) {
        emitCanonicalDeclHasSmallerEffectSummary(D, D->getName());
        // In order not to abort after this pass because of this error:
        // make minimal, clean up EffectCoverageVector, and add to SymbolTable
        // Note: don't complain if effect is not minimal, as the effects of the
        // canonical decl are copied on this decl which often makes it
        // non-minimal.
        EffectSummary::EffectCoverageVector ECV;
        ES->makeMinimal(ECV);
        while(ECV.size() > 0) {
          std::pair<const Effect*, const Effect*> *PairPtr = ECV.pop_back_val();
          const Effect *E1 = PairPtr->first;
          delete E1;
          delete PairPtr;
        }
        bool Success = SymT.setEffectSummary(D, ES);
        assert(Success);
      } else { // The effect summary of the canonical decl covers this.

        // Set the Effect summary of this declaration to be the same
        // as that of the canonical declaration. Both SymTable entries
        // will point to the same EffectSummary object...
        // oops. How do we free it only once?
        bool Success = SymT.setEffectSummary(D, CanFD);
        assert(Success);
      }
    } else { // Case 2: Visiting the canonical Decl.
      // This declaration does not get effects copied from other decls...
      // (hopefully).

      /// C.2. Check Effect Summary is minimal
      EffectSummary::EffectCoverageVector ECV;
      ES->makeMinimal(ECV);
      while(ECV.size() > 0) {
        std::pair<const Effect*, const Effect*> *PairPtr = ECV.pop_back_val();
        const Effect *E1 = PairPtr->first, *E2 = PairPtr->second;
        emitEffectCovered(D, E1, E2);
        OS << "DEBUG:: effect " << E1->toString()
          << " covered by " << E2->toString() << "\n";
        delete E1;
        delete PairPtr;
      }
      OS << "Minimal Effect Summary:\n";
      ES->print(OS);
      bool Success = SymT.setEffectSummary(D, ES);
      assert(Success);
    }
  }
  return true;
}

bool ASaPSemanticCheckerTraverser::VisitRecordDecl (RecordDecl *D) {
  OS << "DEBUG:: printing ASaP attributes for class or struct '";
  D->getDeclName().printName(OS);
  OS << "':\n";
  if (SymT.hasDecl(D)) // in case we have already visited this don't re-visit.
    return true;
  /// A. Detect Region & Param Annotations
  helperPrintAttributes<RegionAttr>(D);
  helperPrintAttributes<RegionParamAttr>(D);

  /// B. Check Region & Param Names
  checkRegionOrParamDecls<RegionAttr>(D);
  SymT.initParameterVector(D); // An empty param vector means the class was
                               // visited and takes no region arguments.
  checkRegionOrParamDecls<RegionParamAttr>(D);
  return true;
}

bool ASaPSemanticCheckerTraverser::VisitEmptyDecl(EmptyDecl *D) {
  OS << "DEBUG:: printing ASaP attributes for empty declaration.\n'";
  /// A. Detect Region & Param Annotations
  helperPrintAttributes<RegionAttr>(D);

  /// B. Check Region & Param Names
  checkRegionOrParamDecls<RegionAttr>(D);

  return true;
}

bool ASaPSemanticCheckerTraverser::VisitNamespaceDecl (NamespaceDecl *D) {
  OS << "DEBUG:: printing ASaP attributes for namespace '";
  D->getDeclName().printName(OS);
  OS << "':\n";
  /// A. Detect Region & Param Annotations
  helperPrintAttributes<RegionAttr>(D);

  /// B. Check Region & Param Names
  checkRegionOrParamDecls<RegionAttr>(D);

  return true;
}

bool ASaPSemanticCheckerTraverser::VisitFieldDecl(FieldDecl *D) {
  OS << "DEBUG:: VisitFieldDecl : ";
  D->print(OS, Ctx.getPrintingPolicy());
  OS << "\n";

  /// A. Detect Region In & Arg annotations
  helperPrintAttributes<RegionArgAttr>(D); /// in region

  /// B. Check RPLs
  bool Success = checkRpls<RegionArgAttr>(D);

  /// C. Check validity of annotations
  if (Success)
    checkTypeRegionArgs(D, 0);

  return true;
}

bool ASaPSemanticCheckerTraverser::VisitVarDecl(VarDecl *D) {
  OS << "DEBUG:: VisitVarDecl: ";
  D->print(OS, Ctx.getPrintingPolicy());
  OS << "\n";
  OS << "DEBUG:: it is " << (D->isTemplateDecl() ? "" : "NOT ")
    << "a template\n";
  OS << "DEBUG:: it is " << (D->isTemplateParameter() ? "" : "NOT ")
    << "a template PARAMETER\n";

  /// A. Detect Region In & Arg annotations
  helperPrintAttributes<RegionArgAttr>(D); /// in region

  /// B. Check RPLs
  bool Success = checkRpls<RegionArgAttr>(D);

  /// C. Check validity of annotations
  if (Success) {
    // Get context to select default annotation
    if (D->isStaticLocal() || D->isStaticDataMember()
        || D->getDeclContext()->isFileContext()) {
      Rpl Global(*SymbolTable::GLOBAL_RplElmt);
      checkTypeRegionArgs(D, &Global);
    } else {
      Rpl Local(*SymbolTable::LOCAL_RplElmt);
      checkTypeRegionArgs(D, &Local);
    }
  }

  return true;
}

bool ASaPSemanticCheckerTraverser::VisitCXXMethodDecl(clang::CXXMethodDecl *D) {
  // ATTENTION This is called after VisitFunctionDecl
  OS << "DEBUG:: VisitCXXMethodDecl\n";
  return true;
}

bool ASaPSemanticCheckerTraverser::
VisitCXXConstructorDecl(CXXConstructorDecl *D) {
  // ATTENTION This is called after VisitCXXMethodDecl
  OS << "DEBUG:: VisitCXXConstructorDecl\n";
  return true;
}

bool ASaPSemanticCheckerTraverser::
VisitCXXDestructorDecl(CXXDestructorDecl *D) {
  // ATTENTION This is called after VisitCXXMethodDecl
  OS << "DEBUG:: VisitCXXDestructorDecl\n";
  return true;
}

bool ASaPSemanticCheckerTraverser::
VisitCXXConversionDecl(CXXConversionDecl *D) {
  // ATTENTION This is called after VisitCXXMethodDecl
  OS << "DEBUG:: VisitCXXConversionDecl\n";
  return true;
}

bool ASaPSemanticCheckerTraverser::
VisitFunctionTemplateDecl(FunctionTemplateDecl *D) {
  OS << "DEBUG:: VisitFunctionTemplateDecl:";
  D->print(OS, Ctx.getPrintingPolicy());
  OS << "\n";
  OS << "DEBUG:: it is " << (D->isTemplateDecl() ? "" : "NOT ")
    << "a template\n";
  OS << "DEBUG:: it is " << (D->isTemplateParameter() ? "" : "NOT ")
    << "a template PARAMETER\n";
  NextFunctionIsATemplatePattern = true;
  return true;
}



