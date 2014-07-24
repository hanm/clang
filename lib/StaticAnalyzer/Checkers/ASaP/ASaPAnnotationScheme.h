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
  RegionNameSet *RegNameSet;
  ParameterVector *ParamVec;
  EffectSummary *EffSum;

  AnnotationSet() : T(0), RegNameSet(0), ParamVec(0), EffSum(0) {}
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
  virtual AnnotationSet makeClassParams(const RecordDecl *D) = 0;

  virtual AnnotationSet makeGlobalType(const VarDecl *D, long ArgNum) = 0;
  virtual AnnotationSet makeStackType(const VarDecl *D, long ArgNum) = 0;
  virtual AnnotationSet makeFieldType(const FieldDecl *D, long ArgNum) = 0;
  virtual AnnotationSet makeParamType(const ParmVarDecl *D, long ArgNum) = 0;
  virtual AnnotationSet makeReturnType(const FunctionDecl *D, long ArgNum) = 0;

  virtual AnnotationSet makeEffectSummary(const FunctionDecl *D) = 0;
  virtual RplVector *makeBaseTypeArgs(const RecordDecl *Derived, long ArgNum) = 0;

protected:
  AnnotationSet helperMakeClassParams(const RecordDecl *D);
  AnnotationSet helperMakeGlobalType(const ValueDecl *D, long ArgNum);
  AnnotationSet helperMakeLocalType(const ValueDecl *D, long ArgNum);
  AnnotationSet helperMakeParametricType(const DeclaratorDecl *D, long ArgNum, QualType QT);
  AnnotationSet helperMakeWritesLocalEffectSummary(const FunctionDecl *D);
  AnnotationSet helperMakeVarType(const ValueDecl *D, long ArgNum);
  AnnotationSet helperMakeVarEffectSummary(const FunctionDecl *D);
  RplVector *helperMakeBaseTypeArgs(const RecordDecl *Derived, long ArgNum);

};

class ParametricAnnotationScheme : public AnnotationScheme {
public:
  // Constructor
  ParametricAnnotationScheme(SymbolTable &SymT) : AnnotationScheme(SymT) {}
  // Destructor
  virtual ~ParametricAnnotationScheme() {}

  // Methods
  virtual AnnotationSet makeClassParams(const RecordDecl *D);

  virtual AnnotationSet makeGlobalType(const VarDecl *D, long ArgNum);
  virtual AnnotationSet makeStackType(const VarDecl *D, long ArgNum);
  virtual AnnotationSet makeFieldType(const FieldDecl *D, long ArgNum);
  virtual AnnotationSet makeParamType(const ParmVarDecl *D, long ArgNum);
  virtual AnnotationSet makeReturnType(const FunctionDecl *D, long ArgNum);

  virtual AnnotationSet makeEffectSummary(const FunctionDecl *D);
  virtual RplVector *makeBaseTypeArgs(const RecordDecl *Derived, long ArgNum);
}; // end class ParametricAnnotationScheme

class SimpleAnnotationScheme : public AnnotationScheme {
public:
  // Constructor
  SimpleAnnotationScheme(SymbolTable &SymT) :AnnotationScheme(SymT) {}
  // Destructor
  virtual ~SimpleAnnotationScheme() {}

  // Methods
  virtual AnnotationSet makeClassParams(const RecordDecl *D);

  virtual AnnotationSet makeGlobalType(const VarDecl *D, long ArgNum);
  virtual AnnotationSet makeStackType(const VarDecl *D, long ArgNum);
  virtual AnnotationSet makeFieldType(const FieldDecl *D, long ArgNum);
  virtual AnnotationSet makeParamType(const ParmVarDecl *D, long ArgNum);
  virtual AnnotationSet makeReturnType(const FunctionDecl *D, long ArgNum);

  virtual AnnotationSet makeEffectSummary(const FunctionDecl *D);
  virtual RplVector *makeBaseTypeArgs(const RecordDecl *Derived, long ArgNum);
}; // end class SimpleAnnotationScheme

class CheckGlobalsAnnotationScheme : public AnnotationScheme {
public:
  // Constructor
  CheckGlobalsAnnotationScheme(SymbolTable &SymT) :AnnotationScheme(SymT) {}
  // Destructor
  virtual ~CheckGlobalsAnnotationScheme() {}

  // Methods
  virtual AnnotationSet makeClassParams(const RecordDecl *D);

  virtual AnnotationSet makeGlobalType(const VarDecl *D, long ArgNum);
  virtual AnnotationSet makeStackType(const VarDecl *D, long ArgNum);
  virtual AnnotationSet makeFieldType(const FieldDecl *D, long ArgNum);
  virtual AnnotationSet makeParamType(const ParmVarDecl *D, long ArgNum);
  virtual AnnotationSet makeReturnType(const FunctionDecl *D, long ArgNum);

  virtual AnnotationSet makeEffectSummary(const FunctionDecl *D);
  virtual RplVector *makeBaseTypeArgs(const RecordDecl *Derived, long ArgNum);
}; // end class CheckGlobalsAnnotationScheme

// Insert effect summary variables wherever they are missing.
class SimpleEffectInferenceAnnotationScheme : public SimpleAnnotationScheme {
public:
  // Constructor
  SimpleEffectInferenceAnnotationScheme(SymbolTable &SymT)
                                       : SimpleAnnotationScheme(SymT) {}
  // Destructor
  virtual ~SimpleEffectInferenceAnnotationScheme() {}

  // Methods (Overridden)
  virtual AnnotationSet makeEffectSummary(const FunctionDecl *D);
}; // end class EffectInferenceAnnotationScheme

// Insert effect summary variables wherever they are missing.
class ParametricEffectInferenceAnnotationScheme
                                  : public ParametricAnnotationScheme {
public:
  // Constructor
  ParametricEffectInferenceAnnotationScheme(SymbolTable &SymT)
                                 : ParametricAnnotationScheme(SymT) {}
  // Destructor
  virtual ~ParametricEffectInferenceAnnotationScheme() {}

  // Methods (Overridden)
  virtual AnnotationSet makeEffectSummary(const FunctionDecl *D);
}; // end class EffectInferenceAnnotationScheme


// Insert effect summary variables and rpl variables wherever they are missing.
class InferenceAnnotationScheme : public ParametricAnnotationScheme {
public:
  // Constructor
  InferenceAnnotationScheme(SymbolTable &SymT)
                           : ParametricAnnotationScheme(SymT) {}
  // Destructor
  virtual ~InferenceAnnotationScheme() {}

  // Methods (Inherited)
  //virtual AnnotationSet makeClassParams(const RecordDecl *D);

  virtual AnnotationSet makeGlobalType(const VarDecl *D, long ArgNum);
  virtual AnnotationSet makeStackType(const VarDecl *D, long ArgNum);
  virtual AnnotationSet makeFieldType(const FieldDecl *D, long ArgNum);
  virtual AnnotationSet makeParamType(const ParmVarDecl *D, long ArgNum);
  virtual AnnotationSet makeReturnType(const FunctionDecl *D, long ArgNum);

  // Methods (Overridden)
  virtual AnnotationSet makeEffectSummary(const FunctionDecl *D);
}; // end class CheckGlobalsAnnotationScheme

} // end namespace asap
} // end namespace clang

#endif
