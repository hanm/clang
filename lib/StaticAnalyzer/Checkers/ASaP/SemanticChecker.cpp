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
buildPartialEffectSummary(FunctionDecl *D, ConcreteEffectSummary &ES) {
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
        assert(Success && "Internal Error: failed adding effect to summary");
      }
    }
  }
}

inline void ASaPSemanticCheckerTraverser::
addASaPTypeToMap(ValueDecl *ValD, ASaPType *T) {
  ///FIXME: Temporary fix for CLANG AST visitor problem
  if (SymT.hasType(ValD)) {
    OS << "ERROR!! Type already in symbol table while in addASaPTypeToMap:";
    ValD->print(OS, Ctx.getPrintingPolicy());
    OS << "\n";
    // This is an error
    OS << "DEBUG:: D(" << ValD << ") has type " << SymT.getType(ValD)->toString() << "\n";
    OS << "DEBUG:: Trying to add Type    " << T->toString() << "\n";
    delete T;
    return; // Do Nothing.
  }

  assert(!SymT.hasType(ValD));
  if (T) {
    OS << "Debug :: adding type: " << T->toString(Ctx) << " to Decl: ";
    ValD->print(OS, Ctx.getPrintingPolicy());
    OS << "(" << ValD << ")\n";
    if (T->hasInheritanceMap()) {
      OS << "DEBUG:: Type has an inheritance map!\n";
    }
    bool Result = SymT.setType(ValD, T);
    assert(Result);
  }
}

void ASaPSemanticCheckerTraverser::
addASaPTypeToMap(ValueDecl *ValD, RplVector *RplV, Rpl *InRpl) {

  ///FIXME: Temporary fix for CLANG AST visitor problem
  /*if (SymT.hasType(ValD)) {
    OS << "ERROR!!! Type already in symbol table while in addASaPTypeToMap:";
    ValD->print(OS, Ctx.getPrintingPolicy());
    OS << "\n";
    // This is an error
    OS << "DEBUG:: D(" << ValD << ") has type " << SymT.getType(ValD)->toString() << "\n";
    OS << "DEBUG:: Trying to add Type " << T->toString() << "\n";
    delete RplV;
    delete InRpl;
    return; // Do Nothing.
  }*/

  //assert(!SymT.hasType(ValD));
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
  OS << "DEBUG:: Adding Base class to inheritance Map!\n"
     << "      BASE=" << BaseQT.getAsString() << "\n"
     << "   DERIVED=" << CXXRD->getQualifiedNameAsString() << "\n";

  const RecordType *RT = BaseQT->getAs<RecordType>();
  assert(RT);
  RecordDecl *BaseD = RT->getDecl();
  assert(BaseD);

  const ParameterVector *ParV = SymT.getParameterVector(BaseD);
  assert(ParV && "Base class has an uninitialized ParamVec");

  const ParameterVector *DerivedParV = SymT.getParameterVector(CXXRD);
  assert(DerivedParV && "Derived class has an uninitialized ParamVec");

  SubstitutionVector *SubV = new SubstitutionVector();
  if (RplVec) {
    assert(ParV->size() == RplVec->size()
          && "Base class and RPL vector must have the same # of region args");
    // Build Substitution Vector
    SubV->buildSubstitutionVector(ParV, RplVec);
  } // else the Substitution Vector stays empty.

  SymT.addBaseTypeAndSub(CXXRD, BaseD, SubV);
}

void ASaPSemanticCheckerTraverser::
emitMisplacedRegionParameter(const Decl *D,
                             const Attr* A,
                             const StringRef &S) {
  StringRef BugName = "Misplaced Region Parameter: Region parameters "
                        "may only appear at the head of an RPL.";
  helperEmitAttributeWarning(Checker, BR, D, A, S, BugName);
}

void ASaPSemanticCheckerTraverser::
emitUndeclaredRplElement(const Decl *D,
                         const Attr *Attr,
                         const StringRef &Str) {
  StringRef BugName = "RPL element was not declared";
  helperEmitAttributeWarning(Checker, BR, D, Attr, Str, BugName);
}

