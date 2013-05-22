//===--- AnnotationSchemes.h - Safe Parallelism checker -----*- C++ -*-----===//
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

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_ANNOTATION_SCHEME_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_ANNOTATION_SCHEME_H

#include "ASaPFwdDecl.h"
#include "ASaPInheritanceMap.h"

namespace clang {
namespace asap {

struct AnnotationSet {
  ASaPType *T;
  //RplVector *RplVec;
  RegionNameSet *RegNameSet;
  ParameterVector *ParamVec;
  EffectSummary *EffSum;
};

// Abstract Base class
class AnnotationScheme {
protected:
  SymbolTable &SymT;
public:
  // Constructor
  AnnotationScheme(SymbolTable &SymT) : SymT(SymT) {}
  // Destructor
  virtual ~AnnotationScheme() {}

  // API Methods
  virtual AnnotationSet makeGlobalType(const VarDecl *D, long ArgNum) = 0;
  virtual AnnotationSet makeStackType(const VarDecl *D, long ArgNum) = 0;

  virtual AnnotationSet makeClassParams(const RecordDecl *D) = 0;
  virtual AnnotationSet makeFieldType(const FieldDecl *D, long ArgNum) = 0;
  virtual AnnotationSet makeParamType(const ParmVarDecl *D, long ArgNum) = 0;
  virtual AnnotationSet makeEffectSummary(const FunctionDecl *D) = 0;

};

class DefaultAnnotationScheme : public AnnotationScheme {
public:
  // Constructor
  DefaultAnnotationScheme(SymbolTable &SymT) :AnnotationScheme(SymT) {}
  // Destructor
  virtual ~DefaultAnnotationScheme() {}

  // Methods
  virtual AnnotationSet makeGlobalType(const VarDecl *D, long ArgNum);
  virtual AnnotationSet makeStackType(const VarDecl *D, long ArgNum);

  virtual AnnotationSet makeClassParams(const RecordDecl *D);
  virtual AnnotationSet makeFieldType(const FieldDecl *D, long ArgNum);
  virtual AnnotationSet makeParamType(const ParmVarDecl *D, long ArgNum);
  virtual AnnotationSet makeEffectSummary(const FunctionDecl *D);

}; // end class DefaultAnnotationScheme

} // end namespace asap
} // end namespace clang

#endif
