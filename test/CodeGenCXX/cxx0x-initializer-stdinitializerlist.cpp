// RUN: %clang_cc1 -std=c++11 -S -emit-llvm -o - %s | FileCheck %s

namespace std {
  typedef decltype(sizeof(int)) size_t;

  // libc++'s implementation
  template <class _E>
  class initializer_list
  {
    const _E* __begin_;
    size_t    __size_;

    initializer_list(const _E* __b, size_t __s)
      : __begin_(__b),
        __size_(__s)
    {}

  public:
    typedef _E        value_type;
    typedef const _E& reference;
    typedef const _E& const_reference;
    typedef size_t    size_type;

    typedef const _E* iterator;
    typedef const _E* const_iterator;

    initializer_list() : __begin_(nullptr), __size_(0) {}

    size_t    size()  const {return __size_;}
    const _E* begin() const {return __begin_;}
    const _E* end()   const {return __begin_ + __size_;}
  };
}

void fn1(int i) {
  // CHECK: define void @_Z3fn1i
  // temporary array
  // CHECK: [[array:%[^ ]+]] = alloca [3 x i32]
  // CHECK: getelementptr inbounds [3 x i32]* [[array]], i{{32|64}} 0
  // CHECK-NEXT: store i32 1, i32*
  // CHECK-NEXT: getelementptr
  // CHECK-NEXT: store
  // CHECK-NEXT: getelementptr
  // CHECK-NEXT: load
  // CHECK-NEXT: store
  // init the list
  // CHECK-NEXT: getelementptr
  // CHECK-NEXT: getelementptr inbounds [3 x i32]*
  // CHECK-NEXT: store i32*
  // CHECK-NEXT: getelementptr
  // CHECK-NEXT: store i{{32|64}} 3
  std::initializer_list<int> intlist{1, 2, i};
}

struct destroyme1 {
  ~destroyme1();
};
struct destroyme2 {
  ~destroyme2();
};
struct witharg1 {
  witharg1(const destroyme1&);
  ~witharg1();
};


void fn2() {
  // CHECK: define void @_Z3fn2v
  void target(std::initializer_list<destroyme1>);
  // objects should be destroyed before dm2, after call returns
  // CHECK: call void @_Z6targetSt16initializer_listI10destroyme1E
  target({ destroyme1(), destroyme1() });
  // CHECK: call void @_ZN10destroyme1D1Ev
  destroyme2 dm2;
  // CHECK: call void @_ZN10destroyme2D1Ev
}

void fn3() {
  // CHECK: define void @_Z3fn3v
  // objects should be destroyed after dm2
  auto list = { destroyme1(), destroyme1() };
  destroyme2 dm2;
  // CHECK: call void @_ZN10destroyme2D1Ev
  // CHECK: call void @_ZN10destroyme1D1Ev
}

void fn4() {
  // CHECK: define void @_Z3fn4v
  void target(std::initializer_list<witharg1>);
  // objects should be destroyed before dm2, after call returns
  // CHECK: call void @_ZN8witharg1C1ERK10destroyme1
  // CHECK: call void @_Z6targetSt16initializer_listI8witharg1E
  target({ witharg1(destroyme1()), witharg1(destroyme1()) });
  // CHECK: call void @_ZN8witharg1D1Ev
  // CHECK: call void @_ZN10destroyme1D1Ev
  destroyme2 dm2;
  // CHECK: call void @_ZN10destroyme2D1Ev
}

void fn5() {
  // CHECK: define void @_Z3fn5v
  // temps should be destroyed before dm2
  // objects should be destroyed after dm2
  // CHECK: call void @_ZN8witharg1C1ERK10destroyme1
  auto list = { witharg1(destroyme1()), witharg1(destroyme1()) };
  // CHECK: call void @_ZN10destroyme1D1Ev
  destroyme2 dm2;
  // CHECK: call void @_ZN10destroyme2D1Ev
  // CHECK: call void @_ZN8witharg1D1Ev
}