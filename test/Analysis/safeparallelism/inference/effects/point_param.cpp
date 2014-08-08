// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=effect-inference %s -verify
//

class [[asap::param("P"), asap::region("Rx,Ry")]] Point {
	int x [[asap::arg("P:Rx")]];
	int y [[asap::arg("P:Ry")]];
    [[asap::param("Q")]] Point (const Point &P[[asap::arg("Q")]]);
public:
    Point() : x(0), y(0) {}
    Point(int x_, int y_) : x(x_), y(y_) {} // expected-warning{{Inferred Effect Summary for Point: [reads(rpl([rLOCAL],[]))]}}
    
	void setX(int _x) { x = _x; } // expected-warning{{Inferred Effect Summary for setX: [reads(rpl([rLOCAL],[])),writes(rpl([p0_P,r0_Rx],[]))]}}
	void setY(int _y) { y = _y; } // expected-warning{{Inferred Effect Summary for setY: [reads(rpl([rLOCAL],[])),writes(rpl([p0_P,r1_Ry],[]))]}}
	void setXY(int _x, int _y) { setX(_x); setY(_y); } // expected-warning{{Inferred Effect Summary for setXY: [writes(rpl([p0_P,r0_Rx],[])),reads(rpl([rLOCAL],[])),writes(rpl([p0_P,r1_Ry],[]))]}}
};

void foo [[asap::region("Rfoo")]] () { // expected-warning{{Inferred Effect Summary for foo: [writes(rpl([r2_Rfoo,r0_Rx],[])),reads(rpl([rLOCAL],[]))]}}
	Point *p1 [[asap::arg("Rfoo")]] = new Point();
	p1->setX(4);
}
