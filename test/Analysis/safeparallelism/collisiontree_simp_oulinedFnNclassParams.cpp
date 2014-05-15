// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
// expected-no-diagnostics

[[asap::region("Left")]];

class [[asap::param("P")]] CollisionTree {
	CollisionTree *left [[asap::arg("P:Left, P:Left")]];

	void intersect [[asap::param("P_cT")]] [[asap::reads("P:*, P_cT:*")]] (
			CollisionTree *collisionTree [[asap::arg("P_cT")]]);
};

void CollisionTree::intersect (
		CollisionTree *collisionTree [[asap::arg("P_cT")]])
{
	collisionTree->intersect(this->left);
}
