// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
// XFAIL: *

[[asap::region("Left")]];

class [[asap::param("P")]] CollisionTree {
	CollisionTree *left [[asap::arg("P:Left, P:Left")]];

	void intersect [[asap::param("P_cT")]] [[asap::reads("P:*, P_cT:*")]] (
			CollisionTree *collisionTree [[asap::arg("P_cT")]]);
};

// When explicitly given, the region arg on the fn param below should match that 
// of the canonical declaration. 
// When not given, the default annotation scheme is currently used which is wrong.
// Instead, the same annotation should be copied from the canonical decl.
// I am not sure why, even though asap::arg is an inheritable attribute, it is not 
// copied. Perhaps for fn parameters, there is no canonical declaration as there is 
// for functions.
void CollisionTree::intersect (
		CollisionTree *collisionTree [[asap::arg("Global")]])
{
	collisionTree->intersect(this->left);
}
