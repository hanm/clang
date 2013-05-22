//===--- ASaPInheritanceMap.h - Safe Parallelism checker ----*- C++ -*-----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the typedef-ed type InheritanceMapT used by the Safe
// Parallelism checker, which tries to prove the safety of parallelism
// given region and effect annotations.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_INHERITANCE_MAP_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_INHERITANCE_MAP_H

#include "llvm/ADT/DenseMap.h"

namespace clang {
namespace asap {

// Instead of Map(Decl -> SubVec) we use the SymbolTableEntry that
// corresponds to the Decl so we won't have to store a pointer to
// the SymbolTable from the SymbolTableEntries, in order to resolve
// the Decls
typedef std::pair<SymbolTableEntry *, const SubstitutionVector *> IMapPair;
typedef llvm::DenseMap<const RecordDecl *, IMapPair> InheritanceMapT;

} // end namespace asap
} // end namespace clang

#endif
