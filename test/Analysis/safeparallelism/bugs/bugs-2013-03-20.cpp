// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
// expected-no-diagnostics

class [[asap::param("P")]] point  {
  double m_x [[asap::arg("P")]];
  double m_y [[asap::arg("P")]];

public:
  point() {}
  point(double x, double y) : m_x(x), m_y(y) {}
  [[asap::param("Q"), asap::reads("Q")]]
  point(const point &p[[asap::arg("Q")]])
      : m_x(p.m_x), m_y(p.m_y)
      {}

  point &operator= [[asap::arg("P"), asap::param("Q"), asap::reads("Q"), asap::writes("P")]] (
    const point &p [[asap::arg("Q")]]
    )
    {
    m_x = p.m_x;
    m_y = p.m_y;

    // Using 'tmp' is a work-around for an ASP bug
    // point *tmp [[asap::arg("class")]] = this;
    //return *tmp;
    return *this;
    }
};

double simple [[asap::reads("Global")]] (const double &d) {
  return d;
}

double simple3 [[asap::param("P"),asap::reads("P")]] (
  const double &d [[asap::arg("P")]]) { 
  return d;
}
