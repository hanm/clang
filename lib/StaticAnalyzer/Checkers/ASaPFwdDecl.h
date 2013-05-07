//=== ASaPFwdDecl.h - Safe Parallelism checker -----*- C++ -*--------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This files defines commonly needed forward declarations used by
// the Safe Parallelism checker, which tries to prove the safety of
// parallelism given region and effect annotations.
//
//===----------------------------------------------------------------===//

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_FWD_DECL_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_FWD_DECL_H

namespace llvm {

class StringRef;
class raw_ostream;

}
using llvm::StringRef;
using llvm::raw_ostream;

namespace clang {

namespace ento {
  class BugReporter;
  class PathDiagnosticLocation;
  class AnalysisManager;
}
using ento::PathDiagnosticLocation;
using ento::BugReporter;
using ento::AnalysisManager;

class AnalysisDeclContext;
class ASTContext;
class Attr;
class CXXConstructorDecl;
class Decl;
class Expr;
class FunctionDecl;
class Stmt;

namespace asap {

class ASaPType;
class Effect;
class EffectVector;
class EffectSummary;
class NamedRplElement;
class ParameterVector;
class ParamRplElement;
class RegionNameSet;
class ResultTriplet;
class Rpl;
class RplElement;
class RplVector;
class SpecialRplElement;
class StarRplElement;
class Substitution;
class SubstitutionVector;
class SymbolTable;
struct VisitorBundle;

} // End namespace asap.
} // End namespace clang.

#endif
