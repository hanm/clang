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

///////////////////////////////////////////////////////////////////////////////
//// Rpl

/// Static
const StringRef Rpl::RPL_LIST_SEPARATOR = ",";
const StringRef Rpl::RPL_NAME_SPEC = "::";

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

/// Member functions
inline void Rpl::addSubstitution(const Substitution &S) {
  SubstitutionSet SubS;
  SubS.insert(S);
  SubV.push_back(SubS);
}

inline void Rpl::addSubstitution(const SubstitutionSet &SubS) {
  SubV.push_back(SubS);
}

term_t Rpl::getSubVPLTerm() const {
    return SubV.getPLTerm();
}

std::string Rpl::toString() const {
  std::string SBuf;
  llvm::raw_string_ostream OS(SBuf);
  print(OS);
  return std::string(OS.str());
}

void Rpl::print(llvm::raw_ostream &OS) const {
  if (SubV.size() > 0) {
    SubV.print(OS);
  }
}

Rpl::Rpl(const Rpl &That)
        : Kind(That.Kind),
          FullySpecified(That.FullySpecified) {
  SubV = That.SubV;
}

///////////////////////////////////////////////////////////////////////////////
//// RplDomain

RplDomain::RplDomain(StringRef Name, const RegionNameVector *RV,
                     const ParameterVector *PV, RplDomain *P)
                    : Name(Name), Params(PV), Parent(P), Used(false) {
  if (RV) {
    Regions = new RegionNameVector(*RV);
  } else {
    Regions = new RegionNameVector();
  }
}

RplDomain::RplDomain(const RplDomain &Dom)
                    : Name(Dom.Name), Params(Dom.Params),
                      Parent(Dom.Parent), Used(Dom.Used) {
  if (Dom.Regions) {
    Regions = new RegionNameVector(*Dom.Regions);
  } else {
    Regions = new RegionNameVector();
  }
}
void RplDomain::addRegion(const NamedRplElement &R) {
  Regions->push_back(R);
}

void RplDomain::markUsed() {
  Used = true;
  if (Parent && !Parent->isUsed())
    Parent->markUsed();
}

void RplDomain::print (llvm::raw_ostream &OS) const {
  OS << "{";
  if (Regions && Regions->size() > 0) {
    OS << "regions[[";
    Regions->print(OS);
    OS << "]], ";
  }
  if (Params && Params->size() > 0) {
    OS << "params[[";
    Params->print(OS);
    OS << "]], ";
  }
  if (Parent) {
    OS << "parent";
    Parent->print(OS);
  }
  OS << "}";
}

//rpl_dom(name, [regions], [params], parent)
term_t RplDomain::getPLTerm() const {
  term_t Result = PL_new_term_ref();
  functor_t DomF = PL_new_functor(PL_new_atom(PL_RplDomain.c_str()),4);
  // 1. Domain Name
  term_t DomNam = PL_new_term_ref();
  PL_put_atom_chars(DomNam, Name.data());
  // 2. Region name List
  term_t RegList;
  if (Regions) {
    RegList = Regions->getPLTerm();
  } else {
    RegList = buildPLEmptyList();
  }
  // 3. Parameter list
  term_t ParamList;
  if (Params) {
    ParamList = Params->getPLTerm();
  } else {
    ParamList = buildPLEmptyList();
  }
  // 4. Parent domain name
  term_t ParentName = PL_new_term_ref();
  if (Parent) {
    PL_put_atom_chars(ParentName, Parent->Name.data());
  } else {
    PL_put_atom_chars(ParentName, PL_NullDomain.c_str());
  }
  int Res = PL_cons_functor(Result, DomF, DomNam, RegList, ParamList, ParentName);
  assert(Res && "Failed to create prolog term_t for RplDomain");
  return Result;
}

void RplDomain::assertzProlog() const {
  assertzTermProlog(getPLTerm(), "Failed to assert 'rpl_domain' to Prolog facts");
}

RplDomain::~RplDomain() {
  delete Regions;
}
///////////////////////////////////////////////////////////////////////////////
//// RplRef

ConcreteRpl::RplRef::RplRef(const ConcreteRpl &R) : rpl(R) {
  firstIdx = 0;
  lastIdx = rpl.RplElements.size()-1;
}
/// Printing (Rpl Ref)
void ConcreteRpl::RplRef::print(llvm::raw_ostream &OS) const {
  int I = firstIdx;
  for (; I < lastIdx; ++I) {
    OS << rpl.RplElements[I]->getName() << RPL_SPLIT_CHARACTER;
  }
  // print last element
  if (I==lastIdx)
    OS << rpl.RplElements[I]->getName();
}