void ASaPSemanticCheckerTraverser::
emitNameSpecifierNotFound(const Decl *D, const Attr *A, StringRef Name) {
  StringRef BugName = "Name specifier was not found";
  helperEmitAttributeWarning(Checker, BR, D, A, Name, BugName);
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

  helperEmitDeclarationWarning(Checker, BR, D, DeclOS.str(), BugNameOS.str());
}

void ASaPSemanticCheckerTraverser::
emitUnknownNumberOfRegionParamsForType(const Decl *D) {
  FatalError = true;
  std::string BugName = "unknown number of region parameters for type";

  std::string sbuf;
  llvm::raw_string_ostream strbuf(sbuf);
  D->print(strbuf, Ctx.getPrintingPolicy());

  OS << "DEBUG:: " << strbuf.str() << ": " << BugName << "\n";
  helperEmitDeclarationWarning(Checker, BR, D, strbuf.str(), BugName);
}

void ASaPSemanticCheckerTraverser::
emitSuperfluousRegionArg(const Decl *D, const Attr *A,
                         int ParamCount, StringRef Str) {
  FatalError = true;
  std::string BugName;
  llvm::raw_string_ostream BugNameOS(BugName);
  BugNameOS << "expects " << ParamCount
    << " region arguments [-> superfluous region argument(s)]";

  helperEmitAttributeWarning(Checker, BR, D, A, Str, BugNameOS.str());
}

void ASaPSemanticCheckerTraverser::
emitEffectCovered(const Decl *D, const Effect *E1, const Effect *E2) {

  OS << "DEBUG:: effect " << E1->toString()
     << " covered by " << E2->toString() << "\n";

  // warning: e1 is covered by e2
  StringRef BugName = "effect summary is not minimal";
  std::string sbuf;
  llvm::raw_string_ostream StrBuf(sbuf);
  StrBuf << "'"; E1->print(StrBuf);
  StrBuf << "' covered by '";
  E2->print(StrBuf); StrBuf << "'";
  //StrBuf << BugName;

  StringRef BugStr = StrBuf.str();
  helperEmitAttributeWarning(Checker, BR, D, E1->getAttr(), BugStr, BugName, false);
}

void ASaPSemanticCheckerTraverser::
emitNoEffectInNonEmptyEffectSummary(const Decl *D, const Attr *A) {
  StringRef BugName = "no_effect is illegal in non-empty effect summary";
  StringRef BugStr = "";

  helperEmitAttributeWarning(Checker, BR, D, A, BugStr, BugName, false);
}

void ASaPSemanticCheckerTraverser::
emitMissingBaseClassArgument(const Decl *D, StringRef Str) {
  FatalError = true;
  StringRef BugName = "base class requires region argument(s)";
  helperEmitDeclarationWarning(Checker, BR, D, Str, BugName);
}

void ASaPSemanticCheckerTraverser::
emitAttributeMustReferToDirectBaseClass(const Decl *D,
                                        const RegionBaseArgAttr *A) {
  StringRef BugName =
    "attribute's first argument must refer to direct base class";
  helperEmitAttributeWarning(Checker, BR, D, A, A->getBaseType(), BugName);
}

void ASaPSemanticCheckerTraverser::
emitDuplicateBaseArgAttributesForSameBase(const Decl *D,
                                          const RegionBaseArgAttr *A1,
                                          const RegionBaseArgAttr *A2) {
  // Not a fatal error
  StringRef BugName = "duplicate attribute for single base class specifier";

  helperEmitAttributeWarning(Checker, BR, D, A1, A1->getBaseType(), BugName);
}

void ASaPSemanticCheckerTraverser::
emitMissingBaseArgAttribute(const Decl *D, StringRef BaseClass) {
  FatalError = true;
  StringRef BugName = "missing base_arg attribute";
  helperEmitDeclarationWarning(Checker, BR, D, BaseClass, BugName);

}

