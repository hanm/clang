// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
// expected-no-diagnostics

class [[asap::param("A, B")]] CollisionTree {
	CollisionTree() {}
	
	void foo() {
		CollisionTree bar [[asap::arg("B, A")]];
	}
};
