// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
//
// expected-no-diagnostics
//

typedef double AreaType;

class [[asap::param("P")]] GeometricObject {
protected:
  double X [[asap::arg("P")]];
  double Y [[asap::arg("P")]];

  // TODO add caching
  //  Cacheable_Value<AreaType> area _amp_arg(P:C);
  //  GeometricObjectMutexType cache_mutex _amp_arg(P:C);  
  
public:
  [[asap::no_effect]] 
  GeometricObject(double X, double Y) : X(X), Y(Y) {};

  inline double getX [[asap::reads("P")]] () { return X; }
  inline double getY [[asap::reads("P")]] () { return Y; }

  virtual inline void setX [[asap::writes("P")]] (double V) {
    // TODO use scoped lock
    X = V;
  }

  virtual inline void setY [[asap::writes("P")]] (double V) {
    // TODO use scoped lock
    Y = V;
  }

  //virtual AreaType getArea [[asap::reads("P")]] () = 0;
  virtual AreaType getArea [[asap::reads("P")]] () = 0;  

  // TODO
  // virtual bool overlapsBB [[ ... ]] (Rectangle *BB) = 0;
  // virtual bool overlaps [[ ... ]] (GeometricObject *GO) = 0;

}; // end GeometricObject


// Class Rectangle
class [[asap::param("Pr"), 
        asap::base_arg("GeometricObject", "Pr")]] 
Rectangle : public GeometricObject {
protected:
  double SideX [[asap::arg("Pr")]];
  double SideY [[asap::arg("Pr")]];

public:
  // Constructor
  //[[asap::no_effect]] 
  Rectangle ();

  //[[asap::no_effect]] 
  Rectangle (double X, double Y, double SideX, double SideY);

  // Virtuals
  virtual AreaType getArea [[asap::reads("Pr")]] ();

  // Setters & Getters
  inline double getSideX [[asap::reads("Pr")]] () { return SideX; }
  inline double getSideY [[asap::reads("Pr")]] () { return SideY; }

  void setSideX [[asap::writes("Pr")]] (double SideX);
  void setSideY [[asap::writes("Pr")]] (double SideY);
  void setSides [[asap::writes("Pr")]] (double SideX, double SideY);
  void set [[asap::writes("Pr")]] (double X, double Y, double SideX, double SideY);
  
}; // end class Rectangle

Rectangle::Rectangle ()
  : GeometricObject(0,0), SideX(0), SideY(0) { }

Rectangle::Rectangle (double X, double Y, double SideX, double SideY)
  : GeometricObject(X, Y), SideX(SideX), SideY(SideY) { }

AreaType Rectangle::getArea() {
    // TODO support caching 
    // return area.get_from_cache(Rectangle_Area(*this), cache_mutex); // reads P, atomic writes P:C
    return SideX * SideY;
}

void Rectangle::setSideX(double SideX) {
  // GeometricObjectMutexType::scoped_lock lock(mutex);  // locks P
  // area.invalidate();                  // writes P:C
  this->SideX = SideX;                // atomic writes P
}

void Rectangle::setSideY(double SideY) {
  // GeometricObjectMutexType::scoped_lock lock(mutex);  // locks P
  // area.invalidate();                                  // writes P:C
  this->SideY = SideY;                                 // atomic writes P
}

void Rectangle::setSides(double SideX, double SideY) {
  // GeometricObjectMutexType::scoped_lock lock(mutex);  // locks P
  // area.invalidate();                                  // writes P:C
  this->SideY = SideY;                                   // atomic writes P
  this->SideX = SideX;                                   // atomic writes P
}

void Rectangle::set(double X, double Y, double SideX, double SideY) {
  // GeometricObjectMutexType::scoped_lock lock(mutex);  // locks P
  // area.invalidate();                                  // writes P:C
  this->X = X;                                           // atomic writes P
  this->Y = Y;                                           // atomic writes P
  this->SideX = SideX;                                   // atomic writes P
  this->SideY = SideY;                                   // atomic writes P
}


///////////////////////////////////////////////////////////////////////////////
// class Square
//
class [[asap::param("Ps"), asap::base_arg("Rectangle", "Ps")]]
Square : public Rectangle {
public:
  // Constructor
  Square(double X, double Y, double Side);

};

Square::Square(double X, double Y, double Side)
  : Rectangle(X, Y, Side, Side) {}


///////////////////////////////////////////////////////////////////////////////
// class Qube
class [[asap::param("Pq"), asap::base_arg("Square", "Pq")]]
Cube : public Square {
protected:
  double SideZ [[asap::arg("Pq")]];
public:
  // Constructor
  Cube(double X, double Y, double Side);

  // Virtuals
  AreaType getArea [[asap::reads("Pq")]] ();
};

Cube::Cube(double X, double Y, double Side)
  : Square(X, Y, Side), SideZ(Side) {}

AreaType Cube::getArea() {
  return 6 * SideZ * SideZ;
}

///////////////////////////////////////////////////////////////////////////////
// class Circle
class [[asap::param("Pc"), asap::base_arg("GeometricObject", "Pc")]]
Circle : public GeometricObject {
  double Radius [[asap::arg("Pc")]];

public:
  // Constructor
  Circle (double X, double Y, double R);

  // Virtuals
  virtual AreaType getArea [[asap::reads("Pc")]] ();

  // Setters & Getters
  void setRadius [[asap::writes("Pc")]] (double R);
  inline double getRadius [[asap::reads("Pc")]] () { return Radius; }

}; // end class Circle


Circle::Circle (double X, double Y, double R)
  : GeometricObject(X,Y), Radius(R) {}

AreaType Circle::getArea() {
  return 2 * 3.14 * Radius;
}

void Circle::setRadius(double R) {
  // GeometricObjectMutexType::scoped_lock lock(mutex);
  // area.invalidate();    // writes P:C
  // bbox.invalidate();    // writes P:BB
  Radius = R;              // atomic writes P
}

