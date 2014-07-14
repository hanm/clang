//===--- OwningVector.h - Owner version of llvm::SmallVector-*- C++ -*----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines a generic derived class of llvm::SmallVector that owns its
//  elements and calls delete on them when destroyed. It also call the copy
//  constructor each time an element is pushed into the vector. The client code
//  that removes an element gets it wrapped in an unique_ptr so that ownership is
//  transferred.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_OWNING_VECTOR_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_OWNING_VECTOR_H

#include <SWI-Prolog.h>

#include "llvm/ADT/SmallVector.h"
using llvm::SmallVector;


template<typename ElmtTyp, int SIZE>
class OwningVector : public llvm::SmallVector<ElmtTyp*, SIZE> {
public:
  // Types
  typedef llvm::SmallVector<ElmtTyp*, SIZE> VectorT;

  // Constructors
  OwningVector() : VectorT() {}

  OwningVector(const ElmtTyp &E) : VectorT() {
    push_back(new ElmtTyp(E));
  }

  OwningVector(const ElmtTyp *E) : VectorT() {
    if (E)
      push_back(new ElmtTyp(*E));
  }

  OwningVector(const OwningVector &From) : VectorT() {
    for (typename VectorT::const_iterator
            I = From.VectorT::begin(),
            E = From.VectorT::end();
         I != E; ++I) {
      push_back(*I); // push_back makes a copy of *I
    }
  }

  // Destructor
  ~OwningVector() {
    for (typename VectorT::const_iterator
            I = VectorT::begin(),
            E = VectorT::end();
         I != E; ++I) {
      delete (*I);
    }
  }

  // Methods
  /// \brief Append the argument Element to the vector.
  inline bool push_back (const ElmtTyp *E) {
    if (E) {
      VectorT::push_back(new ElmtTyp(*E));
      return true;
    } else {
      return false;
    }
  }

  inline void push_back (const ElmtTyp &E) {
    VectorT::push_back(new ElmtTyp(E));
  }

  inline std::unique_ptr<ElmtTyp> pop_back_val() {
    ElmtTyp *Back = VectorT::pop_back_val();
    return std::unique_ptr<ElmtTyp>(Back);
  }

  inline void pop_back() {
    ElmtTyp *Back = VectorT::pop_back_val();
    delete Back;
  }

  /// \brief Remove and return the first RPL in the vector.
  std::unique_ptr<ElmtTyp> pop_front() {
    if (VectorT::size() > 0) {
      ElmtTyp *Result = VectorT::front();
      VectorT::erase(VectorT::begin());
      return std::unique_ptr<ElmtTyp>(Result);
    } else {
      return std::unique_ptr<ElmtTyp>();
    }
  }

  /// \brief Merge @param OwV into @this. Leaves @param OwV empty.
  void take(OwningVector<ElmtTyp, SIZE> *OwV) {
    if (!OwV)
      return; // nothing to take, all done

    while(OwV->VectorT::size() > 0) {
      std::unique_ptr<ElmtTyp> front = OwV->pop_front();
      VectorT::push_back(front.get()); // non-cloning push back
      front.release(); // release without destroying
    }
  }

  typename VectorT::iterator erase (typename VectorT::iterator I) {
    ElmtTyp *Elm = *I;
    I = VectorT::erase(I);
    delete Elm;
    return I;
  }

  term_t getPLTerm() const {
    term_t Result = PL_new_term_ref();
    PL_put_nil(Result);
    int Res;

    for (typename VectorT::const_reverse_iterator
             I = VectorT::rbegin(),
             E = VectorT::rend();
         I != E; ++I) {
      term_t Term = (*I)->getPLTerm();
      Res = PL_cons_list(Result, Term, Result);
      assert(Res && "Failed to add OwningVector element to Prolog list term");
    }
    return Result;
  }

private:
  template<typename in_iter>
  void append(in_iter in_start, in_iter in_end); // NOT IMPLEMENTED

  void append(size_t NumInputs, const ElmtTyp &Elt); // NOT IMPLEMENTED

  typename VectorT::iterator erase (typename VectorT::iterator S, typename VectorT::iterator E);  // NOT IMPLEMENTED

  typename VectorT::iterator insert (typename VectorT::iterator I, const ElmtTyp &Elt);  // NOT IMPLEMENTED
  typename VectorT::iterator insert (typename VectorT::iterator I, size_t NumToInsert, const ElmtTyp &Elt);  // NOT IMPLEMENTED
  template<typename ItTy >
  typename VectorT::iterator insert (typename VectorT::iterator I, ItTy From, ItTy To);


}; // end class OwningVector

#endif

