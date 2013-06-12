//=== SemanticChecker.cpp - Safe Parallelism checker -----*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This file defines the Semantic Checker pass of the Safe Parallelism
// checker, which tries to prove the safety of parallelism given region
// and effect annotations.
//
//===----------------------------------------------------------------===//

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "llvm/ADT/SmallVector.h"

#include "ASaPAnnotationScheme.h"
#include "ASaPType.h"
#include "ASaPSymbolTable.h"
#include "ASaPUtil.h"
#include "Effect.h"
#include "Rpl.h"
#include "SemanticChecker.h"
#include "Substitution.h"

namespace clang {
namespace asap {

template<typename AttrType>
void ASaPSemanticCheckerTraverser::helperPrintAttributes(Decl *D) {
  for (specific_attr_iterator<AttrType>
       I = D->specific_attr_begin<AttrType>(),
       E = D->specific_attr_end<AttrType>();
       I != E; ++I) {
    (*I)->printPretty(OS, Ctx.getPrintingPolicy());
    OS << "\n";
  }
}

template<typename AttrType>
bool ASaPSemanticCheckerTraverser::checkRegionOrParamDecls(Decl *D) {
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

template<typename AttrType>
bool ASaPSemanticCheckerTraverser::checkRpls(Decl* D) {
  bool Success = true;
  const RplVector *RV = 0;
  for (specific_attr_iterator<AttrType>
       I = D->specific_attr_begin<AttrType>(),
       E = D->specific_attr_end<AttrType>();
       I != E; ++I) {
    Success &= checkRpls(D, *I, (*I)->getRpl());
  }
  if (!Success) {
    delete RV;
    RV = 0;
    FatalError = true;
  }
  return Success;
}

template<typename AttrType>
void ASaPSemanticCheckerTraverser::
buildPartialEffectSummary(FunctionDecl *D, EffectSummary &ES) {
  for (specific_attr_iterator<AttrType>
       I = D->specific_attr_begin<AttrType>(),
       E = D->specific_attr_end<AttrType>();
       I != E; ++I) {
    Effect::EffectKind EK = getEffectKind(*I);
    RplVector *Tmp = RplVecAttrMap[*I];

    if (Tmp) { /// Tmp may be NULL if the RPL was ill formed (e.g., contained
               /// undeclared RPL elements).
      for (size_t Idx = 0; Idx < Tmp->size(); ++Idx) {
        const Effect E(EK, Tmp->getRplAt(Idx), *I);
        bool Success = ES.insert(E);
        assert(Success);
      }
    }
  }
}

inline void ASaPSemanticCheckerTraverser::
addASaPTypeToMap(ValueDecl *ValD, ASaPType *T) {
  assert(!SymT.hasType(ValD));
  if (T) {
    OS << "Debug :: adding type: " << T->toString(Ctx) << " to Decl: ";
    ValD->print(OS, Ctx.getPrintingPolicy());
    OS << "\n";
    if (T->hasInheritanceMap()) {
      OS << "DEBUG:: Type has an inheritance map!\n";
    }
    bool Result = SymT.setType(ValD, T);
    assert(Result);
  }
}

void ASaPSemanticCheckerTraverser::
addASaPTypeToMap(ValueDecl *ValD, RplVector *RplV, Rpl *InRpl) {
  assert(!SymT.hasType(ValD));
  const InheritanceMapT *IMap = SymT.getInheritanceMap(ValD);
  ASaPType *T = new ASaPType(ValD->getType(), IMap, RplV, InRpl);
  OS << "DEBUG:: D->getType() = ";
  ValD->getType().print(OS, Ctx.getPrintingPolicy());
  OS << ", isFunction = " << ValD->getType()->isFunctionType() << "\n";
  OS << "Debug:: RV.size=" << (RplV ? RplV->size() : 0)

    << ", T.RV.size=" << T->getArgVSize() << "\n";
  addASaPTypeToMap(ValD, T);
}

void ASaPSemanticCheckerTraverser::
addASaPBaseTypeToMap(CXXRecordDecl *CXXRD,
                     QualType BaseQT, RplVector *RplVec) {
  if (!RplVec)
    return; // Nothing to do
  const ParameterVector *ParV = SymT.getParameterVectorFromQualType(BaseQT);

  // These next two assertions should have been checked before calling this
  assert(ParV && "Base class takes no region parameter");
  assert(ParV->size() == RplVec->size()
         && "Base class and RPL vector must have the same # of region args");
  // Build Substitution Vector
  SubstitutionVector *SubV = new SubstitutionVector();
  SubV->buildSubstitutionVector(ParV, RplVec);

  const RecordType *RT = BaseQT->getAs<RecordType>();
  assert(RT);
  RecordDecl *BaseD = RT->getDecl();
  assert(BaseD);

  SymT.addBaseTypeAndSub(CXXRD, BaseD, SubV);
}

void ASaPSemanticCheckerTraverser::
emitRedeclaredRegionName(const Decl *D, const StringRef &Str) {
  StringRef BugName = "region name already declared at this scope";
  helperEmitDeclarationWarning(BR, D, Str, BugName);
  // Not a Fatal Error
}

inline void ASaPSemanticCheckerTraverser::
emitRedeclaredRegionParameter(const Decl *D, const StringRef &Str) {
  FatalError = true;
  StringRef BugName = "region parameter already declared at this scope";
  helperEmitDeclarationWarning(BR, D, Str, BugName);
}

void ASaPSemanticCheckerTraverser::
emitMisplacedRegionParameter(const Decl *D,
                             const Attr* A,
                             const StringRef &S) {
  StringRef BugName = "Misplaced Region Parameter: Region parameters "
                        "may only appear at the head of an RPL.";
  helperEmitAttributeWarning(BR, D, A, S, BugName);
}

void ASaPSemanticCheckerTraverser::
emitUndeclaredRplElement(const Decl *D,
                         const Attr *Attr,
                         const StringRef &Str) {
  StringRef BugName = "RPL element was not declared";
  helperEmitAttributeWarning(BR, D, Attr, Str, BugName);
}

void ASaPSemanticCheckerTraverser::
emitNameSpecifierNotFound(const Decl *D, const Attr *A, StringRef Name) {
  StringRef BugName = "Name specifier was not found";
  helperEmitAttributeWarning(BR, D, A, Name, BugName);
}

void ASaPSemanticCheckerTraverser::
emitMissingRegionArgs(const Decl *D, const Attr* A, int ParamCount) {
  FatalError = true;
  std::string BugName;
  llvm::raw_string_ostream BugNameOS(BugName);
  BugNameOS << "expects " << ParamCount
    << " region arguments [-> missing region argument(s)]";

  std::string SBuf;
  llvm::raw_string_ostream DeclOS(SBuf);
  D->print(DeclOS, Ctx.getPrintingPolicy());

  helperEmitDeclarationWarning(BR, D, DeclOS.str(), BugNameOS.str());
}

void ASaPSemanticCheckerTraverser::
emitUnknownNumberOfRegionParamsForType(const Decl *D) {
  FatalError = true;
  std::string BugName = "unknown number of region parameters for type";

  std::string sbuf;
  llvm::raw_string_ostream strbuf(sbuf);
  D->print(strbuf, Ctx.getPrintingPolicy());

  helperEmitDeclarationWarning(BR, D, strbuf.str(), BugName);
}

void ASaPSemanticCheckerTraverser::
emitSuperfluousRegionArg(const Decl *D, const Attr *A,
                         int ParamCount, StringRef Str) {
  FatalError = true;
  std::string BugName;
  llvm::raw_string_ostream BugNameOS(BugName);
  BugNameOS << "expects " << ParamCount
    << " region arguments [-> superfluous region argument(s)]";

  helperEmitAttributeWarning(BR, D, A, Str, BugNameOS.str());
}

void ASaPSemanticCheckerTraverser::
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

void ASaPSemanticCheckerTraverser::
emitCanonicalDeclHasSmallerEffectSummary(const Decl *D, const StringRef &Str) {
  StringRef BugName = "effect summary of canonical declaration does not cover"\
    " the summary of this declaration";
  helperEmitDeclarationWarning(BR, D, Str, BugName);
}

void ASaPSemanticCheckerTraverser::
emitEffectCovered(const Decl *D, const Effect *E1, const Effect *E2) {
  // warning: e1 is covered by e2
  StringRef BugName = "effect summary is not minimal";
  std::string sbuf;
  llvm::raw_string_ostream StrBuf(sbuf);
  StrBuf << "'"; E1->print(StrBuf);
  StrBuf << "' covered by '";
  E2->print(StrBuf); StrBuf << "'";
  //StrBuf << BugName;

  StringRef BugStr = StrBuf.str();
  helperEmitAttributeWarning(BR, D, E1->getAttr(), BugStr, BugName, false);
}

void ASaPSemanticCheckerTraverser::
emitNoEffectInNonEmptyEffectSummary(const Decl *D, const Attr *A) {
  StringRef BugName = "no_effect is illegal in non-empty effect summary";
  StringRef BugStr = "";

  helperEmitAttributeWarning(BR, D, A, BugStr, BugName, false);
}

void ASaPSemanticCheckerTraverser::
emitMissingBaseClassArgument(const Decl *D, StringRef Str) {
  FatalError = true;
  StringRef BugName = "base class requires region argument(s)";
  helperEmitDeclarationWarning(BR, D, Str, BugName);
}

void ASaPSemanticCheckerTraverser::
emitAttributeMustReferToDirectBaseClass(const Decl *D,
                                        const RegionBaseArgAttr *A) {
  StringRef BugName =
    "attribute's first argument must refer to direct base class";
  helperEmitAttributeWarning(BR, D, A, A->getBaseType(), BugName);
}

void ASaPSemanticCheckerTraverser::
emitDuplicateBaseArgAttributesForSameBase(const Decl *D,
                                          const RegionBaseArgAttr *A1,
                                          const RegionBaseArgAttr *A2) {
  // Not a fatal error
  StringRef BugName = "duplicate attribute for single base class specifier";

  helperEmitAttributeWarning(BR, D, A1, A1->getBaseType(), BugName);
}

void ASaPSemanticCheckerTraverser::
emitMissingBaseArgAttribute(const Decl *D, StringRef BaseClass) {
  FatalError = true;
  StringRef BugName = "missing base_arg attribute";
  helperEmitDeclarationWarning(BR, D, BaseClass, BugName);

}

void ASaPSemanticCheckerTraverser::
emitEmptyStringRplDisallowed(const Decl *D, Attr *A) {
  FatalError = true;
  StringRef BugName = "the empty string is not a valid RPL";

  helperEmitAttributeWarning(BR, D, A, "", BugName);
}

// End Emit Functions
//////////////////////////////////////////////////////////////////////////
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
checkBaseTypeRegionArgs(NamedDecl *D, const RegionBaseArgAttr *Att,
                        QualType BaseQT, const Rpl *DefaultInRpl) {
  assert(D && "Decl argument should not be null");
  assert(Att && "Attr argument should not be null");
  RplVector *RplVec = RplVecAttrMap[Att];
  if (Att && !RplVec && FatalError)
    return; // don't check an error already occured

  // How many In/Arg annotations does the type require?
  OS << "DEBUG:: calling getRegionParamCount on type: ";
  BaseQT.print(OS, Ctx.getPrintingPolicy());
  OS << "\n";

  ResultTriplet ResTriplet = SymT.getRegionParamCount(BaseQT);

  checkParamAndArgCounts(D, Att, BaseQT, ResTriplet, RplVec, DefaultInRpl);
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

  ResultTriplet ResTriplet = SymT.getRegionParamCount(QT);
  ResultKind ResKin = ResTriplet.ResKin;

  if (ResKin == RK_NOT_VISITED) {
    assert(ResTriplet.DeclNotVis);
    OS << "DEBUG:: DeclNotVisited : ";
    ResTriplet.DeclNotVis->print(OS, Ctx.getPrintingPolicy());
    OS << "\n";
    // Calling visitor on the Declaration which has not yet been visited
    // to learn how many region parameters this type takes.
    VisitRecordDecl(ResTriplet.DeclNotVis);
    OS << "DEBUG:: done with the recursive visiting\n";
    checkTypeRegionArgs(D, DefaultInRpl);
  } else {
    checkParamAndArgCounts(D, A, QT, ResTriplet, RplVec, DefaultInRpl);
  }

  OS << "DEBUG:: DONE checkTypeRegionArgs\n";
}

void ASaPSemanticCheckerTraverser::
addToMap(Decl *D, RplVector *RplVec, QualType QT) {
  if (ValueDecl *VD = dyn_cast<ValueDecl>(D)) {
    addASaPTypeToMap(VD, RplVec, 0);
  } else if (CXXRecordDecl *CXXRD = dyn_cast<CXXRecordDecl>(D)) {
    addASaPBaseTypeToMap(CXXRD, QT, RplVec);
  } else {
    assert(false && "Called 'checkParamAndArgCounts' with invalid Decl type.");
  }
}


void ASaPSemanticCheckerTraverser::
checkParamAndArgCounts(NamedDecl *D, const Attr* Att, QualType QT,
                       const ResultTriplet &ResTriplet,
                       RplVector *RplVec, const Rpl *DefaultInRpl) {

  ResultKind ResKin = ResTriplet.ResKin;
  long ParamCount = ResTriplet.NumArgs;
  OS << "DEBUG:: called 'getRegionParamCount(QT)' : "
     << "(" << stringOf(ResKin)
     << ", " << ParamCount << ") DONE!\n";
  long ArgCount = (RplVec) ? RplVec->size() : 0;
  OS << "ArgCount = " << ArgCount << "\n";
  OS << "DefaultInRpl ="  <<  ((DefaultInRpl) ? DefaultInRpl->toString() : "")
     << "\n";
  switch(ResKin) {
  case RK_ERROR:
    emitUnknownNumberOfRegionParamsForType(D);
    break;
  case RK_VAR:
    // Type is TemplateTypeParam -- Any number of region args
    // could be ok. At least ParamCount are needed though.
    if (ParamCount > ArgCount &&
      ParamCount > ArgCount + (DefaultInRpl ? 1 : 0)) {
        emitMissingRegionArgs(D, Att, ParamCount);
    }
    break;
  case RK_OK:
    OS << "DEBUG:: here01\n";
    if (ParamCount > ArgCount &&
        ParamCount > ArgCount + (DefaultInRpl?1:0)) {
      ValueDecl *ValD = dyn_cast<ValueDecl>(D);
      OS << "DEBUG:: here02\n";
      if (!RplVec && ValD) {
        OS << "DEBUG:: here03\n";
        AnnotationSet AnSe = SymT.makeDefaultType(ValD, ParamCount);
        OS << "DEBUG:: Default type created:" << AnSe.T->toString() << "\n";
        addASaPTypeToMap(ValD, AnSe.T);
      } else {
        emitMissingRegionArgs(D, Att, ParamCount);
      }
    } else if (ParamCount < ArgCount &&
               ParamCount < ArgCount + (DefaultInRpl?1:0)) {
      // Superfluous region args
      std::string SBuf;
      llvm::raw_string_ostream BufStream(SBuf);
      int I = ParamCount;
      for (; I < ArgCount-1; ++I) {
        RplVec->getRplAt(I)->print(BufStream);
        BufStream << ", ";
      }
      if (I < ArgCount) {
        RplVec->getRplAt(I)->print(BufStream);
      }
      emitSuperfluousRegionArg(D, Att, ParamCount, BufStream.str());
    } else {
      assert(ParamCount>=0);
      OS << "DEBUG:: here1\n";
      if (ParamCount > ArgCount) {
        assert(DefaultInRpl);
        if (RplVec) {
          RplVec->push_front(DefaultInRpl);
        } else {
          RplVec = new RplVector(*DefaultInRpl);
          OS << "DEBUG:: here2\n";
        }
      }
      assert(ParamCount == 0 ||
        (size_t)ParamCount == RplVec->size());
      OS << "DEBUG:: here3\n";
      if (ValueDecl *VD = dyn_cast<ValueDecl>(D)) {
        OS << "DEBUG:: here4\n";
        addASaPTypeToMap(VD, RplVec, 0);
      } else if (CXXRecordDecl *CXXRD = dyn_cast<CXXRecordDecl>(D)) {
        addASaPBaseTypeToMap(CXXRD, QT, RplVec);
      } else {
        assert(false && "Called 'checkParamAndArgCounts' with invalid Decl type.");
      }
      OS << "DEBUG:: here5\n";
    }
    break;
    default:
      assert(false && "Called 'checkParamAndArgCounts' with invalid ResTriplet.ResKind");
      break;
  } //  end switch
}

bool ASaPSemanticCheckerTraverser::checkRpls(Decl *D, Attr *Att,
                                             StringRef RplsStr) {
  /// First check that we have not already parsed this attribute's RPL
  RplVector *RV = RplVecAttrMap[Att];
  if (RV)
    return true;
  // else:
  bool Failed = false;

  RV = new RplVector();
  llvm::SmallVector<StringRef, 8> RplVec;
  RplsStr.split(RplVec, ","); // split into vector of string per RPL
  //OS << "DEBUG:: checkRpls: #Rpls=" << RplVec.size() << "\n";

  for (size_t I = 0 ; !Failed && I != RplVec.size(); ++I) {
    Rpl *R = checkRpl(D, Att, RplVec[I].trim());
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
    RplVecAttrMap[Att] = RV;
    return true;
  }
}

Rpl *ASaPSemanticCheckerTraverser::checkRpl(Decl *D, Attr *Att,
                                            StringRef RplStr) {
  if (RplStr.size() <= 0) {
    emitEmptyStringRplDisallowed(D, Att);
    return 0;
  }
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
        emitNameSpecifierNotFound(D, Att, Vec[0]);
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
          emitNameSpecifierNotFound(D, Att, Vec[I]);
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
      emitUndeclaredRplElement(D, Att, Head);
      Result = false;
    } else { // RplEl != NULL
      OS << "DEBUG:: found RplElement:" << RplEl->getName() << "\n";
      if (Count>0 && (isa<ParamRplElement>(RplEl)
        || isa<CaptureRplElement>(RplEl))) {
          /// Error: region parameter is only allowed at the head of an Rpl
          emitMisplacedRegionParameter(D, Att, Head);
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
      bool Success = ES.insert(E);
      assert(Success);
    }
  }
}

const RegionBaseArgAttr *ASaPSemanticCheckerTraverser::
findBaseArg(const CXXRecordDecl *D, StringRef BaseStr) {
  OS << "DEBUG:: findBaseArg for type '" << BaseStr <<"'\n";
  const RegionBaseArgAttr *Result = 0;
  bool multipleAttrs = false;
  // iterate over base_args of D
  for (specific_attr_iterator<RegionBaseArgAttr>
       I = D->specific_attr_begin<RegionBaseArgAttr>(),
       E = D->specific_attr_end<RegionBaseArgAttr>();
       I != E; ++I) {
    RegionBaseArgAttr *A = *I;
    // FIXME: string comparisons are only going to get as this far...
    if (!BaseStr.compare(A->getBaseType())) { // found!!
      if (!Result) {
        Result = *I;
      } else {
        multipleAttrs = true;
        //emitMultipleAttributesForBaseClass(D, A, BaseStr);
        emitDuplicateBaseArgAttributesForSameBase(D, Result, A);
      }
    }
  }
  return Result;
}

const CXXBaseSpecifier *ASaPSemanticCheckerTraverser::
findBaseDecl(const CXXRecordDecl *D, StringRef BaseStr) {
  const CXXBaseSpecifier *Result = 0;
  for (CXXRecordDecl::base_class_const_iterator
       I = D->bases_begin(), E = D->bases_end();
       I!=E; ++I) {

    std::string BaseClassStr = (*I).getType().getAsString();
    OS << "DEBUG::: BaseClass = " << BaseClassStr << "\n";
    std::string prefix("class ");
    if (!BaseClassStr.compare(0, prefix.length(), prefix)) {
      BaseClassStr = BaseClassStr.substr(prefix.length(),
                                        BaseClassStr.length()-prefix.length());
    }

    // FIXME: string comparisons are only going to get as this far...
    if (BaseStr.compare(BaseClassStr)==0) {
      Result = I;
      break;
    }
  }
  return Result;
}

void ASaPSemanticCheckerTraverser::
checkBaseSpecifierArgs(CXXRecordDecl *D) {
  assert(D);
  OS << "DEBUG:: checkBaseSpecifierArgs\n";
  // 1. Before actually doing any checking, for each base class,
  // check that it has been visited and that we know how many region
  // arguments it takes
  bool Error = false;
  for (CXXRecordDecl::base_class_const_iterator
          I = D->bases_begin(), E = D->bases_end();
       I!=E; ++I) {
    ResultTriplet ResTriplet =
      SymT.getRegionParamCount((*I).getType());
    switch(ResTriplet.ResKin) {
    case RK_NOT_VISITED:
      assert(ResTriplet.DeclNotVis);
      ResTriplet.DeclNotVis->print(OS, Ctx.getPrintingPolicy());
      // Calling visitor on the Declaration which has not yet been visited
      // to learn how many region parameters this type takes.
      VisitRecordDecl(ResTriplet.DeclNotVis);
      break;
    case RK_ERROR:
      emitUnknownNumberOfRegionParamsForType(D);
      Error = true;
      break;
    case RK_VAR:
    case RK_OK:
      // do nothing
      OS << "DEBUG:: #args needed = " << ResTriplet.NumArgs << "\n";
      break;
    }
  }
  OS << "DEBUG:: checkBaseSpecifierArgs (DONE w. Step 1)\n";
  // 2. Check that for each base class there is an attribute.
  for (CXXRecordDecl::base_class_const_iterator
       I = D->bases_begin(), E = D->bases_end();
       I!=E; ++I) {
    std::string BaseClassStr = (*I).getType().getAsString();
    OS << "DEBUG::: BaseClass = " << BaseClassStr << "\n";
    std::string prefix("class ");
    if (!BaseClassStr.compare(0, prefix.length(), prefix)) {
      BaseClassStr = BaseClassStr.substr(prefix.length(),
                                        BaseClassStr.length()-prefix.length());
    }
    OS << "DEBUG::: BaseClass = " << BaseClassStr << "\n";
    const RegionBaseArgAttr *Att = findBaseArg(D, BaseClassStr);
    if (!Att) {
      emitMissingBaseArgAttribute(D, BaseClassStr);
      // TODO: add default instead of giving error
    }
  }

  // 3. For each attributetakes region arguments, find if the needed
  // annotation (attribute) was provided.
  for (specific_attr_iterator<RegionBaseArgAttr>
       I = D->specific_attr_begin<RegionBaseArgAttr>(),
       E = D->specific_attr_end<RegionBaseArgAttr>();
       I != E; ++I) {
    // 1. Check that attribute refers to valid base type
    StringRef BaseStr =  (*I)->getBaseType();
    const CXXBaseSpecifier *BaseSpec = findBaseDecl(D,BaseStr);
    if (!BaseSpec) {
      emitAttributeMustReferToDirectBaseClass(D, *I);
      continue;
    }
    // 2. Check for duplicates
    bool FoundDuplicate = false;
    for (specific_attr_iterator<RegionBaseArgAttr> J = I;
         J != E; ++J) {
      if (J!=I) { // this is because I can't init J = I+1;
        if ((*I)->getBaseType().compare((*J)->getBaseType())==0) {
          emitDuplicateBaseArgAttributesForSameBase(D, *I, *J);
          FoundDuplicate = true;
        }
      }
    }
    if (FoundDuplicate) {
      // skip this one, we'll use the last attribute (this choise is arbitrary)
      continue;
    }
    // 3. Now check that the number of arguments given by the annotation
    // is valid for the base class.
    StringRef Rpls = (*I)->getRpl();
    bool Success = checkRpls(D, *I, Rpls);
    if (Success) {
      //Rpl Local(*SymbolTable::LOCAL_RplElmt);
      checkBaseTypeRegionArgs(D, *I, BaseSpec->getType(), 0);
    }
  }
  OS << "DEBUG:: checkBaseSpecifierArgs (DONE!)\n";
}

ASaPSemanticCheckerTraverser::
ASaPSemanticCheckerTraverser(VisitorBundle &_VB)
  : VB(_VB),
  BR(VB.BR),
  Ctx(VB.Ctx),
  OS(VB.OS),
  SymT(VB.SymT),
  FatalError(false),
  NextFunctionIsATemplatePattern(false) {}

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
  D->dump(OS);
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


/*bool ASaPSemanticCheckerTraverser::TraverseFunctionDecl(FunctionDecl *D) {
  TRY_TO(WalkUpFromFunctionDecl(D));
  { CODE; }
  TRY_TO(TraverseDeclContextHelper(dyn_cast<DeclContext>(D)));
  return true;
}*/
/*
#define TRY_TO(CALL_EXPR) \
  do { if (!getDerived().CALL_EXPR) return false; } while (0)

#define DEF_TRAVERSE_TYPELOC(TYPE, CODE)
bool ASaPSemanticCheckerTraverser::
TraverseFunctionNoProtoTypeLoc(FunctionNoProtoTypeLoc TL) {
    if (getDerived().shouldWalkTypesOfTypeLocs())
      TRY_TO(WalkUpFromFunctionNoProtoType(const_cast<FunctionNoProtoType*>(TL.getTypePtr())));
    TRY_TO(WalkUpFromFunctionNoProtoTypeLoc(TL));                                  \
    { CODE; }                                                           \
    return true;                                                        \
  }
DEF_TRAVERSE_TYPELOC(FunctionNoProtoType, {
    TRY_TO(TraverseTypeLoc(TL.getResultLoc()));
  }
*/

bool ASaPSemanticCheckerTraverser::VisitFunctionDecl(FunctionDecl *D) {
  OS << "DEBUG:: VisitFunctionDecl\n";
  OS << "D->isThisDeclarationADefinition() = "
     << D->isThisDeclarationADefinition() << "\n";
  OS << "D->getTypeSourceInfo() = " << D->getTypeSourceInfo() << "\n";

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
      OS << "DEBUG:: CanFD != D\n";
      OS << "DEBUG:: D="; D->print(OS); OS << "\n";
      OS << "DEBUG:: CanFD="; CanFD->print(OS); OS << "\n";
      OS << "DEBUG:: D="; D->dump(OS); OS << "\n";
      OS << "DEBUG:: CanFD="; CanFD->dump(OS); OS << "\n";

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
        // as that of the canonical declaration.
        // (Makes a copy of the effect summary).
        bool Success = SymT.setEffectSummary(D, CanFD);
        assert(Success);
      }
    } else { // Case 2: Visiting the canonical Decl.
      // This declaration does not get effects copied from other decls.
      // (...hopefully).
      if (ES->size()==0) {
        AnnotationSet AnSe = SymT.makeDefaultEffectSummary(CanFD);
        delete ES;
        ES = AnSe.EffSum;
        OS << "Implicit Effect Summary:\n";
        ES->print(OS);
      } else {
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
      }
      bool Success = SymT.setEffectSummary(D, ES);
      assert(Success);
    }
  }
  return true;
}

