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
//  that removes an element is responsible for deallocating it when done.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_OWNING_VECTOR_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_OWNING_VECTOR_H

#include "llvm/ADT/SmallVector.h"

using llvm::SmallVector;
using std::auto_ptr;

// TODO: employ llvm::OwningPtr
// i.e., derive from llvm::SmallVector<llvm::OwningPtr<ElmtTyp>, SIZE>
// so that pop_back_val returns an OwningPtr and memory leaks are avoided.
template<typename ElmtTyp, int SIZE>
class OwningVector : public llvm::SmallVector<ElmtTyp*, SIZE> {
public:
  // Types
  typedef llvm::SmallVector<ElmtTyp*, SIZE> VectorT;

public:
  /// Constructors
  OwningVector() {}

  OwningVector(const ElmtTyp &E) {
    push_back(new ElmtTyp(E));
  }

  OwningVector(const ElmtTyp *S) {
    if (S)
      push_back(new ElmtTyp(*S));
  }

  /// Destructor
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

  inline auto_ptr<ElmtTyp> pop_back_val() {
    ElmtTyp *Back = VectorT::pop_back_val();
    return auto_ptr<ElmtTyp>(Back);
  }

  inline void pop_back() {
    ElmtTyp *Back = VectorT::pop_back_val();
    delete Back;
  }

private:
  template<typename in_iter>
  void append(in_iter in_start, in_iter in_end); // NOT IMPLEMENTED

  void append(size_t NumInputs, const ElmtTyp &Elt); // NOT IMPLEMENTED

  typename VectorT::iterator erase (typename VectorT::iterator I);  // NOT IMPLEMENTED
  typename VectorT::iterator erase (typename VectorT::iterator S, typename VectorT::iterator E);  // NOT IMPLEMENTED

  typename VectorT::iterator insert (typename VectorT::iterator I, const ElmtTyp &Elt);  // NOT IMPLEMENTED
  typename VectorT::iterator insert (typename VectorT::iterator I, size_t NumToInsert, const ElmtTyp &Elt);  // NOT IMPLEMENTED
  template<typename ItTy >
  typename VectorT::iterator insert (typename VectorT::iterator I, ItTy From, ItTy To);


}; // end class OwningVector

#endif

