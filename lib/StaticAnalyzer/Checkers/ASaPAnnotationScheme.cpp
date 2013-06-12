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


///////////////////////////////////////////////////////////////////////////////
AnnotationSet DefaultAnnotationScheme::
makeClassParams(const RecordDecl *D) {
  AnnotationSet Result;
  ParamRplElement Param("P");
  Result.ParamVec = new ParameterVector(Param);
  return Result;
}

AnnotationSet DefaultAnnotationScheme::
makeGlobalType(const VarDecl *D, long ArgNum) {
  return helperMakeGlobalType(D, ArgNum);
}

AnnotationSet DefaultAnnotationScheme::
makeStackType(const VarDecl *D, long ArgNum) {
  return helperMakeLocalType(D, ArgNum);
}

AnnotationSet DefaultAnnotationScheme::
makeFieldType(const FieldDecl *D, long ArgNum) {
  const RecordDecl *ReD = D->getParent();
  const ParameterVector *ParamV = SymT.getParameterVector(ReD);

  AnnotationSet Result;
  RplVector RplV;
  assert(ParamV && ParamV->size()>0
         && "Internal error: empty region parameter vector.");

  const ParamRplElement &Param = *ParamV->getParamAt(0);
  for (int I = 0; I < ArgNum; ++I) {
    RplV.push_back(Rpl(Param));
  }
  Result.T = new ASaPType(D->getType(), SymT.getInheritanceMap(D), &RplV);
  return Result;
}

AnnotationSet DefaultAnnotationScheme::
makeParamType(const ParmVarDecl *D, long ArgNum) {
  AnnotationSet Result;
  // 1st Arg = Local, then create a new parameter for each subsequent one
  RplVector RplV;
  RplV.push_back(Rpl(*SymbolTable::LOCAL_RplElmt));
  Result.ParamVec = new ParameterVector();
  for (int I = 1; I < ArgNum; ++I) {
    std::stringstream ss;
    ss << D->getNameAsString() << I;
    std::string ParamName = ss.str();
    ParamRplElement Param(ParamName);
    Result.ParamVec->push_back(Param);
    RplV.push_back(Rpl(*SymbolTable::LOCAL_RplElmt));
  }
  Result.T = new ASaPType(D->getType(), SymT.getInheritanceMap(D), &RplV);
  return Result;
}

AnnotationSet DefaultAnnotationScheme::
makeReturnType(const FunctionDecl *D, long ArgNum) {
  assert(false && "Implement Me!");
  return helperMakeLocalType(D, ArgNum);
}

AnnotationSet DefaultAnnotationScheme::
makeEffectSummary(const FunctionDecl *D) {
  AnnotationSet Result;
  assert(false && "Implement Me!");
  return Result;
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
