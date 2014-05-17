// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=inference %s -verify
// expected-no-diagnostics

class [[asap::param("A, B")]] CollisionTree {
	void bar() {}
	
	void foo() {
		bar();
	}
};
