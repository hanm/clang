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
    int x [[asap::arg("R")]];
  };

  class [[asap::region("First"), asap::region("Second")]] DataPair {
    Data first [[asap::arg("First")]];
    Data second [[asap::arg("Second")]];

    // FIXME: constructor.
  };

}
