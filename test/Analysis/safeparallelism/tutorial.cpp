// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify

// Tutorial ported from DPJ tutorial
// http://dpj.cs.uiuc.edu/DPJ/Download_files/DPJTutorial.html

/// expected-no-diagnostics
namespace ASPTutorial {
  // 2.1 Basic Concepts.
  class [[asap::region("Rx"), asap::region("Ry")]]
    Point {
    double X [[asap::arg("Rx")]];
    double Y [[asap::arg("Ry")]];

  public:
    void setX [[asap::writes("Rx")]] (double x) {
      X = x;
    }

    void setY [[asap::writes("Ry")]] (double y) {
      Y = y;
    }

    void setXY [[asap::writes("Rx"), asap::writes("Ry")]]
      (double x, double y) {
        // TODO - cobegin kind of parallel syntax support in asp
        this->setX(x);
        this->setY(y);
      }
  };

  // 2.2 Region Path List
  class [[asap::region("A"), asap::region("B"), asap::region("C")]]
    RPLExample {
    int X [[asap::arg("A:B")]];
    int Y [[asap::arg("A:C")]];

  public:
    void method [[asap::writes("A:*")]] (int x, int y) {
      X = x;
      Y = y;
    }
  };

  // 2.3 Class and Method Region Parameters.
  class [[asap::param("R")]] Data {
    friend class DataPair;
    int x [[asap::arg("R")]];
  };

  class [[asap::region("First"), asap::region("Second"), asap::region("Links")]]
    DataPair {
    Data *first [[asap::arg("Links"), asap::arg("First")]];
    Data *second [[asap::arg("Links"), asap::arg("Second")]];

  public:
    // Do we need to report effects here?
    // We only need to declare effects on the formal arguments.
    // The effects on fields being initialized do not need to be reported
    // because access to the object being initialized is not available to
    // any other code, making this object isolated (in its own private region)
    // until the constructor returns.
    // FIXME: Checker complains that the writes effect on Links is not covered
    // here. I must make a special case for constructors not to complain about
    // effects on anything directly under 'this'(i.e., this->F, for any field F)
    [[asap::no_effect]] // make this annotation implicit for constructors
    DataPair(Data *First [[asap::arg("First")]],
             Data *Second [[asap::arg("Second")]]) {
      // FIXME: this crashes the checker.
      first = First;
      second = Second;
    }

    void updateBoth [[asap::reads("Links"), 
                      asap::writes("First"), asap::writes("Second")]]
      (int firstX, int secondX) {
      // FIXME: cobegin
      first->x = firstX;
      second->x = secondX;
    }
  };

}