bool ASaPSemanticCheckerTraverser::VisitRecordDecl (RecordDecl *D) {
  if (SymT.hasDecl(D)) // in case we have already visited this don't re-visit.
    return true;
  OS << "DEBUG:: printing ASaP attributes for class or struct '";
  OS << D->getDeclName();
  OS << "':\n";
  /// A. Detect Region & Param Annotations
  helperPrintAttributes<RegionAttr>(D);
  helperPrintAttributes<RegionParamAttr>(D);
  helperPrintAttributes<RegionBaseArgAttr>(D);
  /// B. Check Region Names
  checkRegionOrParamDecls<RegionAttr>(D);

  /// C. Check Param Names
  SymT.initParameterVector(D); // An empty param vector means the class was
                               // visited and takes no region arguments.
  checkRegionOrParamDecls<RegionParamAttr>(D);

  /// D. Check BaseArg Attributes (or lack thereof)
  OS << "DEBUG:: D:" << D << "\n";
  OS << "DEBUG:: D->getDefinition:" << D->getDefinition() << "\n";
  OS << "DEBUG:: D:" << D << "\n";

  CXXRecordDecl *CxD = dyn_cast<CXXRecordDecl>(D);
  OS << "DEBUG:: CxD:" << CxD << "\n";

  if (CxD && CxD->getDefinition()) {
    //assert(isa<CXXRecordDecl>(D));
    //assert(CxD->getDefinition());
    OS << "DEBUG:: D is a CXXRecordDecl and has numBases = ";
    unsigned int num = CxD->getNumBases();
    OS <<  num <<"\n";
    checkBaseSpecifierArgs(CxD->getDefinition());
  }
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
  OS << D->getDeclName();
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
    const SpecialRplElement *DefaultEl =
      (D->isStaticLocal() || D->isStaticDataMember()
        || D->getDeclContext()->isFileContext()) ? SymbolTable::GLOBAL_RplElmt
                                                 : SymbolTable::LOCAL_RplElmt;
    Rpl Default(*DefaultEl);
    checkTypeRegionArgs(D, &Default);
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

} // end namespace asap
} // end namespace clang