void ASaPSemanticCheckerTraverser::
emitEmptyStringRplDisallowed(const Decl *D, Attr *A) {
  FatalError = true;
  StringRef BugName = "the empty string is not a valid RPL";

  helperEmitAttributeWarning(Checker, BR, D, A, "", BugName);
}

void ASaPSemanticCheckerTraverser::
emitTemporaryObjectNeedsAnnotation(const CXXTemporaryObjectExpr *Exp,
                                   const CXXRecordDecl *Class) {
  std::string BugString;
  llvm::raw_string_ostream BugStringOS(BugString);
  Exp->printPretty(BugStringOS, 0, Ctx.getPrintingPolicy());

  StringRef BugName = "region argument required but not yet supported in this syntax";
  helperEmitStatementWarning(Checker, BR, SymT.VB.AC, Exp, 0,
                             BugStringOS.str(),
                             BugName, false);
}
// End Emit Functions
//////////////////////////////////////////////////////////////////////////

const RplElement *ASaPSemanticCheckerTraverser::
findRegionOrParamName(const Decl *D, StringRef Name) {
  if (!D)
    return 0;
  /// 1. try to find among regions or region parameters
  const RplElement *Result = SymT.lookupRegionOrParameterName(D, Name);
  if (!Result) {
    if (const FunctionDecl *FD = dyn_cast<FunctionDecl>(D)) {
      const FunctionDecl *CanD = FD->getCanonicalDecl();
      if (CanD && FD != CanD) {
        Result = SymT.lookupRegionOrParameterName(CanD, Name);
      }
    }
  }
  return Result;
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
  RegionArgAttr *Att = D->getAttr<RegionArgAttr>();
  RplVector *RplVec = (Att) ? RplVecAttrMap[Att] : 0;
  if (Att && !RplVec && FatalError)
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

  //assert(ResKin != RK_NOT_VISITED); // Make sure we are not using recursion
  if (ResKin == RK_NOT_VISITED) {
    assert(ResTriplet.DeclNotVis);
    OS << "DEBUG:: DeclNotVisited : ";
    ResTriplet.DeclNotVis->print(OS, Ctx.getPrintingPolicy());
    OS << "\n";
    OS << "DEBUG:: nameAsString:: " << ResTriplet.DeclNotVis->getNameAsString() << "\n";
    ResTriplet.DeclNotVis->dump(OS);
    OS << "\n";
    assert(!ResTriplet.DeclNotVis->getNameAsString().compare("__va_list_tag")
           && "Only expect __va_list_tag decl not to be visited here");
    // Calling visitor on the Declaration which has not yet been visited
    // to learn how many region parameters this type takes.
    VisitRecordDecl(ResTriplet.DeclNotVis);
    OS << "DEBUG:: done with the recursive visiting\n";
    checkTypeRegionArgs(D, DefaultInRpl);
  } else {
    checkParamAndArgCounts(D, Att, QT, ResTriplet, RplVec, DefaultInRpl);
  }

  OS << "DEBUG:: DONE checkTypeRegionArgs\n";
}

void ASaPSemanticCheckerTraverser::
addToMap(Decl *D, RplVector *RplVec, QualType QT) {
  if (ValueDecl *VD = dyn_cast<ValueDecl>(D)) {
    addASaPTypeToMap(VD, RplVec, 0);
  } else if (CXXRecordDecl *CXXRD = dyn_cast<CXXRecordDecl>(D)) {
    // TODO
    //if (CXXRD->isUnion())
    //  addASaPTypeToMap(CXXRD, RplVec, 0);
    //else
    addASaPBaseTypeToMap(CXXRD, QT, RplVec);
  } else {
    assert(false && "Called 'checkParamAndArgCounts' with invalid Decl type.");
  }
}