std::string ConcreteRpl::RplRef::toString() const {
  std::string SBuf;
  llvm::raw_string_ostream OS(SBuf);
  print(OS);
  return std::string(OS.str());
}

bool ConcreteRpl::RplRef::isUnder(RplRef &RHS) {
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

bool ConcreteRpl::RplRef::isIncludedIn(RplRef &RHS) {
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

bool ConcreteRpl::RplRef::isDisjointLeft(RplRef &That) {
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

bool ConcreteRpl::RplRef::isDisjointRight(RplRef &That) {
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
// end ConcreteRpl::RplRef; start ConcreteRpl

term_t ConcreteRpl::getRplElementsPLTerm() const {
  term_t RplElList = buildPLEmptyList();
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

term_t ConcreteRpl::getPLTerm() const {
  term_t Result = PL_new_term_ref();
  functor_t RplFunctor = PL_new_functor(PL_new_atom(PL_ConcreteRpl.c_str()), 2);
  // 1. build RPL element list
  term_t RplElList = getRplElementsPLTerm();
  // 2. build (empty) substitution list
  term_t SubList = getSubVPLTerm();
  // 3. combine the two lists into a functor term
  int Res = PL_cons_functor(Result, RplFunctor, RplElList, SubList);
  assert(Res && "Failed to create prolog term_t for RPL");
  return Result;
}

inline bool ConcreteRpl::isPrivate() const {
  return (this->RplElements.size()==1 &&
      this->RplElements[0] == SymbolTable::LOCAL_RplElmt);
}

void ConcreteRpl::print(raw_ostream &OS) const {
  RplElementVectorTy::const_iterator I = RplElements.begin();
  RplElementVectorTy::const_iterator E = RplElements.end();
  if (I != E) {
    OS << (*I)->getName();
    ++I;
  }
  for (; I != E; ++I) {
    OS << Rpl::RPL_SPLIT_CHARACTER
       << (*I)->getName();
  }
  Rpl::print(OS);
}

Trivalent ConcreteRpl::isUnder(const Rpl &That) const {
  /*const CaptureRplElement *Cap = dyn_cast<CaptureRplElement>(RplElements.front());
  if (Cap) {
    return Cap->upperBound().isUnder(That);
  }*/
  // else
  if (isa<VarRpl>(&That)) {
    return RK_DUNNO;
  }
  // else
  const ConcreteRpl *ConcThat = dyn_cast<ConcreteRpl>(&That);
  assert(ConcThat && "Unexpected kind of RPL");
  RplRef *LHS = new RplRef(*this);
  RplRef *RHS = new RplRef(*ConcThat);
  Trivalent Result = boolToTrivalent(LHS->isUnder(*RHS));
  delete LHS; delete RHS;
  return Result;
}

Trivalent ConcreteRpl::isIncludedIn(const Rpl &That) const {
  /*const CaptureRplElement *cap = dyn_cast<CaptureRplElement>(RplElements.front());
  if (cap) {
    return cap->upperBound().isIncludedIn(That);
  }*/
  if (isa<VarRpl>(&That)) {
    StringRef Name = SymbolTable::Table->makeFreshConstraintName();
    RplInclusionConstraint *RIC = new RplInclusionConstraint(Name, *this, That);
    SymbolTable::Table->addConstraint(RIC);
    return RK_DUNNO;
  }
  // else
  const ConcreteRpl *ConcThat = dyn_cast<ConcreteRpl>(&That);
  assert(ConcThat && "Unexpected kind of RPL");
  RplRef *LHS = new RplRef(*this);
  RplRef *RHS = new RplRef(*ConcThat);
  Trivalent Result = boolToTrivalent(LHS->isIncludedIn(*RHS));
  delete LHS; delete RHS;
  OSv2 << "DEBUG:: ~~~~~ isIncludedIn[RPL](" << this->toString()
    << "[" << this << "], " << That.toString() << "[" << &That
    << "])=" << (Result==RK_TRUE ? "true" : "false-or-dunno") << "\n";
  return Result;
}

Trivalent ConcreteRpl::isDisjoint(const Rpl &That) const {
  if (isa<VarRpl>(&That)) {
    return RK_DUNNO;
  }
  // else
  const ConcreteRpl *ConcThat = dyn_cast<ConcreteRpl>(&That);
  assert(ConcThat && "Unexpected kind of RPL");

  RplRef LHS1(*this);
  RplRef RHS1(*ConcThat);
  RplRef LHS2(*this);
  RplRef RHS2(*ConcThat);

  return boolToTrivalent(isPrivate() || ConcThat->isPrivate()
         || LHS1.isDisjointLeft(RHS1) || LHS2.isDisjointRight(RHS2));
}

Trivalent ConcreteRpl::substitute(const Substitution *S) {
  Trivalent Result = RK_FALSE; // set to true if substitution is
  if (!S || !S->getFrom() || !S->getTo())
    return Result; // Nothing to do.
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
    Result = RK_TRUE;
    if (const ConcreteRpl *ConcToRpl = dyn_cast<ConcreteRpl>(&ToRpl)) {
      OSv2 << "DEBUG:: found '" << FromEl.getName()
        << "' replaced with '" ;
      ToRpl.print(OSv2);
      I = RplElements.erase(I);
      I = RplElements.insert(I, ConcToRpl->RplElements.begin(),
                                ConcToRpl->RplElements.end());
      OSv2 << "' == '";
      print(OSv2);
      OSv2 << "'\n";
    } else {
      assert(isa<VarRpl>(&ToRpl) && "Unexpected kind of Rpl");
      addSubstitution(*S);
    }
  }
  os << "DEBUG:: after substitution(" << FromEl.getName() << "<-";
  ToRpl.print(os);
  os << "): ";
  print(os);
  os << "\n";
  return Result;
}

void ConcreteRpl::substitute(const SubstitutionSet *SubS) {
  if (SubS)
    SubS->applyTo(*this);
}

inline void ConcreteRpl::appendRplTail(ConcreteRpl *That) {
  if (!That)
    return;
  if (That->length()>1)
    RplElements.append(That->length()-1, (*(That->RplElements.begin() + 1)));
}

/*Rpl *ConcreteRpl::upperBound() {
  if (isEmpty() || !isa<CaptureRplElement>(RplElements.front()))
    return this;
  // else
  ConcreteRpl *upperBound = dyn_cast<CaptureRplElement>(RplElements.front())->upperBound();
  upperBound->appendRplTail(this);
  return upperBound;
}*/

void ConcreteRpl::join(Rpl *That) {
  if (!That)
    return;

  assert(isa<ConcreteRpl>(That) && "Unsupported join of Concrete with non-Concrete Rpl");
  ConcreteRpl *ConcThat = dyn_cast<ConcreteRpl>(That);

  ConcreteRpl Result;
  // join from the left
  RplElementVectorTy::const_iterator
    ThisI = this->RplElements.begin(),
    ThatI = ConcThat->RplElements.begin(),
    ThisE = this->RplElements.end(),
    ThatE = ConcThat->RplElements.end();
  for ( ; ThisI != ThisE && ThatI != ThatE && (*ThisI == *ThatI);
    ++ThisI, ++ThatI) {
      Result.appendElement(*ThisI);
  }
  if (ThisI != ThisE) {
    // put a star in the middle and join from the right
    assert(ThatI != ThatE);
    Result.appendElement(SymbolTable::STAR_RplElmt);
    Result.setFullySpecified(RK_FALSE);
    // count how many elements from the right we will need to append
    RplElementVectorTy::const_reverse_iterator
      ThisI = this->RplElements.rbegin(),
      ThatI = ConcThat->RplElements.rbegin(),
      ThisE = this->RplElements.rend(),
      ThatE = ConcThat->RplElements.rend();
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
  this->RplElements = Result.RplElements; // vector copy operation
  return;
}

/*Rpl *Rpl::capture() {
  if (this->isFullySpecified()) return this;
  else return new Rpl(*new CaptureRplElement(*this));
}*/

///////////////////////////////////////////////////////////////////////////////
//// VarRpl

Trivalent VarRpl::isUnder(const Rpl &That) const {
  if (this == &That)
    return RK_TRUE;
  else
    return RK_DUNNO;
}

Trivalent VarRpl::isIncludedIn(const Rpl &That) const {
  if (this == &That)
    return RK_TRUE;
  else {
    StringRef Name = SymbolTable::Table->makeFreshConstraintName();
    RplInclusionConstraint *RIC = new RplInclusionConstraint(Name, *this, That);
    SymbolTable::Table->addConstraint(RIC);
    return RK_DUNNO;
  }
}

Trivalent VarRpl::isDisjoint(const Rpl &That) const {
  if (this == &That)
    return RK_FALSE;
  else
    return RK_DUNNO;
}

void VarRpl::join(Rpl *That) {
  assert(false && "join operation not yet supported for VarRpl type");
}

Trivalent VarRpl::substitute(const Substitution *S) {
  if (S) {
    addSubstitution(*S);
    return RK_DUNNO;
  } else {
    return RK_FALSE;
  }
}

void VarRpl::substitute(const SubstitutionSet *SubS) {
  if (SubS)
    addSubstitution(*SubS);
}

void VarRpl::print(raw_ostream &OS) const {
  OS << "VarRpl:" << Name;
  Rpl::print(OS);
  if (Domain)
    Domain->print(OS);
}

term_t VarRpl::getPLTerm() const {
  term_t Result = PL_new_term_ref();
  functor_t RplFunctor = PL_new_functor(PL_new_atom(PL_VarRpl.c_str()), 2);
  // Name
  term_t RplElList = getRplElementsPLTerm();
  // Subs
  term_t SubList = getSubVPLTerm();
  // Build functor term
  int Res = PL_cons_functor(Result, RplFunctor, RplElList, SubList);
  assert(Res && "Failed to create prolog term_t for RPL");
  return Result;
}

term_t VarRpl::getRplElementsPLTerm() const {
  term_t RplElList = buildPLEmptyList();
  term_t RplEl = PL_new_term_ref();
  PL_put_atom_chars(RplEl, Name.data());
  bool Res = PL_cons_list(RplElList, RplEl, RplElList);
  assert(Res && "Failed to add RPL element to Prolog list term");

  return RplElList;
}
///////////////////////////////////////////////////////////////////////////////
//// ParameterSet

bool ParameterSet::hasElement(const RplElement *Elmt) const {
  for(const_iterator I = begin(), E = end();
      I != E; ++I) {
    if (Elmt == *I)
      return true;
  }
  return false;
}

inline void ParameterSet::print(llvm::raw_ostream &OS) const {
  if (size() <= 0)
    return;
  OS << "{";
  const_iterator I = begin(), E = end();
  if (I != E) {
    (*I)->print(OS);
    ++I;
  }
  for(; I != E; ++I) {
    OS << ",";
    (*I)->print(OS);
  }
  OS << "}";
}

std::string ParameterSet::toString() const {
  std::string SBuf;
  llvm::raw_string_ostream OS(SBuf);
  print(OS);
  return std::string(OS.str());
}


///////////////////////////////////////////////////////////////////////////////
//// ParameterVector
void ParameterVector::addToParamSet(ParameterSet *PSet) const {
  for(VectorT::const_iterator I = begin(), E = end();
      I != E; ++I) {
    const ParamRplElement *El = *I;
    PSet->insert(El);
  }
}

/// \brief Returns the ParamRplElement with name=Name or null.
const ParamRplElement *ParameterVector::lookup(StringRef Name) const {
  for(VectorT::const_iterator I = begin(), E = end();
      I != E; ++I) {
    const ParamRplElement *El = *I;
    if (El->getName().compare(Name) == 0)
      return El;
  }
  return 0;
}

/// \brief Returns true if the argument RplElement is contained.
bool ParameterVector::hasElement(const RplElement *Elmt) const {
  for(VectorT::const_iterator I = begin(), E = end();
      I != E; ++I) {
    if (Elmt == *I)
      return true;
  }
  return false;
}

void ParameterVector::take(ParameterVector *&PV) {
  if (!PV)
    return;

  BaseClass::take(PV);
  assert(PV->size()==0);
  delete PV;
  PV = 0;
}

void ParameterVector::assertzProlog () const {
  for (ParameterVector::const_iterator
          PI = begin(), PE = end();
        PI != PE; ++PI) {
    term_t ParamT = PL_new_term_ref();
    functor_t RPFunctor =
      PL_new_functor(PL_new_atom(PL_RgnParam.c_str()), 1);
    int Res = PL_cons_functor(ParamT, RPFunctor, (*PI)->getPLTerm());
    assert(Res && "Failed to create 'rgn_param' Prolog term");
    assertzTermProlog(ParamT,"Failed to assert 'rgn_param' to Prolog facts");
  }
}


///////////////////////////////////////////////////////////////////////////////
//// RplVector

RplVector::RplVector(const ParameterVector &ParamVec) {
  for(ParameterVector::const_iterator
        I = ParamVec.begin(),
        E = ParamVec.end();
      I != E; ++I) {
    if (*I) {
      ConcreteRpl Param(**I);
      push_back(Param);
    }
  }
}

  /// \brief Same as performing deref() DerefNum times.
std::unique_ptr<Rpl> RplVector::deref(size_t DerefNum) {
  std::unique_ptr<Rpl> Result;
  assert(DerefNum < size());
  for (VectorT::iterator I = begin();
        DerefNum > 0 && I != end(); ++I, --DerefNum) {
    Result.reset(*I);
    I = VectorT::erase(I);
  }
  return Result;
}

/// \brief Joins this to That.
void RplVector::join(RplVector *That) {
  if (!That)
    return;
  assert(That->size() == this->size());

  VectorT::iterator
          ThatI = That->begin(),
          ThisI = this->begin(),
          ThatE = That->end(),
          ThisE = this->end();
  for ( ;
        ThatI != ThatE && ThisI != ThisE;
        ++ThatI, ++ThisI) {
    Rpl *LHS = *ThisI;
    Rpl *RHS = *ThatI;
    assert(LHS);
    LHS->join(RHS); // modify *ThisI in place
  }
  return;
}

/// \brief Return true when this is included in That, false otherwise.
Trivalent RplVector::isIncludedIn (const RplVector &That) const {
  Trivalent Result = RK_TRUE;
  assert(That.size() == this->size());

  VectorT::const_iterator
          ThatI = That.begin(),
          ThisI = this->begin(),
          ThatE = That.end(),
          ThisE = this->end();
  for ( ;
        ThatI != ThatE && ThisI != ThisE;
        ++ThatI, ++ThisI) {
    Rpl *LHS = *ThisI;
    Rpl *RHS = *ThatI;
    Trivalent V = LHS->isIncludedIn(*RHS);
    if (V == RK_FALSE) {
      Result = RK_FALSE;
      break;
    } else if (V == RK_DUNNO) {
      Result = RK_DUNNO;
    }
  }
  OSv2 << "DEBUG:: [" << this->toString() << "] is "
      << (Result==RK_TRUE?"":"(possibly) not ")
      << "included in [" << That.toString() << "]\n";
  return Result;
}

void RplVector::substitute(const Substitution *S) {
  for(VectorT::const_iterator I = begin(), E = end();
        I != E; ++I) {
    if (*I)
      (*I)->substitute(S);
  }
}

void RplVector::substitute(const SubstitutionSet *S) {
  for(VectorT::const_iterator I = begin(), E = end();
        I != E; ++I) {
    if (*I)
      (*I)->substitute(S);
  }
}

//static
RplVector *RplVector::merge(const RplVector *A, const RplVector *B) {
  if (!A && !B)
    return 0;
  if (!A && B)
    return new RplVector(*B);
  if (A && !B)
    return new RplVector(*A);
  // invariant henceforth A!=null && B!=null
  RplVector *LHS;
  const RplVector *RHS;
  (A->size() >= B->size()) ? ( LHS = new RplVector(*A), RHS = B)
                           : ( LHS = new RplVector(*B), RHS = A);
  // fold RHS into LHS
  VectorT::const_iterator RHSI = RHS->begin(), RHSE = RHS->end();
  while (RHSI != RHSE) {
    LHS->push_back(*RHSI);
    ++RHSI;
  }
  return LHS;
}

/// \brief Returns the union of two RPL Vectors but destroys its inputs.
//static
RplVector *RplVector::destructiveMerge(RplVector *&A, RplVector *&B) {
  if (!A)
    return B;
  if (!B)
    return A;
  // invariant henceforth A!=null && B!=null
  RplVector *LHS, *RHS;
  (A->size() >= B->size()) ? ( LHS = A, RHS = B)
                           : ( LHS = B, RHS = A);
  // fold RHS into LHS
  VectorT::iterator RHSI = RHS->begin();
  while (RHSI != RHS->end()) {
    Rpl *R = *RHSI;
    LHS->VectorT::push_back(R);
    RHSI = RHS->VectorT::erase(RHSI);
  }
  delete RHS;
  A = B = 0;
  return LHS;
}

///////////////////////////////////////////////////////////////////////////////
//// RegionNameSet
const NamedRplElement *RegionNameSet::lookup (StringRef Name) {
  for (SetT::iterator I = begin(), E = end();
        I != E; ++I) {
    const NamedRplElement *El = *I;
    if (El->getName().compare(Name) == 0)
      return El;
  }
  return 0;
}

void RegionNameSet::assertzProlog() const {
  for (SetT::iterator I = begin(), E = end();
        I != E; ++I) {
    const NamedRplElement *El = *I;
    term_t RegionT = PL_new_term_ref();
    functor_t RNFunctor =
      PL_new_functor(PL_new_atom(PL_RgnName.c_str()), 1);
    int Res = PL_cons_functor(RegionT, RNFunctor, El->getPLTerm());
    assert(Res && "Failed to create 'rgn_name' Prolog term");
    assertzTermProlog(RegionT, "Failed to assert 'rgn_name' to Prolog facts");
  }
}

} // End namespace asap.
} // End namespace clang.

