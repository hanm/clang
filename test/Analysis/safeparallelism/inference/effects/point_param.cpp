// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=inference %s -verify
//
//

class [[asap::param("P"), asap::region("Rx,Ry")]] Point {
	int x [[asap::arg("P:Rx")]];
	int y [[asap::arg("P:Ry")]];
    [[asap::param("Q")]] Point (const Point &P[[asap::arg("Q")]]);
public:
    Point() : x(0), y(0) {}
    Point(int x_, int y_) : x(x_), y(y_) {} // expected-warning{{Solution for Point: [Reads Effect on Local,Reads Effect on Local]}}
    
	void setX(int _x) { x = _x; } // expected-warning{{Solution for setX: [Reads Effect on Local,Writes Effect on P:Rx]}}
	void setY(int _y) { y = _y; } // expected-warning{{Solution for setY: [Reads Effect on Local,Writes Effect on P:Ry]}}
	void setXY(int _x, int _y) { setX(_x); setY(_y); } // expected-warning{{Solution for setXY: [Reads Effect on Local,Invocation Effect,Reads Effect on Local,Invocation Effect]}}
};

void foo [[asap::region("Rfoo")]] () { // expected-warning{{Solution for foo: [Invocation Effect,Reads Effect on Local]}}
	Point *p1 [[asap::arg("Rfoo")]] = new Point();
	p1->setX(4);
}
