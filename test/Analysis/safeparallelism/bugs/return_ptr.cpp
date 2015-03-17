// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=simple %s -verify
// expected-no-diagnostics

int * foo /*[[asap::arg("Local, Global")]]*/ () {
	return nullptr;
}

void test() {
	int * ip = foo();
}
