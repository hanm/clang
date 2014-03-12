// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=inference %s -verify


class [[asap::param("P"), asap::region("Rx,Ry")]] Point  {
    double x [[asap::arg("P:Rx")]];
    double y [[asap::arg("P:Ry")]];
public:
    Point() {}
    Point(double x_, double y_) : x(x_), y(y_) {} 
    // implicitly: [[asap::reads("Local")]] point(double x_ [[asap:arg("Local")]], double y_ [[asap::arg("Local")]]) : x(x_), y(y_) {}

    // getters and setters
    double getX () { return x; }
    double getY () { return y; }
    
    void setX (int x_) { x = x_; }
    void setY (int y_) { y = y_; }
    void setXY (int x_, int y_) { setX(x_); setY(y_); }
}; // end class Point

