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

#include <string>
#include <sstream>

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
helperMakeGlobalType(const VarDecl *D, long ArgNum) {
  AnnotationSet Result;

  RplVector RplV;
  for (int I = 0; I < ArgNum; ++I) {
    RplV.push_back(Rpl(*SymbolTable::GLOBAL_RplElmt));
  }
  Result.T = new ASaPType(D->getType(), SymT.getInheritanceMap(D), &RplV);
  return Result;
}

inline AnnotationSet AnnotationScheme::
helperMakeLocalType(const ValueDecl *D, long ArgNum) {
  AnnotationSet Result;

  RplVector RplV;
  for (int I = 0; I < ArgNum; ++I) {
    RplV.push_back(Rpl(*SymbolTable::LOCAL_RplElmt));
  }
  Result.T = new ASaPType(D->getType(), SymT.getInheritanceMap(D), &RplV);
  return Result;
}


inline AnnotationSet AnnotationScheme::
helperMakeWritesLocalEffectSummary(const FunctionDecl *D) {
  AnnotationSet Result;
  // Writes Local
  Rpl LocalRpl(*SymbolTable::LOCAL_RplElmt);
  Effect WritesLocal(Effect::EK_WritesEffect, &LocalRpl);
  Result.EffSum = new EffectSummary(WritesLocal);

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
    RplV.push_back(Rpl(*SymbolTable::LOCAL_RplElmt));
  }
  for (; I < ArgNum; ++I) {
    std::stringstream ss;
    ss << D->getNameAsString() << "_" << I;
    // FIXME: small memory leak. The allocated string is not deallocated
    // when the ParamRplElement is destroyed at the end of checking.
    std::string *ParamName = new std::string(ss.str());
    ParamRplElement Param(*ParamName);
    Result.ParamVec->push_back(Param); // makes a (persistent) copy of Param
    RplV.push_back(Rpl(*Result.ParamVec->back())); // use the persistent copy
  }
  Result.T = new ASaPType(D->getType(), SymT.getInheritanceMap(D), &RplV);
  return Result;

}

///////////////////////////////////////////////////////////////////////////////
AnnotationSet ParametricAnnotationScheme::
makeClassParams(const RecordDecl *D) {
  AnnotationSet Result;
  ParamRplElement Param("P");
  Result.ParamVec = new ParameterVector(Param);
  return Result;
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
  const RecordDecl *ReD = D->getParent();
  const ParameterVector *ParamV = SymT.getParameterVector(ReD);

  AnnotationSet Result;
  RplVector RplV;

  if (!ParamV || ParamV->size()==0) {
    Result.ParamVec = new ParameterVector();
    ParamRplElement Param("P");
    Result.ParamVec->push_back(Param);
    ParamV = Result.ParamVec;
  }
  assert(ParamV && ParamV->size()>0
         && "Internal error: empty region parameter vector.");

  const ParamRplElement &Param = *ParamV->getParamAt(0);
  for (int I = 0; I < ArgNum; ++I) {
    RplV.push_back(Rpl(Param));
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
  QT = FT->getResultType();
  return helperMakeParametricType(D, ArgNum, QT);
}

AnnotationSet ParametricAnnotationScheme::
makeEffectSummary(const FunctionDecl *D) {
  return helperMakeWritesLocalEffectSummary(D);
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
  return helperMakeLocalType(D, ArgNum);
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
  return helperMakeLocalType(D, ArgNum);
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
  Rpl LocalRpl(*SymbolTable::LOCAL_RplElmt);
  Effect WritesLocal(Effect::EK_WritesEffect, &LocalRpl);
  Result.EffSum = new EffectSummary(WritesLocal);

  // Reads Global
  Rpl GlobalRpl(*SymbolTable::GLOBAL_RplElmt);
  Effect ReadsGlobal(Effect::EK_ReadsEffect, &GlobalRpl);
  Result.EffSum->insert(ReadsGlobal);

  return Result;
}

} // end namespace asap
} // end namespace clang
