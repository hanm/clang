// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=simple  %s -verify
// expected-no-diagnostics

class [[asap::param("P")]] IntersectInvoker {
public:
	void operator() [[asap::reads("P")]] () const {}
};

class [[asap::region("R")]] CollisionTree {
public:
	void intersect [[asap::reads("R")]] ()
	{
		IntersectInvoker leftFn [[asap::arg("R")]];
		leftFn();
	}
};
