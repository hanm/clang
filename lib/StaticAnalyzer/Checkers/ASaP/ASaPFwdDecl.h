//=== ASaPFwdDecl.h - Safe Parallelism checker -----*- C++ -*--------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This file defines commonly needed forward declarations used by
// the Safe Parallelism checker, which tries to prove the safety of
// parallelism given region and effect annotations.
//
//===----------------------------------------------------------------===//

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_FWD_DECL_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_FWD_DECL_H

namespace llvm {

class StringRef;
class raw_ostream;
class raw_fd_ostream;

}
using llvm::StringRef;
using llvm::raw_ostream;
using llvm::raw_fd_ostream;

namespace clang {

namespace ento {
  class BugReporter;
  class PathDiagnosticLocation;
  class AnalysisManager;
  class CheckerBase;
}
using ento::PathDiagnosticLocation;
using ento::BugReporter;
using ento::AnalysisManager;
using ento::CheckerBase;

class AnalysisDeclContext;
class ASTContext;
class Attr;
class CallExpr;
class CXXConstructorDecl;
class CXXRecordDecl;
class Decl;
class DeclaratorDecl;
class Expr;
class ExprIterator;
class FunctionDecl;
class ParmVarDecl;
class Stmt;
class ValueDecl;
class VarDecl;

namespace asap {

class AnnotationScheme;
class ASaPType;
class ConcreteEffectSummary;
class ConcreteRpl;
class Effect;
class EffectInclusionConstraint;
class EffectVector;
class EffectSummary;
class NamedRplElement;
class ParameterSet;
class ParameterVector;
class ParamRplElement;
class RegionNameSet;
class ResultTriplet;
class Rpl;
class RplElement;
class RplVector;
class SpecialRplElement;
class SpecificNIChecker;
class StarRplElement;
class Substitution;
class SubstitutionSet;
class SubstitutionVector;
class SymbolTable;
class SymbolTableEntry;
class VarEffectSummary;
class VarRpl;
struct VisitorBundle;

} // End namespace asap.
} // End namespace clang.

#endif
