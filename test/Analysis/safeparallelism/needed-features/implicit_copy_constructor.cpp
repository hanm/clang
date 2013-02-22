// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify

/// expected-no-diagnostics

  class [[asap::param("R")]] Data {
  public:
    int x [[asap::arg("R")]];
    int y [[asap::arg("R")]];
  };


  // A version of DataPair using embedded objects
  class [[asap::region("First, Second, Links")]]
    DataPair {
    Data first [[asap::arg("First")]];
    Data second [[asap::arg("Second")]];

  public:
    [[asap::no_effect]]
    DataPair(Data First [[asap::arg("First")]],
             Data Second [[asap::arg("Second")]]) {
      first = First; // FIXME: This crashes because it is an implicit call
                     // to copy the whole class, which is not annotated with
                     // effects and thus looking up its effects fails
      second = Second;
    }

  }; // end class DataPair 

