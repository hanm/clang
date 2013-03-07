//=== Substitution.cpp - Safe Parallelism checker -----*- C++ -*-----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------===//
//
// This files defines the Substitution class used by the Safe Parallelism
// checker, which tries to prove the safety of parallelism given region
// and effect annotations.
//
//===----------------------------------------------------------------===//

using namespace clang::asap;

// TODO support substitution over multiple parameters
class Substitution {
private:
  // Fields
  const RplElement *FromEl; // RplElement not owned by class
  const Rpl *ToRpl;         // Rpl not owned by class
public:
  // Constructor
  Substitution() : FromEl(0),  ToRpl(0) {}

  Substitution(const RplElement *FromEl, const Rpl *ToRpl) :
    FromEl(FromEl), ToRpl(ToRpl) {
    assert(FromEl);
    assert(ToRpl);
  }
  // Getters
  inline const RplElement *getFrom() const { return FromEl; }
  inline const Rpl *getTo() const { return ToRpl; }
  // Setters
  void set(const RplElement *FromEl, const Rpl *ToRpl) {
    this->FromEl = FromEl;
    this->ToRpl = ToRpl;
  }
  // Apply
  /// \brief Apply substitution to RPL
  inline void applyTo(Rpl *R) const {
    if (FromEl && ToRpl) {
      assert(R);
      R->substitute(*FromEl, *ToRpl);
    }
  }
  // print
  /// \brief Print Substitution: [From<-To]
  void print(raw_ostream &OS) const {
    StringRef FromName = "<MISSING>";
    StringRef ToName = "<MISSING>";
    if (FromEl)
      FromName = FromEl->getName();
    if (ToRpl)
      ToName = ToRpl->toString();
    OS << "[" << FromName << "<-" << ToName << "]";
  }
  /// \brief Return a string for the Substitution.
  inline std::string toString() const {
    std::string SBuf;
    llvm::raw_string_ostream OS(SBuf);
    print(OS);
    return std::string(OS.str());
  }


}; // end class Substitution

// An ordered sequence of substitutions
class SubstitutionVector {
public:
  // Type
#ifndef SUBSTITUTION_VECTOR_SIZE
  #define SUBSTITUTION_VECTOR_SIZE 4
#endif
  typedef llvm::SmallVector<Substitution*, SUBSTITUTION_VECTOR_SIZE>
    SubstitutionVecT;
private:
  SubstitutionVecT SubV;
public:
  // Constructor
  SubstitutionVector() {}
  SubstitutionVector(Substitution *S) { SubV.push_back(S); }
  // Methods
  /// \brief Return an iterator at the first RPL of the vector.
  inline SubstitutionVecT::iterator begin () { return SubV.begin(); }
  /// \brief Return an iterator past the last RPL of the vector.
  inline SubstitutionVecT::iterator end () { return SubV.end(); }
  /// \brief Return a const_iterator at the first RPL of the vector.
  inline SubstitutionVecT::const_iterator begin () const { return SubV.begin(); }
  /// \brief Return a const_iterator past the last RPL of the vector.
  inline SubstitutionVecT::const_iterator end () const { return SubV.end(); }
  /// \brief Return the size of the RPL vector.
  inline size_t size () const { return SubV.size(); }
  /// \brief Append the argument Substitution to the Substitution vector.
  inline void push_back(Substitution *S) {
    assert(S);
    SubV.push_back(S);
  }

  // Apply
  void applyTo(Rpl *R) const {
    assert(R);
    for(SubstitutionVecT::const_iterator I = SubV.begin(), E = SubV.end();
        I != E; ++I) {
      assert(*I);
      (*I)->applyTo(R);
    }
  }
  // Print
  /// \brief Print Substitution vector.
  void print(raw_ostream &OS) const {
    SubstitutionVecT::const_iterator
      I = SubV.begin(),
      E = SubV.end();
    for(; I != E; ++I) {
      assert(*I);
      (*I)->print(OS);
    }
  }
  /// \brief Return a string for the Substitution vector.
  inline std::string toString() const {
    std::string SBuf;
    llvm::raw_string_ostream OS(SBuf);
    print(OS);
    return std::string(OS.str());
  }


}; // end class SubstitutionVector
