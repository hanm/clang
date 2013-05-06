// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
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
  virtual AreaType getArea [[asap::no_effect]] () = 0;  // expected-warning{{overridden virtual function does not cover the effects}}  expected-warning{{overridden virtual function does not cover the effects}} 

  // TODO
  // virtual bool overlapsBB [[ ... ]] (Rectangle *BB) = 0;
  // virtual bool overlaps [[ ... ]] (GeometricObject *GO) = 0;

}; // end GeometricObject


// Class Rectangle
class [[asap::param("P"), 
        asap::base_arg("GeometricObject", "P")]] 
Rectangle : public GeometricObject {
  double SideX [[asap::arg("P")]];
  double SideY [[asap::arg("P")]];

public:
  // Constructor
  //[[asap::no_effect]] 
  Rectangle ();

  //[[asap::no_effect]] 
  Rectangle (double X, double Y, double SideX, double SideY);

  // Virtuals
  virtual AreaType getArea [[asap::reads("P")]] ();

  // Setters & Getters
  inline double getSideX [[asap::reads("P")]] () { return SideX; }
  inline double getSideY [[asap::reads("P")]] () { return SideY; }

  void setSideX [[asap::writes("P")]] (double SideX);
  void setSideY [[asap::writes("P")]] (double SideY);
  void setSides [[asap::writes("P")]] (double SideX, double SideY);
  void set [[asap::writes("P")]] (double X, double Y, double SideX, double SideY);
  
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

// class Circle
class [[asap::param("P"), asap::base_arg("GeometricObject", "P")]]
Circle : public GeometricObject {
  double Radius [[asap::arg("P")]];

public:
  // Constructor
  Circle (double X, double Y, double R);

  // Virtuals
  virtual AreaType getArea [[asap::reads("P")]] ();

  // Setters & Getters
  void setRadius [[asap::writes("P")]] (double R);
  inline double getRadius [[asap::reads("P")]] () { return Radius; }

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

