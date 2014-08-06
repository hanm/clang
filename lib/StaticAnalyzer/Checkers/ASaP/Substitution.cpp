//=== Substitution.cpp - Safe Parallelism checker -----*- C++ -*--------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This file defines the Substitution and SubstitutionVector classes used
// by the Safe Parallelism checker, which tries to prove the safety of
// parallelism given region and effect annotations.
//
//===----------------------------------------------------------------===//

#include "ASaPSymbolTable.h"
#include "ASaPType.h"
#include "Effect.h"
#include "Rpl.h"
#include "Substitution.h"

using namespace clang;
using namespace clang::asap;

namespace clang {
namespace asap {

Substitution::Substitution(const RplElement *FromEl, const Rpl *ToRpl) :
  FromEl(FromEl) {
  assert(FromEl);
  if (ToRpl)
    this->ToRpl = ToRpl->clone();
  else
    this->ToRpl = 0;
}

Substitution::Substitution(const Substitution &Sub) :
  FromEl(Sub.FromEl) {
  assert(FromEl);
  if (Sub.ToRpl)
    this->ToRpl = Sub.ToRpl->clone();
  else
    this->ToRpl = 0;
}

Substitution::~Substitution() {
  delete ToRpl;
}

void Substitution::set(const RplElement *FromEl, const Rpl *ToRpl) {
  this->FromEl = FromEl;
  if (ToRpl)
    this->ToRpl = ToRpl->clone();
  else
    this->ToRpl = 0;
}
term_t Substitution::getPLTerm() const {
  term_t Result = PL_new_term_ref();
  functor_t SubFunctor =
    PL_new_functor(PL_new_atom(PL_ParamSub.c_str()), 2);
  assert(FromEl && "Subtitution missing left hand side");
  assert(ToRpl && "Substitution missing right hand side");
  int Res = PL_cons_functor(Result, SubFunctor,
                            FromEl->getPLTerm(),
                            ToRpl->getRplElementsPLTerm());
  assert(Res && "Failed to create prolog term_t for Substitution");
  return Result;
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

//////////////////////////////////////////////////////////////////////////
void SubstitutionSet::
buildSubstitutionSet(const ParameterVector *ParV, const RplVector *RplVec) {
  if (!ParV && !RplVec)
    return; // empty SubstitutionVector

  assert(ParV && RplVec);
  assert(ParV->size() <= RplVec->size());
  for (size_t I = 0; I < ParV->size(); ++I) {
    const Rpl *ToRpl = RplVec->getRplAt(I);
    const ParamRplElement *FromEl = ParV->getParamAt(I);
    if (*ToRpl != *FromEl) {
      Substitution Sub(FromEl, ToRpl);
      insert(Sub);
    }
  }
}

void SubstitutionSet::print(llvm::raw_ostream &OS) const {
  OS << "subst_set{";
  for(SetT::const_iterator I = begin(), E = end();
      I != E; ++I) {
    assert(*I);
    (*I)->print(OS);
  }
  OS << "}";
}

std::string SubstitutionSet::toString() const {
  std::string SBuf;
  llvm::raw_string_ostream OS(SBuf);
  print(OS);
  return std::string(OS.str());
}

void SubstitutionSet::applyTo(ConcreteRpl &R) const {
  for(SetT::const_iterator I = begin(), E = end();
      I != E; ++I) {
      Trivalent Result = R.substitute(*I);
      if (Result == RK_TRUE)
        break; // Each Rpl has at most one parameter, so stop after
               // the first successful application
  }
}

term_t SubstitutionSet::getPLTerm() const {
  // TODO
}

//////////////////////////////////////////////////////////////////////////
void SubstitutionVector::push_back_vec(const SubstitutionVector *SubV) {
  if (SubV) {
    for(VectorT::const_iterator I = SubV->begin(), E = SubV->end();
        I != E; ++I) {
      this->push_back(*I);
    }
  }
}

} // end namespace clang
} // end namespace asap

