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

  /// ATTENTION: the clients of pop_back_value are responsible for deallocating
  /// the memory of the object returned.
}; // end class OwningVector

#endif

