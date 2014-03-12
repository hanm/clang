// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=inference %s -verify
//
// expected-no-diagnostics
class split {
};

template<typename T>
class blocked_range {

  //typedef std::size_t size_type;
  typedef unsigned int size_type;

  T my_begin;
  T my_end;
  size_type my_grainsize;

 
  bool empty() const {return !(my_begin<my_end);}
  bool nonEmpty() const {return (my_begin<my_end);}

  size_type size() const {
    return size_type(my_end-my_begin);
  }

  bool is_divisible() const {return my_grainsize<size();}

  static T do_split( blocked_range& r ) {
    T middle = r.my_begin + (r.my_end-r.my_begin)/2u;
    r.my_end = middle;
    return middle;
  }

  blocked_range( blocked_range& r, split ) :
    my_end(r.my_end),
    my_begin(do_split(r)),
    my_grainsize(r.my_grainsize)
  {}

};
