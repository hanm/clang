//===--- AnnotationSchemes.cpp - Safe Parallelism checker ---*- C++ -*-----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the various AnnotationScheme classes used by the Safe
// Parallelism checker, which tries to prove the safety of parallelism
// given region and effect annotations.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"

#include "ASaPAnnotationScheme.h"
#include "ASaPSymbolTable.h"
#include "ASaPType.h"
#include "Effect.h"
#include "Rpl.h"

namespace clang {
namespace asap {

inline AnnotationSet AnnotationScheme::
helperMakeGlobalType(const ValueDecl *D, long ArgNum) {
  AnnotationSet Result;

  RplVector RplV;
  for (int I = 0; I < ArgNum; ++I) {
    RplV.push_back(ConcreteRpl(*SymbolTable::GLOBAL_RplElmt));
  }
  Result.T = new ASaPType(D->getType(), SymT.getInheritanceMap(D), &RplV);
  return Result;
}

inline AnnotationSet AnnotationScheme::
helperMakeLocalType(const ValueDecl *D, long ArgNum) {
  AnnotationSet Result;
  int I = 0;
  RplVector RplV;
  if (!D->getType()->isReferenceType()) {
    RplV.push_back(ConcreteRpl(*SymbolTable::LOCAL_RplElmt));
    ++I;
  }
  for (; I < ArgNum; ++I) {
    RplV.push_back(ConcreteRpl(*SymbolTable::GLOBAL_RplElmt));
  }
  Result.T = new ASaPType(D->getType(), SymT.getInheritanceMap(D), &RplV);
  return Result;
}

inline AnnotationSet AnnotationScheme::
helperMakeVarType(const ValueDecl *D, long ArgNum) {
  AnnotationSet Result;
  //assert(false && "method not implemented yet");
  RplVector RplVec;
  for (int I = 0; I < ArgNum; ++I) {
    // 1. Create new RplVarElement
    VarRpl *RplVar = SymT.createFreshRplVar(D);
    OSv2 << "DEBUG:: RplVar = " << RplVar->toString() << "\n";
    // 2. Push it back
    RplVec.push_back(RplVar);
  }
  Result.T = new ASaPType(D->getType(), SymT.getInheritanceMap(D), &RplVec);
  return Result;
}

inline AnnotationSet AnnotationScheme::
helperMakeWritesLocalEffectSummary(const FunctionDecl *D) {
  AnnotationSet Result;
  // Writes Local
  ConcreteRpl LocalRpl(*SymbolTable::LOCAL_RplElmt);
  Effect WritesLocal(Effect::EK_WritesEffect, &LocalRpl);
  Result.EffSum = new ConcreteEffectSummary(WritesLocal);

  return Result;
}

inline AnnotationSet AnnotationScheme::
helperMakeParametricType(const DeclaratorDecl *D, long ArgNum, QualType QT) {

  AnnotationSet Result;
  RplVector RplV;
  int I = 0;
  Result.ParamVec = new ParameterVector();

  OSv2 << "DEBUG:: QT (" << (QT->isScalarType()? "is Scalar" : "is *NOT* Scalar")
       << ") = " << QT.getAsString() << "\n";
  if (QT->isScalarType()) {
    // 1st Arg = Local, then create a new parameter for each subsequent one
    I = 1;
    RplV.push_back(ConcreteRpl(*SymbolTable::LOCAL_RplElmt));
  }
  for (; I < ArgNum; ++I) {
    StringRef ParamName = SymT.makeFreshParamName(D->getNameAsString());
    ParamRplElement Param(ParamName, ParamName);
    Result.ParamVec->push_back(Param); // makes a (persistent) copy of Param
    RplV.push_back(ConcreteRpl(*Result.ParamVec->back())); // use the persistent copy
  }
  Result.T = new ASaPType(D->getType(), SymT.getInheritanceMap(D), &RplV);
  return Result;

}

inline AnnotationSet AnnotationScheme::
helperMakeClassParams(const RecordDecl *D) {
  AnnotationSet Result;
  StringRef ParamName = SymT.makeFreshParamName(D->getNameAsString());

  ParamRplElement Param(ParamName, ParamName);
  Result.ParamVec = new ParameterVector(Param);
  return Result;
}

RplVector *AnnotationScheme::
helperMakeBaseTypeArgs(const RecordDecl *Derived, long ArgNum) {
  RplVector *Result = 0;
  const ParameterVector *ParamV = SymT.getParameterVector(Derived);
  OSv2 << "DEBUG:: helperMakeBaseTypeArgs:: ParamV = " << ParamV << "\n";
  if (ParamV) {
    OSv2 << "   (size=" << ParamV->size() << ") \n";
    OSv2 << "   "; ParamV->print(OSv2); OSv2 << "\n";
  }
  OSv2 << "        Derived:";
  Derived->print(OSv2);
  OSv2 << "\n";

  if (ParamV && ParamV->size() > 0) {
    Result = new RplVector();
    for(int I = 0; I < ArgNum; ++I) {
      Result->push_back(ConcreteRpl(*ParamV->front()));
    }
  }
  return Result;
}

///////////////////////////////////////////////////////////////////////////////
AnnotationSet ParametricAnnotationScheme::
makeClassParams(const RecordDecl *D) {
  // we don't need a class parameter if the class has no fields
  bool GenerateParam = false;
  if (!D->field_empty()) {
    GenerateParam = true;
  } else {
    if (const CXXRecordDecl *CXXD = dyn_cast<CXXRecordDecl>(D)) {
      if (const CXXRecordDecl *CXXDef = CXXD->getDefinition()) {
        for (CXXRecordDecl::base_class_const_iterator
               I = CXXDef->bases_begin(), E = CXXDef->bases_end();
             I!=E; ++I) {
          QualType BaseQT = (*I).getType();
          // Check if the base class takes no region arguments
          ResultTriplet ResTriplet =
            SymT.getRegionParamCount(BaseQT);
          // If the base type is a template type variable, skip the check.
          // We will only check fully instantiated template code.
          if (ResTriplet.ResKin!=RK_OK)
            continue;

          assert(ResTriplet.ResKin==RK_OK && "Unknown number of region parameters");
          if (ResTriplet.NumArgs>0) {
            GenerateParam = true;
            break;
          }
        } // end for-all base classes
      }
    }
  }

  if (GenerateParam) {
    return helperMakeClassParams(D);
  } else {
    AnnotationSet Result;
    return Result;
  }
}

AnnotationSet ParametricAnnotationScheme::
makeGlobalType(const VarDecl *D, long ArgNum) {
  return helperMakeGlobalType(D, ArgNum);
}

AnnotationSet ParametricAnnotationScheme::
makeStackType(const VarDecl *D, long ArgNum) {
  return helperMakeLocalType(D, ArgNum);
}

AnnotationSet ParametricAnnotationScheme::
makeFieldType(const FieldDecl *D, long ArgNum) {
  assert(D && "Internal Error: Unexpected null-pointer declaration.");
  const RecordDecl *ReD = D->getParent();
  assert(ReD && "Internal Error: Unexpected null-pointer enclosing class declaration");
  const ParameterVector *ParamV = SymT.getParameterVector(ReD);

  AnnotationSet Result;
  RplVector RplV;

  // NOTE: We are not allowed to add a parameter here.
  // We should do it though the makeClassParams method during the
  // CollectRegionNamesAndParams pass.
  assert(ParamV && ParamV->size() > 0
         && "Internal error: empty region parameter vector.");

  const ParamRplElement &Param = *ParamV->getParamAt(0);
  for (int I = 0; I < ArgNum; ++I) {
    RplV.push_back(ConcreteRpl(Param));
  }
  Result.T = new ASaPType(D->getType(), SymT.getInheritanceMap(D), &RplV);
  return Result;
}

AnnotationSet ParametricAnnotationScheme::
makeParamType(const ParmVarDecl *D, long ArgNum) {
  return helperMakeParametricType(D, ArgNum, D->getType());
}

AnnotationSet ParametricAnnotationScheme::
makeReturnType(const FunctionDecl *D, long ArgNum) {
  QualType QT = D->getType();
  assert(QT->isFunctionType());
  const FunctionType *FT = QT->getAs<FunctionType>();
  QT = FT->getReturnType();
  return helperMakeParametricType(D, ArgNum, QT);
}

AnnotationSet ParametricAnnotationScheme::
makeEffectSummary(const FunctionDecl *D) {
  return helperMakeWritesLocalEffectSummary(D);
}

RplVector *ParametricAnnotationScheme::
makeBaseTypeArgs(const RecordDecl *Derived, long ArgNum) {
  return helperMakeBaseTypeArgs(Derived, ArgNum);
}

///////////////////////////////////////////////////////////////////////////////

AnnotationSet SimpleAnnotationScheme::
makeClassParams(const RecordDecl *D) {
  AnnotationSet Result;
  Result.ParamVec = 0;
  return Result;
}

AnnotationSet SimpleAnnotationScheme::
makeGlobalType(const VarDecl *D, long ArgNum) {
  return helperMakeGlobalType(D, ArgNum);
}

AnnotationSet SimpleAnnotationScheme::
makeStackType(const VarDecl *D, long ArgNum) {
  return helperMakeLocalType(D, ArgNum);
}

AnnotationSet SimpleAnnotationScheme::
makeFieldType(const FieldDecl *D, long ArgNum) {
  return helperMakeGlobalType(D, ArgNum);
}

AnnotationSet SimpleAnnotationScheme::
makeParamType(const ParmVarDecl *D, long ArgNum) {
  return helperMakeLocalType(D, ArgNum);
}

AnnotationSet SimpleAnnotationScheme::
makeReturnType(const FunctionDecl *D, long ArgNum) {
  return helperMakeLocalType(D, ArgNum);
}

AnnotationSet SimpleAnnotationScheme::
makeEffectSummary(const FunctionDecl *D) {
  return helperMakeWritesLocalEffectSummary(D);
}

RplVector *SimpleAnnotationScheme::
makeBaseTypeArgs(const RecordDecl *Derived, long ArgNum) {
  return helperMakeBaseTypeArgs(Derived, ArgNum);
}

///////////////////////////////////////////////////////////////////////////////

AnnotationSet CheckGlobalsAnnotationScheme::
makeClassParams(const RecordDecl *D) {
  AnnotationSet Result;
  Result.ParamVec = 0;
  return Result;
}

AnnotationSet CheckGlobalsAnnotationScheme::
makeGlobalType(const VarDecl *D, long ArgNum) {
  return helperMakeGlobalType(D, ArgNum);
}

AnnotationSet CheckGlobalsAnnotationScheme::
makeStackType(const VarDecl *D, long ArgNum) {
  return helperMakeLocalType(D, ArgNum);
}

AnnotationSet CheckGlobalsAnnotationScheme::
makeFieldType(const FieldDecl *D, long ArgNum) {
  return helperMakeGlobalType(D, ArgNum);
}

AnnotationSet CheckGlobalsAnnotationScheme::
makeParamType(const ParmVarDecl *D, long ArgNum) {
  return helperMakeLocalType(D, ArgNum);
}

AnnotationSet CheckGlobalsAnnotationScheme::
makeReturnType(const FunctionDecl *D, long ArgNum) {
  return helperMakeLocalType(D, ArgNum);
}

AnnotationSet CheckGlobalsAnnotationScheme::
makeEffectSummary(const FunctionDecl *D) {
  AnnotationSet Result;
  // Writes Local
  ConcreteRpl LocalRpl(*SymbolTable::LOCAL_RplElmt);
  Effect WritesLocal(Effect::EK_WritesEffect, &LocalRpl);
  ConcreteEffectSummary *CES = new ConcreteEffectSummary(WritesLocal);
  Result.EffSum = CES;

  // Reads Global
  ConcreteRpl GlobalRpl(*SymbolTable::GLOBAL_RplElmt);
  Effect ReadsGlobal(Effect::EK_ReadsEffect, &GlobalRpl);
  CES->insert(ReadsGlobal);
  return Result;
}

RplVector *CheckGlobalsAnnotationScheme::
makeBaseTypeArgs(const RecordDecl *Derived, long ArgNum) {
  return helperMakeBaseTypeArgs(Derived, ArgNum);
}

///////////////////////////////////////////////////////////////////////////////
AnnotationSet EffectInferenceAnnotationScheme::
makeEffectSummary(const FunctionDecl *D) {
  AnnotationSet Result;
  Result.EffSum = SymT.createFreshEffectSumVar(D);
  return Result;
}

///////////////////////////////////////////////////////////////////////////////
AnnotationSet InferenceAnnotationScheme::
makeGlobalType(const VarDecl *D, long ArgNum) {
  return helperMakeVarType(D, ArgNum);
}

AnnotationSet InferenceAnnotationScheme::
makeStackType(const VarDecl *D, long ArgNum) {
  return helperMakeVarType(D, ArgNum);
}

AnnotationSet InferenceAnnotationScheme::
makeFieldType(const FieldDecl *D, long ArgNum) {
  return helperMakeVarType(D, ArgNum);
}

AnnotationSet InferenceAnnotationScheme::
makeParamType(const ParmVarDecl *D, long ArgNum) {
  return helperMakeVarType(D, ArgNum);

}

AnnotationSet InferenceAnnotationScheme::
makeReturnType(const FunctionDecl *D, long ArgNum) {
  return helperMakeVarType(D, ArgNum);
}

AnnotationSet InferenceAnnotationScheme::
makeEffectSummary(const FunctionDecl *D) {
  AnnotationSet Result;
  Result.EffSum = SymT.createFreshEffectSumVar(D);
  return Result;
}

} // end namespace asap
} // end namespace clang
