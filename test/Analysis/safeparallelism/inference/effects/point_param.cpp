// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=inference %s -verify
//
// expected-no-diagnostics

class [[asap::param("P"), asap::region("Rx,Ry")]] Point {
	int x [[asap::arg("P:Rx")]];
	int y [[asap::arg("P:Ry")]];
    [[asap::param("Q")]] Point (const Point &P[[asap::arg("Q")]]);
public:
    Point() : x(0), y(0) {}
    Point(int x_, int y_) : x(x_), y(y_) {}
    
	void setX(int _x) { x = _x; }
	void setY(int _y) { y = _y; }
	void setXY(int _x, int _y) { setX(_x); setY(_y); }
};

void foo [[asap::region("Rfoo")]] () {
	Point *p1 [[asap::arg("Rfoo")]] = new Point();
	p1->setX(4);
}
