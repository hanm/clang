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

#include "ASaPType.h"
#include "Rpl.h"

#include "ASaPAnnotationScheme.h"
#include "ASaPSymbolTable.h"

namespace clang {
namespace asap {

AnnotationSet DefaultAnnotationScheme::
makeGlobalType(const VarDecl *D, long ArgNum) {
  AnnotationSet Result;

  RplVector RplV;
  for (int I = 0; I < ArgNum; ++I) {
    RplV.push_back(Rpl(*SymbolTable::GLOBAL_RplElmt));
  }
  Result.T = new ASaPType(D->getType(), SymT.getInheritanceMap(D), &RplV);
  return Result;
}

AnnotationSet DefaultAnnotationScheme::
makeStackType(const VarDecl *D, long ArgNum) {
  AnnotationSet Result;

  RplVector RplV;
  for (int I = 0; I < ArgNum; ++I) {
    RplV.push_back(Rpl(*SymbolTable::LOCAL_RplElmt));
  }
  Result.T = new ASaPType(D->getType(), SymT.getInheritanceMap(D), &RplV);
  return Result;
}

AnnotationSet DefaultAnnotationScheme::
makeClassParams(const RecordDecl *D) {
  AnnotationSet Result;
  ParamRplElement Param("P");
  Result.ParamVec = new ParameterVector(Param);
  return Result;
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
makeEffectSummary(const FunctionDecl *D) {
  AnnotationSet Result;
  assert(false && "Implement Me!");
  return Result;
}

} // end namespace asap
} // end namespace clang
