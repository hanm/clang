//=== Rpl.cpp - Safe Parallelism checker -----*- C++ -*--------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This file defines the Rpl and RplVector classes used by the Safe
// Parallelism checker, which tries to prove the safety of parallelism
// given region and effect annotations.
//
//===----------------------------------------------------------------===//

#include "llvm/Support/Casting.h"

#include "Rpl.h"
#include "ASaPSymbolTable.h"
#include "ASaPUtil.h"
#include "Substitution.h"

namespace clang {
namespace asap {
/// Static
std::pair<StringRef, StringRef> Rpl::splitRpl(StringRef &String) {
  size_t Idx = 0;
  do {
    Idx = String.find(RPL_SPLIT_CHARACTER, Idx);
    OSv2 << "DEBUG:: Rpl::splitRpl: Idx = " << Idx << ", size = " << String.size() << "\n";
    if (Idx == StringRef::npos)
      break;

  } while (Idx < String.size() - 2
    && String[Idx+1] == RPL_SPLIT_CHARACTER && Idx++ && Idx++);

  if (Idx == StringRef::npos) // npos (== ~0) is the value for "not found"
    return std::pair<StringRef, StringRef>(String, "");
  else
    return std::pair<StringRef, StringRef>(String.slice(0,Idx),
                                           String.slice(Idx+1, String.size()));
}



/// Static
const StringRef Rpl::RPL_LIST_SEPARATOR = ",";
const StringRef Rpl::RPL_NAME_SPEC = "::";


RplDomain::RplDomain(RegionNameVector *RV, const ParameterVector *PV, RplDomain *P){
  Regions = RV;
  Params = PV;
  Parent = P;
}

void RplDomain::addRegion(NamedRplElement *R){
  Regions->push_back(R);
}


void RplDomain::print (llvm::raw_ostream& OS) {
  OS << "----------------------------\n";
  OS << "Params: \n";
  if(Params)
    Params->print(OS);
  OS << "\n";
  OS << "Regions: \n";
  if(Regions)
    Regions->print(OS);
  OS << "\n";
  OS << "**Parent**\n";
  if (Parent)
    Parent->print(OS);
  OS << "----------------------------\n";
}

bool Rpl::isValidRegionName(const llvm::StringRef& Str) {
  // false if it is one of the Special Rpl Elements
  // => it is not allowed to redeclare them
  if (SymbolTable::isSpecialRplElement(Str))
    return false;

  // must start with [_a-zA-Z]
  const char c = Str.front();
  if (c != '_' &&
    !( c >= 'a' && c <= 'z') &&
    !( c >= 'A' && c <= 'Z'))
    return false;
  // all remaining characters must be in [_a-zA-Z0-9]
  for (size_t i=0; i < Str.size(); i++) {
    const char c = Str[i];
    if (c != '_' &&
      !( c >= 'a' && c <= 'z') &&
      !( c >= 'A' && c <= 'Z') &&
      !( c >= '0' && c <= '9'))
      return false;
  }
  return true;
}

bool Rpl::RplRef::isUnder(RplRef& RHS) {
  OSv2  << "DEBUG:: ~~~~~~~~isUnder[RplRef]("
        << this->toString() << ", " << RHS.toString() << ")\n";
  /// R <= Root
  if (RHS.isEmpty())
    return true;
  if (isEmpty()) /// and RHS is not Empty
    return false;
  /// R <= R' <== R c= R'
  if (isIncludedIn(RHS)) return true;
  /// R:* <= R' <== R <= R'
  if (getLastElement() == SymbolTable::STAR_RplElmt)
    return stripLast().isUnder(RHS);
  /// R:r <= R' <==  R <= R'
  /// R:[i] <= R' <==  R <= R'
  return stripLast().isUnder(RHS);
  // TODO z-regions
}

bool Rpl::RplRef::isIncludedIn(RplRef& RHS) {
  OSv2  << "DEBUG:: ~~~~~~~~isIncludedIn[RplRef]("
        << this->toString() << ", " << RHS.toString() << ")\n";
  if (RHS.isEmpty()) {
    /// Root c= Root
    if (isEmpty()) return true;
    /// RPL c=? Root and RPL!=Root ==> not included
    else /*!isEmpty()*/ return false;
  } else { /// RHS is not empty
    /// R c= R':* <==  R <= R'
    if (RHS.getLastElement() == SymbolTable::STAR_RplElmt) {
      OSv2 <<"DEBUG:: isIncludedIn[RplRef] last elmt of RHS is '*'\n";
      return isUnder(RHS.stripLast());
    }
    ///   R:r c= R':r    <==  R <= R'
    /// R:[i] c= R':[i]  <==  R <= R'
    if (!isEmpty()) {
      if ( *getLastElement() == *RHS.getLastElement() )
        return stripLast().isIncludedIn(RHS.stripLast());
    }
    return false;
  }
}

bool Rpl::RplRef::isDisjointLeft(RplRef &That) {
  if (this->isEmpty()) {
    if (That.isEmpty())
      return false;
    else
      return true;
  } else { // 'this' is not empty
    if (That.isEmpty())
      return true;
    else { // both 'this' and 'That' non empty
      if (this->getFirstElement() == That.getFirstElement())
          return this->stripFirst().isDisjointLeft(That.stripFirst());
      else
        if (this->getFirstElement() == SymbolTable::STAR_RplElmt
            || That.getFirstElement() == SymbolTable::STAR_RplElmt)
          return false;
        else
          return true;
    }
  }
}

bool Rpl::RplRef::isDisjointRight(RplRef &That) {
  if (this->isEmpty()) {
    if (That.isEmpty())
      return false;
    else
      return true;
  } else { // 'this' is not empty
    if (That.isEmpty())
      return true;
    else { // both 'this' and 'That' non empty
      if (this->getLastElement() == That.getLastElement())
          return this->stripLast().isDisjointRight(That.stripLast());
      else
        if (this->getLastElement() == SymbolTable::STAR_RplElmt
            || That.getLastElement() == SymbolTable::STAR_RplElmt)
          return false;
        else
          return true;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// end Rpl::RplRef; start Rpl

term_t Rpl::getRplElementsPLTerm() const {
  term_t RplElList = PL_new_term_ref();
  PL_put_nil(RplElList);
  int Res = 0;
  for (RplElementVectorTy::const_reverse_iterator I = RplElements.rbegin(),
                                                  E = RplElements.rend();
       I != E; ++I) {
    term_t RplEl = (*I)->getPLTerm();
    Res = PL_cons_list(RplElList, RplEl, RplElList);
    assert(Res && "Failed to add RPL element to Prolog list term");
  }
  return RplElList;
}

term_t Rpl::getPLTerm() const {
  term_t Result = PL_new_term_ref();
  functor_t RplFunctor = PL_new_functor(PL_new_atom("rpl"), 2);
  // 1. build RPL element list
  term_t RplElList = getRplElementsPLTerm();
  // 2. build (empty) substitution list
  term_t SubList = PL_new_term_ref();
  PL_put_nil(SubList);
  // 3. combine the two lists into a functor term
  int Res = PL_cons_functor(Result, RplFunctor, RplElList, SubList);
  assert(Res && "Failed to create prolog term_t for RPL");
  return Result;
}

bool Rpl::isDisjoint(const Rpl &That) const {
  RplRef LHS1(*this);
  RplRef RHS1(That);
  RplRef LHS2(*this);
  RplRef RHS2(That);

  return LHS1.isDisjointLeft(RHS1) || LHS2.isDisjointRight(RHS2);
}

void Rpl::print(raw_ostream &OS) const {
  RplElementVectorTy::const_iterator I = RplElements.begin();
  RplElementVectorTy::const_iterator E = RplElements.end();
  for (; I < E-1; I++) {
    OS << (*I)->getName() << Rpl::RPL_SPLIT_CHARACTER;
  }
  // print last element
  if (I < E)
    OS << (*I)->getName();
}

std::string Rpl::toString() const {
  std::string SBuf;
  llvm::raw_string_ostream OS(SBuf);
  print(OS);
  return std::string(OS.str());
}

bool Rpl::isUnder(const Rpl& That) const {
  const CaptureRplElement *cap = dyn_cast<CaptureRplElement>(RplElements.front());
  if (cap) {
    cap->upperBound().isUnder(That);
  }

  RplRef* LHS = new RplRef(*this);
  RplRef* RHS = new RplRef(That);
  bool Result = LHS->isIncludedIn(*RHS);
  delete LHS; delete RHS;
  return Result;
}

bool Rpl::isIncludedIn(const Rpl& That) const {
  const CaptureRplElement *cap = dyn_cast<CaptureRplElement>(RplElements.front());
  if (cap) {
    cap->upperBound().isIncludedIn(That);
  }

  RplRef* LHS = new RplRef(*this);
  RplRef* RHS = new RplRef(That);
  bool Result = LHS->isIncludedIn(*RHS);
  delete LHS; delete RHS;
  OSv2 << "DEBUG:: ~~~~~ isIncludedIn[RPL](" << this->toString()
    << "[" << this << "], " << That.toString() << "[" << &That
    << "])=" << (Result ? "true" : "false") << "\n";
  return Result;
}

void Rpl::substitute(const Substitution *S) {
  if (!S || !S->getFrom() || !S->getTo())
    return; // Nothing to do.
  const RplElement &FromEl = *S->getFrom();
  const Rpl &ToRpl = *S->getTo();
  os << "DEBUG:: before substitution(" << FromEl.getName() << "<-";
  ToRpl.print(os);
  os <<"): ";
  assert(RplElements.size()>0);
  print(os);
  os << "\n";
  /// A parameter is only allowed at the head of an Rpl
  RplElementVectorTy::iterator I = RplElements.begin();
  if (*(*I) == FromEl) {
    OSv2 << "DEBUG:: found '" << FromEl.getName()
      << "' replaced with '" ;
    ToRpl.print(OSv2);
    I = RplElements.erase(I);
    I = RplElements.insert(I, ToRpl.RplElements.begin(),
      ToRpl.RplElements.end());
    OSv2 << "' == '";
    print(OSv2);
    OSv2 << "'\n";
  }
  os << "DEBUG:: after substitution(" << FromEl.getName() << "<-";
  ToRpl.print(os);
  os << "): ";
  print(os);
  os << "\n";
}

inline void Rpl::appendRplTail(Rpl* That) {
  if (!That)
    return;
  if (That->length()>1)
    RplElements.append(That->length()-1, (*(That->RplElements.begin() + 1)));
}

Rpl *Rpl::upperBound() {
  if (isEmpty() || !isa<CaptureRplElement>(RplElements.front()))
    return this;
  // else
  Rpl* upperBound = & dyn_cast<CaptureRplElement>(RplElements.front())->upperBound();
  upperBound->appendRplTail(this);
  return upperBound;
}

void Rpl::join(Rpl* That) {
  if (!That)
    return;
  Rpl Result;
  // join from the left
  RplElementVectorTy::const_iterator
    ThisI = this->RplElements.begin(),
    ThatI = That->RplElements.begin(),
    ThisE = this->RplElements.end(),
    ThatE = That->RplElements.end();
  for ( ; ThisI != ThisE && ThatI != ThatE && (*ThisI == *ThatI);
    ++ThisI, ++ThatI) {
      Result.appendElement(*ThisI);
  }
  if (ThisI != ThisE) {
    // put a star in the middle and join from the right
    assert(ThatI != ThatE);
    Result.appendElement(SymbolTable::STAR_RplElmt);
    Result.FullySpecified = false;
    // count how many elements from the right we will need to append
    RplElementVectorTy::const_reverse_iterator
      ThisI = this->RplElements.rbegin(),
      ThatI = That->RplElements.rbegin(),
      ThisE = this->RplElements.rend(),
      ThatE = That->RplElements.rend();
    int ElNum = 0;
    for(; ThisI != ThisE && ThatI != ThatE && (*ThisI == *ThatI);
        ++ThisI, ++ThatI, ++ElNum);
    // append ElNum elements from the right
    if (ElNum > 0) {
      Result.RplElements.append(ElNum,
        *(RplElements.begin() + RplElements.size() - ElNum));
    }
  }
  // return
  this->RplElements = Result.RplElements;
  return;
}

Rpl *Rpl::capture() {
  if (this->isFullySpecified()) return this;
  else return new Rpl(*new CaptureRplElement(*this));
}

} // End namespace asap.
} // End namespace clang.