void ASaPSemanticCheckerTraverser::
helperMissingRegionArgs(NamedDecl *D, const Attr* Att,
                        RplVector *RplVec, long ParamCount) {
  ValueDecl *ValD = dyn_cast<ValueDecl>(D);
  if (!RplVec && ValD) {
    // 1. if no args were given -> try to use defaults
    AnnotationSet AnSe = SymT.makeDefaultType(ValD, ParamCount);

    OS << "DEBUG:: Default type created:" << AnSe.T->toString()
        << "  for decl(" << ValD << "): ";
    ValD->print(OS, Ctx.getPrintingPolicy());
    OS << "\n";
    addASaPTypeToMap(ValD, AnSe.T);
  } else {
    // 2. else, if args were given but not enough -> emit an error
    emitMissingRegionArgs(D, Att, ParamCount);
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
  //OS << "QT:" << QT.getAsString() << "\n";

  // Ignore DefaultInRpl if QualType is a ReferenceType
  if (QT->isReferenceType())
    DefaultInRpl = 0;
  else if (QT->isFunctionType()) {
    const FunctionType *FT = QT->getAs<FunctionType>();
    QualType ResultQT = FT->getReturnType();
    if (ResultQT->isReferenceType())
      DefaultInRpl = 0;
    //OS << "ResultQT:" << ResultQT.getAsString() << " [is Reference="
    //   << (ResultQT->isReferenceType()?"true":"false") << "]\n";
  }

  switch(ResKin) {
  case RK_ERROR:
    emitUnknownNumberOfRegionParamsForType(D);
    break;
  case RK_VAR:
    // Type is TemplateTypeParam -- Any number of region args
    // could be ok. At least ParamCount are needed though.
    if (ParamCount > ArgCount &&
        ParamCount > ArgCount + (DefaultInRpl ? 1 : 0)) {
      // possibly missing region args
      helperMissingRegionArgs(D, Att, RplVec, ParamCount);
    }
    break;
  case RK_OK:
    if (ParamCount > ArgCount &&
        ParamCount > ArgCount + (DefaultInRpl?1:0)) {
      // possibly missing region args
      helperMissingRegionArgs(D, Att, RplVec, ParamCount);
    } else if (ParamCount < ArgCount) {
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
      // This is the case where there were enough region args
      // possibly including the DefaultInRpl
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
      addToMap(D, RplVec, QT);
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
      // Find the specified context
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
    } else { // Vec.size() <= 1 : There was no Context specifier ('::').
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

void ASaPSemanticCheckerTraverser::
buildEffectSummary(FunctionDecl *D, ConcreteEffectSummary &ES) {
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
  for (CXXRecordDecl::base_class_const_iterator
          I = D->bases_begin(), E = D->bases_end();
       I!=E; ++I) {
    ResultTriplet ResTriplet =
      SymT.getRegionParamCount((*I).getType());
    switch(ResTriplet.ResKin) {
    case RK_NOT_VISITED:
      assert(false && "Internal Error: New pre-pass should have found declaration of base class");
      assert(ResTriplet.DeclNotVis);
      ResTriplet.DeclNotVis->print(OS, Ctx.getPrintingPolicy());
      // Calling visitor on the Declaration which has not yet been visited
      // to learn how many region parameters this type takes.
      VisitRecordDecl(ResTriplet.DeclNotVis);
      break;
    case RK_ERROR:
      emitUnknownNumberOfRegionParamsForType(D);
      break;
    case RK_VAR:
    case RK_OK:
      // do nothing
      OS << "DEBUG:: #args needed = " << ResTriplet.NumArgs << "\n";
      break;
    }
  }
  OS << "DEBUG:: checkBaseSpecifierArgs (DONE w. Step 1)\n";
  // 2. Check that for each base class there is an attribute,
  // unless the base class takes no region arguments
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

    // Check if the base class takes no region arguments
    ResultTriplet ResTriplet =
      SymT.getRegionParamCount((*I).getType());
    // If the base type is a template type variable, skip the check.
    // We will only check fully instantiated template code.
    if (ResTriplet.ResKin==RK_VAR)
      continue;

    assert(ResTriplet.ResKin==RK_OK && "Unknown number of region parameters");
    if (ResTriplet.NumArgs==0) {
      // add an empty substitution to the inheritance map
      addASaPBaseTypeToMap(D, (*I).getType(), 0);
    } else {
      const RegionBaseArgAttr *Att = findBaseArg(D, BaseClassStr);
      if (!Att) {
        emitMissingBaseArgAttribute(D, BaseClassStr);
        // TODO: add default instead of giving error
      }
    }
  }

  // 3. For each base_arg attribute:
  //    1. Check that it refers to a valid base type
  //    2. Check for duplicates
  //    3. Check that the number of region args is valid
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
ASaPSemanticCheckerTraverser()
  : Checker(SymbolTable::VB.Checker),
    BR(*SymbolTable::VB.BR),
    Ctx(*SymbolTable::VB.Ctx),
    OS(*SymbolTable::VB.OS),
    SymT(*SymbolTable::Table),
    FatalError(false) {}

ASaPSemanticCheckerTraverser::~ASaPSemanticCheckerTraverser() {
  for(RplVecAttrMapT::const_iterator
    I = RplVecAttrMap.begin(),
    E = RplVecAttrMap.end();
  I != E; ++I) {
    delete (*I).second;
  }
}

bool ASaPSemanticCheckerTraverser::VisitValueDecl(ValueDecl *D) {
  OS << "DEBUG:: VisitValueDecl (" << D << ") : ";
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

bool ASaPSemanticCheckerTraverser::VisitFunctionDecl(FunctionDecl *D) {
  OS << "DEBUG:: VisitFunctionDecl (" << D << ")\n";
  OS << "D->isThisDeclarationADefinition() = "
     << D->isThisDeclarationADefinition() << "\n";
  OS << "D->getTypeSourceInfo() = " << D->getTypeSourceInfo() << "\n";

  OS << "DEBUG:: D " << (D->isTemplateDecl() ? "IS " : "is NOT ")
      << "a template\n";
  OS << "DEBUG:: D " << (D->isTemplateParameter() ? "IS " : "is NOT ")
      << "a template PARAMETER\n";
  OS << "DEBUG:: D " << (D->isFunctionTemplateSpecialization() ? "IS " : "is NOT ")
      << "a function template SPECIALIZATION\n";

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
  /// B.1 Check ReturnType
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
    /// C.0. Check this function does not already have an effect summary.
    ///      There seems to be a bug in the traverser causing a duplicate
    ///      visitation of functions occassionally...
    if (SymT.hasEffectSummary(D)) {
      return true;
    }
    /// C.1. Build Effect Summary
    ConcreteEffectSummary *CES = new ConcreteEffectSummary();
    EffectSummary *ES = CES;
    buildEffectSummary(D, *CES);
    OS << "Effect Summary from source file:\n";
    CES->print(OS);
    const FunctionDecl *CanFD = D->getCanonicalDecl();
    if (!CES || CES->size()==0) {
      AnnotationSet AnSe = SymT.makeDefaultEffectSummary(CanFD);
      delete CES;
      CES = 0;
      ES = AnSe.EffSum;
      OS << "Implicit Effect Summary:\n";
      ES->print(OS);
      OS << "\n";
    } else {
      /// C.2. Check Effect Summary is minimal
      ConcreteEffectSummary::EffectCoverageVector ECV;
      CES->makeMinimal(ECV);
      // Emit "effect not minimal" errors only on canonical declarations
      // because other (re)declarations get attributes copied from the
      // canonical declaration, which would lead to too many false positive
      // errors in this case.
      bool EmitErrors = (D==CanFD) ? true : false;
      while(ECV.size() > 0) {
        std::pair<const Effect*, const Effect*> *PairPtr = ECV.pop_back_val();
        const Effect *E1 = PairPtr->first, *E2 = PairPtr->second;
        if (EmitErrors)
          emitEffectCovered(D, E1, E2);
        delete E1;
        delete E2;
        delete PairPtr;
      }
      OS << "Minimal Effect Summary:\n";
      CES->print(OS);
    }
    bool Success = SymT.setEffectSummary(D, ES);
    assert(Success);

    /// Note: Check Effects covered by canonical Declaration in
    /// subsequent pass because canonical Declaration may not have
    /// been encountered yet.
  }
  return true;
}

bool ASaPSemanticCheckerTraverser::VisitRecordDecl (RecordDecl *D) {
  OS << "DEBUG:: VisitRecordDecl (" << D << ") : ";
  OS << D->getDeclName();
  OS << "':\n";
  /// A. Detect Region & Param Annotations
  helperPrintAttributes<RegionAttr>(D);
  helperPrintAttributes<RegionParamAttr>(D);
  helperPrintAttributes<RegionBaseArgAttr>(D);

  /// C. Check Param Names
  // An empty param vector means the class was visited and takes zero
  // region arguments.
  if (!SymT.hasParameterVector(D))
    SymT.initParameterVector(D);

  /// D. Check BaseArg Attributes (or lack thereof)
  OS << "DEBUG:: D               :" << D << "\n";
  OS << "DEBUG:: D->getDefinition:" << D->getDefinition() << "\n";

  if (D->getDefinition() && D != D->getDefinition()) {
    OS << "DEBUG:: D     :\n"; D->dump(OS); OS << "\n";
    OS << "DEBUG:: D->Def:\n"; D->getDefinition()->dump(OS); OS << "\n";
  }

  CXXRecordDecl *CxD = dyn_cast<CXXRecordDecl>(D);
  OS << "DEBUG:: CxD             :" << CxD << "\n";

  if (CxD && CxD->getDefinition()) {
    //assert(isa<CXXRecordDecl>(D));
    //assert(CxD->getDefinition());
    OS << "DEBUG:: D is a CXXRecordDecl and has numBases = ";
    unsigned int num = CxD->getNumBases();
    OS <<  num << "\n";
    checkBaseSpecifierArgs(CxD->getDefinition());
  }
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
  OS << "DEBUG:: VisitCXXMethodDecl (" << D << ")\n";
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
  return true;
}

bool ASaPSemanticCheckerTraverser::
VisitCXXTemporaryObjectExpr(CXXTemporaryObjectExpr *Exp) {
  OS << "DEBUG:: VisitCXXTemporaryObjectExpr:";
  Exp->printPretty(OS, 0, Ctx.getPrintingPolicy());
  OS << "\n";
  // Check that the type doesn't a param because there is no way to provide it yet
  // using our current syntax.
  // FIXME: one exception is when this not is encountered in a return expression
  CXXRecordDecl *Class = Exp->getConstructor()->getParent();
  const ParameterVector *PVec =SymT.getParameterVector(Class);
  if ( PVec && PVec->size()>0 ) {
    OS << "DEBUG:: ParVec(size) = " << PVec->size() << "\n";
    OS << "DEBUG:: Class = ";
    Class->print(OS);
    OS << "\n";
    // FIXME: we should try to apply the automatic annotation scheme,
    // but there doesn't seem to be a declaration AST node and we have
    // been attatching such information to declarations so far...
    emitTemporaryObjectNeedsAnnotation(Exp, Class);
  }
  return true;

}

bool ASaPSemanticCheckerTraverser::
TraverseTypedefDecl(TypedefDecl *D) {
  OS << "DEBUG:: TraverseTypedefDecl (" << D << ") : ";
  if (D) {
    D->print(OS, Ctx.getPrintingPolicy());
    OS << "\n";
    D->dump(OS);
  }
  // Don't Walk-Up or Visit nodes under a TypedefDecl.
  return true;
}

} // end namespace asap
} // end namespace clang
