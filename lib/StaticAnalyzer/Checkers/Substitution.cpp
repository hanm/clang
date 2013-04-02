//=== Substitution.cpp - Safe Parallelism checker -----*- C++ -*--------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This files defines the Substitution and SubstitutionVector classes used 
// by the Safe Parallelism checker, which tries to prove the safety of 
// parallelism given region and effect annotations.
//
//===----------------------------------------------------------------===//

#include "Substitution.h"
#include "Rpl.h"
#include "ASaPSymbolTable.h"
#include "Effect.h"

using namespace clang;
using namespace clang::asap;

namespace clang {
namespace asap {

Substitution::Substitution(const RplElement *FromEl, const Rpl *ToRpl) :
  FromEl(FromEl) {
  assert(FromEl);
  if (ToRpl)
    this->ToRpl = new Rpl(*ToRpl);
  else
    this->ToRpl = 0;
}

Substitution::Substitution(const Substitution &Sub) :
  FromEl(Sub.FromEl) {
  assert(FromEl);
  if (Sub.ToRpl)
    this->ToRpl = new Rpl(*Sub.ToRpl);
  else
    this->ToRpl = 0;
}

Substitution::~Substitution() {
  delete ToRpl;
}

void Substitution::set(const RplElement *FromEl, const Rpl *ToRpl) {
  this->FromEl = FromEl;
  if (ToRpl)
    this->ToRpl = new Rpl(*ToRpl);
  else
    this->ToRpl = 0;
}

void Substitution::applyTo(Rpl *R) const {
  if (R && FromEl && ToRpl) {
    R->substitute(*FromEl, *ToRpl);
  }
}

void Substitution::applyTo(Effect *E) const {
  if (E && FromEl && ToRpl) {
    E->substitute(*this);
  }
}

void Substitution::print(llvm::raw_ostream &OS) const {
  OS << "[";
  if (FromEl) {
    OS << FromEl->getName();
  } else {
    OS << "<MISSING>";
  }
  OS << "<-";
  if (ToRpl) {
    OS << ToRpl->toString();
  } else {
    OS << "<MISSING";
  }
  OS << "]";
}

std::string Substitution::toString() const {
  std::string SBuf;
  llvm::raw_string_ostream OS(SBuf);
  print(OS);
  return std::string(OS.str());
}

} // end namespace clang
} // end namespace asap

