//===--- OwningPtrSet.h - Owner version of llvm::SmallPtrSet-*- C++ ---*---===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines a generic derived class of llvm::SmallPtrSet that owns its
//  elements and calls delete on them when destroyed. It also call the copy
//  constructor each time an element is inserted into the set. The client code
//  must use 'take' instead of erase to keep possesion of a pointer removed from
//  the set.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_OWNING_PTRSET_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_ASAP_OWNING_PTRSET_H

#include "llvm/ADT/SmallPtrSet.h"
using llvm::SmallPtrSet;


template<typename T, int SIZE>
class OwningPtrSet : public llvm::SmallPtrSet<T*, SIZE> {
public:
  // Types
  typedef llvm::SmallPtrSet<T*, SIZE> SetT;

  // Constructors
  OwningPtrSet() {}

  OwningPtrSet(const T &E) {
    insert(E);
  }

  OwningPtrSet(const T *E) {
    insert(E);
  }

  OwningPtrSet(const OwningPtrSet &Set) : SetT() {
    for (typename SetT::const_iterator
            I = Set.SetT::begin(),
            E = Set.SetT::end();
         I != E; ++I) {
      insert(*I);
    }
  }

  // Destructor
  ~OwningPtrSet() {
    for (typename SetT::const_iterator
            I = SetT::begin(),
            E = SetT::end();
         I != E; ++I) {
      delete (*I);
    }
  }

  bool insert(const T *E) {
    if (E)
      return SetT::insert(new T(*E));
    else
      return false;
  }

  inline bool insert(const T &E) {
    return SetT::insert(new T(E));
  }


  std::unique_ptr<T> take_one() {
    if (SetT::size() > 0) {
      T *Result = *SetT::begin();
      SetT::erase(*SetT::begin());
      return std::unique_ptr<T>(Result);
    } else {
      return std::unique_ptr<T>();
    }
  }

  /// \brief Merge @param OwS into @this. Leaves @param OwS empty.
  void take(OwningPtrSet<T, SIZE> *OwS) {
    if (!OwS)
      return; // nothing to take, all done

    while(OwS->SetT::size() > 0) {
      std::unique_ptr<T> One = OwS->take_one();
      SetT::insert(One.release()); // non-cloning push back
      //front.release(); // release without destroying
    }
  }

  bool erase(T *E) {
    bool Result = SetT::erase(E);
    if (Result) {
      delete E;
    }
    return Result;
  }

  inline bool take(T *E) {
    return SetT::erase(E);
  }

  void print (llvm::raw_ostream& OS) const {
    if(SetT::size() <= 0)
      return;

    OS << "{";
    typename SetT::const_iterator I = SetT::begin(), E = SetT::end();
    if (I != E) {
      (*I)->print(OS);
      ++I;
    }
    for(; I != E; ++I) {
      OS << ", ";
      (*I)->print(OS);
    }
    OS << "}";
  }

  inline std::string toString() const {
    std::string SBuf;
    llvm::raw_string_ostream OS(SBuf);
    print(OS);
    return std::string(OS.str());
  }

  term_t getPLTerm() const {
    term_t Result = clang::asap::buildPLEmptyList();
    int Res;

    for (typename SetT::const_iterator I = SetT::begin(), E = SetT::end();
         I != E; ++I) {
      term_t Term = (*I)->getPLTerm();
      Res = PL_cons_list(Result, Term, Result);
      assert(Res && "Failed to add OwningPtrSet element to Prolog list term");
    }
    return Result;
  }
}; // end class OwningPtrSet

#endif
