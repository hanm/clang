// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=parametric-effect-inference %s -verify

class [[asap::region("Left, Right")]] [[asap::param("R")]] CollisionTree {

protected:
	// children trees
	CollisionTree *left [[asap::arg("R:Left, R:Left")]];
	CollisionTree *right [[asap::arg("R:Right, R:Right")]];

public:
	void intersect [[asap::param("R_cT")]] /*[[asap::reads("R:*, R_cT:*")]]*/ (
			CollisionTree &collisionTree [[asap::arg("R_cT")]]) //; 
{ // expected-warning{{[reads(rpl([p0_R,rSTAR,r0_Left],[])),reads(rpl([p0_R,rSTAR,r1_Right],[])),reads(rpl([p1_R_cT,rSTAR,r0_Left],[])),reads(rpl([p1_R_cT,rSTAR,r1_Right],[]))]}}
	if (left != nullptr) {
		collisionTree.intersect(*left);
		collisionTree.intersect(*right);
	}
	else if (collisionTree.left != nullptr) {
		this->intersect(*collisionTree.left);
		this->intersect(*collisionTree.right);
	}
}

};
/*
void CollisionTree::intersect (CollisionTree &collisionTree [[asap::arg("R_cT")]])
{
	if (left != nullptr) {
		collisionTree.intersect(*left);
		collisionTree.intersect(*right);
	}
	else if (collisionTree.left != nullptr) {
		this->intersect(*collisionTree.left);
		this->intersect(*collisionTree.right);
	}
}*/
