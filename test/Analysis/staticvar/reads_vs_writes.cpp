// RUN: %clang_cc1  -analyze -analyzer-checker=alpha.StaticVariableAccessChecker -verify %s

void* operator new(unsigned); 
void* operator new[](unsigned); 
void operator delete(void*);
void operator delete[](void*); 

//==============================================================================
// Classification of static variable references into reads and writes.
//==============================================================================

// First, a whole bunch of variables with static duration are
// declared. 

// Variable of a simple type
int i,j,k,l;
bool b;

// Array
int a[4] = {0,1,2,3};

// Pointer
int* p = new int[5];

// Reference
int& r = i;

struct C
{
  int d;
  static int sd;

  C(int i=0) : d(i) {}

  int get() const;
  void set(int i);
};

struct D
{
  int d;
  static int sd;

  D(int i=0) : d(i) {}

  int get() const;
  void set(int i);
};


// Global binary ops
D& operator  +=(D&, int);
D& operator  -=(D&, int);
D& operator  *=(D&, int);
D& operator  /=(D&, int);
D& operator  %=(D&, int);
D& operator <<=(D&, int);
D& operator >>=(D&, int);
D& operator  &=(D&, int);
D& operator  |=(D&, int);
D& operator  ^=(D&, int);

      D& operator,(const D&, D&);
const D& operator,(const D&, const D&);

int  operator ->*(const D&, int D::*);
int& operator ->*(D&, int D::*);

// Global unary ops
int  operator *(const D&);
int& operator *(D&);

D*        operator &(D&);
const D*  operator &(const D&);

D& operator++(D&);
D& operator--(D&);
D operator++(D&, int);
D operator--(D&, int);


struct E
{
  int d;
  static int sd;

  E(int i=0) : d(i) {}

  int get() const;
  void set(int i);

  E& operator=(int);
  E& operator=(const E&);

  operator int() const;

  // Binary ops
  E& operator  +=(int);
  E& operator  -=(int);
  E& operator  *=(int);
  E& operator  /=(int);
  E& operator  %=(int);
  E& operator <<=(int);
  E& operator >>=(int);
  E& operator  &=(int);
  E& operator  |=(int);
  E& operator  ^=(int);
  
        E& operator,(E&) const;
  const E& operator,(const E&) const;

  int& operator [](int);
  int  operator [](int) const;

  E*       operator->();
  const E* operator->() const;

  int& operator ->*(int E::*);
  int  operator ->*(int E::*) const;
  
  // Unary ops
  int  operator *();
  int& operator *() const;
  
  const E*  operator &() const;
  E*        operator &();
  
  E& operator++();
  E& operator--();
  E operator++(int);
  E operator--(int);
  
  E& operator()(int);
  const E& operator()(int) const;

  E& operator()(const char*, ...);
  const E& operator()(const char*, ...) const;

  void* operator new(unsigned) ;
  void* operator new[](unsigned) ;
  void operator delete(void*) throw();
  void operator delete[](void*) throw();

};

// Object
C c = C(3);
D d = D(5);
E e = E(7);

const C c_cst = C(3);
const D d_cst = D(5);
const E e_cst = E(7);

// Pointer to object
C* cp = new C(5);
D* dp = new D(5);
E* ep = new E(5);

const C* cp_cst = new C(5);
const D* dp_cst = new D(5);
const E* ep_cst = new E(5);

// Reference to object
C& cr = c;
D& dr = d;
E& er = e;

const C& cr_cst = c;
const D& dr_cst = d;
const E& er_cst = e;

namespace function_parameters {

  template <typename T>
  void byValue(T);

  template <typename T>
  void byCstRef(const T&);
  template <typename T>
  void byRef(T&);

  template <typename T>
  void byPtr(T*);
  template <typename T>
  void byPtrToCst(const T*);

  void f1() 
  {
    byValue(i);     
    byCstRef(i);    
    byRef(i);       
    byPtr(&i);      
    byPtrToCst(&i); 

    byValue(c);     
    byCstRef(c);    
    byRef(c);       
    byPtr(&c);      
    byPtrToCst(&c); 

    byValue(cr);                                           
    byCstRef(cr);                                          
    byRef(cr);      
    byPtr(&cr);     
    byPtrToCst(&cr);

    byValue(*cp);                                          
    byCstRef(*cp);                                         
    byRef(*cp);     
    byPtr(cp);      
    byPtrToCst(cp); 


    byValue(d);     
    byCstRef(d);    
    byRef(d);       
    byPtr(&d);      
    byPtrToCst(&d); 
    byPtrToCst(&(const D)d); 

    byValue(dr);                                           
    byCstRef(dr);                                          
    byRef(dr);      
    byPtr(&dr);     
    byPtrToCst(&dr);
    byPtrToCst(&(const D&)dr);

    byValue(*dp);                                          
    byCstRef(*dp);                                         
    byRef(*dp);     
    byPtr(dp);      
    byPtrToCst(dp); 


    byValue(e);     
    byCstRef(e);    
    byRef(e);       
    byPtr(&e);      
    byPtrToCst(&e); 
    byPtrToCst(&(const E)e); 

    byValue(er);                                           
    byCstRef(er);                                          
    byRef(er);      
    byPtr(&er);     
    byPtrToCst(&er);
    byPtrToCst(&(const E&)er); 

    byValue(*ep);                                          
    byCstRef(*ep);                                         
    byRef(*ep);     
    byPtr(ep);      
    byPtrToCst(ep); 


    byValue(c_cst);     
    byCstRef(c_cst);    
    byPtrToCst(&c_cst); 

    byValue(cr_cst);                                           
    byCstRef(cr_cst);                                          
    byPtrToCst(&cr_cst);

    byValue(*cp_cst);                                          
    byCstRef(*cp_cst);                                         
    byPtrToCst(cp_cst); 


    byValue(d_cst);     
    byCstRef(d_cst);    
    byPtrToCst(&d_cst); 

    byValue(dr_cst);                                           
    byCstRef(dr_cst);                                          
    byPtrToCst(&dr_cst);

    byValue(*dp_cst);                                          
    byCstRef(*dp_cst);                                         
    byPtrToCst(dp_cst); 


    byValue(e_cst);     
    byCstRef(e_cst);    
    byPtrToCst(&e_cst); 

    byValue(er_cst);                                           
    byCstRef(er_cst);                                          
    byPtrToCst(&er_cst);

    byValue(*ep_cst);                                          
    byCstRef(*ep_cst);                                         
    byPtrToCst(ep_cst); 
  }

}

namespace member_function {

  void f1(int x) 
  {
    x += c.get();   
    c.set(x);       
    x += cr.get();  
    cr.set(x);      
    x += cp->get(); 
    cp->set(x);     

    x += d.get();   
    d.set(x);       
    x += dr.get();  
    dr.set(x);      
    x += dp->get(); 
    dp->set(x);     

    x += e.get();   
    e.set(x);       
    x += er.get();  
    er.set(x);      
    x += ep->get(); 
    ep->set(x);     
  }
}

namespace assign {

  void f1(int x) 
  {
    x = x;
    i = x;      // expected-warning-re{{Write to the variable '.*' with static duration}}
    x = i;      
    i = i;      // expected-warning-re{{Write to the variable '.*' with static duration}} \
                

    c   = C(1); // expected-warning-re{{Write to the variable '.*' with static duration}}
    cr  = C(2); // expected-warning-re{{Write to the variable '.*' with static duration}}
    *cp = C(3); // expected-warning-re{{Write to the variable '.*' with static duration}}

    d   = D(1); // expected-warning-re{{Write to the variable '.*' with static duration}}
    dr  = D(2); // expected-warning-re{{Write to the variable '.*' with static duration}}
    *dp = D(3); // expected-warning-re{{Write to the variable '.*' with static duration}}

    e   = E(1); // expected-warning-re{{Write to the variable '.*' with static duration}}
    er  = E(2); // expected-warning-re{{Write to the variable '.*' with static duration}}
    *ep = E(3); // expected-warning-re{{Write to the variable '.*' with static duration}}

    e   = 1;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    er  = 2;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    *ep = 3;    // expected-warning-re{{Write to the variable '.*' with static duration}}
  }
}

namespace unary_ops {

  void f1(int x) 
  {
    *&i = x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    (i) = x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    *+p = x;    // expected-warning-re{{Write to the variable '.*' with static duration}}

    *&c  = C(1);// expected-warning-re{{Write to the variable '.*' with static duration}}
    (c)  = C(2);// expected-warning-re{{Write to the variable '.*' with static duration}}
    *+cp = C(3);// expected-warning-re{{Write to the variable '.*' with static duration}}

    *&d  = D(1);// expected-warning-re{{Write to the variable '.*' with static duration}}
    (d)  = D(2);// expected-warning-re{{Write to the variable '.*' with static duration}}
    *+dp = D(3);// expected-warning-re{{Write to the variable '.*' with static duration}}

    *&e  = E(1);// expected-warning-re{{Write to the variable '.*' with static duration}}
    (e)  = E(2);// expected-warning-re{{Write to the variable '.*' with static duration}}
    *+ep = E(3);// expected-warning-re{{Write to the variable '.*' with static duration}}
  }

}

namespace pre_post {

  void f1() 
  {
    i++;        // expected-warning-re{{Write to the variable '.*' with static duration}}
    i--;        // expected-warning-re{{Write to the variable '.*' with static duration}}
    ++i;        // expected-warning-re{{Write to the variable '.*' with static duration}}
    --i;        // expected-warning-re{{Write to the variable '.*' with static duration}}

    (c.d)++;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    (c.d)--;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    ++(c.d);    // expected-warning-re{{Write to the variable '.*' with static duration}}
    --(c.d);    // expected-warning-re{{Write to the variable '.*' with static duration}}

    (cr.d)++;   // expected-warning-re{{Write to the variable '.*' with static duration}}
    (cr.d)--;   // expected-warning-re{{Write to the variable '.*' with static duration}}
    ++(cr.d);   // expected-warning-re{{Write to the variable '.*' with static duration}}
    --(cr.d);   // expected-warning-re{{Write to the variable '.*' with static duration}}

    (cp->d)++;  // expected-warning-re{{Write to the variable '.*' with static duration}}
    (cp->d)--;  // expected-warning-re{{Write to the variable '.*' with static duration}}
    ++(cp->d);  // expected-warning-re{{Write to the variable '.*' with static duration}}
    --(cp->d);  // expected-warning-re{{Write to the variable '.*' with static duration}}

    d++;        // expected-warning-re{{Write to the variable '.*' with static duration}}
    d--;        // expected-warning-re{{Write to the variable '.*' with static duration}}
    ++d;        // expected-warning-re{{Write to the variable '.*' with static duration}}
    --d;        // expected-warning-re{{Write to the variable '.*' with static duration}}
                
    dr++;       // expected-warning-re{{Write to the variable '.*' with static duration}}
    dr--;       // expected-warning-re{{Write to the variable '.*' with static duration}}
    ++dr;       // expected-warning-re{{Write to the variable '.*' with static duration}}
    --dr;       // expected-warning-re{{Write to the variable '.*' with static duration}}

    (*dp)++;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    (*dp)--;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    ++(*dp);    // expected-warning-re{{Write to the variable '.*' with static duration}}
    --(*dp);    // expected-warning-re{{Write to the variable '.*' with static duration}}

    e++;        // expected-warning-re{{Write to the variable '.*' with static duration}}
    e--;        // expected-warning-re{{Write to the variable '.*' with static duration}}
    ++e;        // expected-warning-re{{Write to the variable '.*' with static duration}}
    --e;        // expected-warning-re{{Write to the variable '.*' with static duration}}
                
    er++;       // expected-warning-re{{Write to the variable '.*' with static duration}}
    er--;       // expected-warning-re{{Write to the variable '.*' with static duration}}
    ++er;       // expected-warning-re{{Write to the variable '.*' with static duration}}
    --er;       // expected-warning-re{{Write to the variable '.*' with static duration}}

    (*ep)++;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    (*ep)--;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    ++(*ep);    // expected-warning-re{{Write to the variable '.*' with static duration}}
    --(*ep);    // expected-warning-re{{Write to the variable '.*' with static duration}}
  }

}

namespace assign_ops {

  void f1(int x, bool lb) 
  {
    i  += x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    i  -= x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    i  *= x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    i  /= x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    i  %= x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    i <<= x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    i >>= x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    i  &= x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    i  |= x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    i  ^= x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
  
    b  &= lb;   // expected-warning-re{{Write to the variable '.*' with static duration}}
    b  |= lb;   // expected-warning-re{{Write to the variable '.*' with static duration}}
    b  ^= lb;   // expected-warning-re{{Write to the variable '.*' with static duration}}

    d  += x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    d  -= x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    d  *= x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    d  /= x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    d  %= x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    d <<= x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    d >>= x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    d  &= x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    d  |= x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    d  ^= x;    // expected-warning-re{{Write to the variable '.*' with static duration}}

    e  += x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    e  -= x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    e  *= x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    e  /= x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    e  %= x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    e <<= x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    e >>= x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    e  &= x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    e  |= x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    e  ^= x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
  }

}

namespace compound_assigns {

  void f1(int x) 
  {
    i  = j  = x;    // expected-warning-re 2 {{Write to the variable '.*' with static duration}}
    i  = x += x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    x  = i += x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    i += x  = x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    x += i  = x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    i += x += x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    x += i += x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
                    
    (i  = j)  = x;  // expected-warning-re{{Write to the variable '.*' with static duration}} \
                    
    (i  = x) += x;  // expected-warning-re{{Write to the variable '.*' with static duration}}
    (x  = i) += x;  // expected-warning{{unsequenced modification and access to 'x'}} 
    (i += x)  = x;  // expected-warning-re{{Write to the variable '.*' with static duration}}
    (x += i)  = x;  // expected-warning{{unsequenced modification and access to 'x'}}
    (i += x) += x;  // expected-warning-re{{Write to the variable '.*' with static duration}}
    (x += i) += x;  // expected-warning{{unsequenced modification and access to 'x'}} 
                    
    --i = x;        // expected-warning-re{{Write to the variable '.*' with static duration}}
                    
    ++(x = i);      
    ++(x = ++i);    // expected-warning-re{{Write to the variable '.*' with static duration}}
    ++(i = x);      // expected-warning-re{{Write to the variable '.*' with static duration}}

    ++(x += i);     
    ++(x += ++i);   // expected-warning-re{{Write to the variable '.*' with static duration}}
    ++(i += x);     // expected-warning-re{{Write to the variable '.*' with static duration}}
  }

  void f2(int x) 
  {
    d  = dr = x;    // expected-warning-re 2 {{Write to the variable '.*' with static duration}}
    d  = x += x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    d += x  = x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    d += x += x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
                    
    (d  = dr) = x;  // expected-warning-re{{Write to the variable '.*' with static duration}} \
                    
    (d  = x) += x;  // expected-warning-re{{Write to the variable '.*' with static duration}}
    (d += x)  = x;  // expected-warning-re{{Write to the variable '.*' with static duration}}
    (d += x) += x;  // expected-warning-re{{Write to the variable '.*' with static duration}}
                    
    --d = x;        // expected-warning-re{{Write to the variable '.*' with static duration}}
                    
    ++(d = x);      // expected-warning-re{{Write to the variable '.*' with static duration}}

    ++(d += x);     // expected-warning-re{{Write to the variable '.*' with static duration}}
  }

  void f3(int x) 
  {
    e  = er = x;    // expected-warning-re 2 {{Write to the variable '.*' with static duration}}
    e  = x += x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    x  = e += x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    e += x  = x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    x += e  = x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    e += x += x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
    x += e += x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
                    
    (e  = er) = x;  // expected-warning-re{{Write to the variable '.*' with static duration}} \
                    
    (e  = x) += x;  // expected-warning-re{{Write to the variable '.*' with static duration}}
    (x  = e) += x;  // expected-warning{{unsequenced modification and access to 'x'}}
    (e += x)  = x;  // expected-warning-re{{Write to the variable '.*' with static duration}}
    (x += e)  = x;  // expected-warning{{unsequenced modification and access to 'x'}}
    (d += x) += x;  // expected-warning-re{{Write to the variable '.*' with static duration}}
    (x += e) += x;  // expected-warning{{unsequenced modification and access to 'x'}}
                    
    --e = x;        // expected-warning-re{{Write to the variable '.*' with static duration}}
                    
    ++(x = e);      
    ++(x = ++e);    // expected-warning-re{{Write to the variable '.*' with static duration}}
    ++(e = x);      // expected-warning-re{{Write to the variable '.*' with static duration}}

    ++(x += e);     
    ++(x += ++e);   // expected-warning-re{{Write to the variable '.*' with static duration}}
    ++(e += x);     // expected-warning-re{{Write to the variable '.*' with static duration}}
  }
}

namespace comma_op {

  void f1(int x) 
  {
    x,i = 3;        // expected-warning-re{{Write to the variable '.*' with static duration}} \
                    // expected-warning{{expression result unused}}
    i,x = 3;    // expected-warning{{expression result unused}}

    (x,i) = 3;      // expected-warning-re{{Write to the variable '.*' with static duration}} \
                    // expected-warning{{expression result unused}}
    (i,x) = 3;        // expected-warning{{expression result unused}}

    
    x,c = C(3);     // expected-warning-re{{Write to the variable '.*' with static duration}} \
                    // expected-warning{{expression result unused}}
    c,x = 3;       // expected-warning{{expression result unused}}

    (x,c) = C(3);   // expected-warning-re{{Write to the variable '.*' with static duration}} \
                    // expected-warning{{expression result unused}}
    (c,x) = 3;  // expected-warning{{expression result unused}}

    
    x,d = D(3);     // expected-warning-re{{Write to the variable '.*' with static duration}} 
    d,x = 3;        

    D ld;
    (ld,d) = D(3);  // expected-warning-re{{Write to the variable '.*' with static duration}} 
    (d,ld) = D(4);  


    x,e = E(3);     // expected-warning-re{{Write to the variable '.*' with static duration}} \
                    // expected-warning{{expression result unused}}
    e,x = 3;        

    E le;
    (le,e) = E(3);  // expected-warning-re{{Write to the variable '.*' with static duration}} 
    (e,le) = E(4);  
  }

}

namespace array_subscript {

  void f1(int x, int la[4], int* lp,
                      C lc,   D ld,   E le)
  {
    a[x] = x;   // expected-warning-re{{Write to the variable '.*' with static duration}}
    x[a] = x;   // expected-warning-re{{Write to the variable '.*' with static duration}}
    p[x] = x;   // expected-warning-re{{Write to the variable '.*' with static duration}}
    x[p] = x;   // expected-warning-re{{Write to the variable '.*' with static duration}}
    
    la[i] = x;  
    i[la] = x;  
    lp[i] = x;  
    i[lp] = x;  

    cp[x] = lc; // expected-warning-re{{Write to the variable '.*' with static duration}}
    x[cp] = lc; // expected-warning-re{{Write to the variable '.*' with static duration}}
    lc = cp[x]; 
    lc = x[cp]; 

    dp[x] = ld; // expected-warning-re{{Write to the variable '.*' with static duration}}
    x[dp] = ld; // expected-warning-re{{Write to the variable '.*' with static duration}}
    ld = dp[x]; 
    ld = x[dp]; 

    ep[x] = le; // expected-warning-re{{Write to the variable '.*' with static duration}}
    x[ep] = le; // expected-warning-re{{Write to the variable '.*' with static duration}}
    le = ep[x]; 
    le = x[ep]; 
  }

}

namespace pointer_arithmetic {

  void f1(int x, int la[4], int* lp,
                      C lc,   D ld,   E le)
  {
    *(a + x) = x;   // expected-warning-re{{Write to the variable '.*' with static duration}}
    *(x + a) = x;   // expected-warning-re{{Write to the variable '.*' with static duration}}
    *(p + x) = x;   // expected-warning-re{{Write to the variable '.*' with static duration}}
    *(x + p) = x;   // expected-warning-re{{Write to the variable '.*' with static duration}}
    
    *(a - x) = x;   // expected-warning-re{{Write to the variable '.*' with static duration}}
    *(p - x) = x;   // expected-warning-re{{Write to the variable '.*' with static duration}}
    
    *(la + i) = x;  
    *(lp + i) = x;  
    *(i + la) = x;  
    *(i + lp) = x;  

    *(la - i) = x;  
    *(lp - i) = x;  

    *(cp + x) = lc; // expected-warning-re{{Write to the variable '.*' with static duration}}
    *(x + cp) = lc; // expected-warning-re{{Write to the variable '.*' with static duration}}
    lc = *(cp + x); 
    lc = *(x + cp); 

    *(cp - x) = lc; // expected-warning-re{{Write to the variable '.*' with static duration}}
    lc = *(cp - x); 

    *(dp + x) = ld; // expected-warning-re{{Write to the variable '.*' with static duration}}
    *(x + dp) = ld; // expected-warning-re{{Write to the variable '.*' with static duration}}
    ld = *(dp + x); 
    ld = *(x + dp); 

    *(dp - x) = ld; // expected-warning-re{{Write to the variable '.*' with static duration}}
    ld = *(dp - x); 

    *(ep + x) = le; // expected-warning-re{{Write to the variable '.*' with static duration}}
    *(x + ep) = le; // expected-warning-re{{Write to the variable '.*' with static duration}}
    le = *(ep + x); 
    le = *(x + ep); 

    *(ep - x) = le; // expected-warning-re{{Write to the variable '.*' with static duration}}
    le = *(ep - x); 
  }

}

namespace conditional_op {

  void f1(int x, int y, bool lb) 
  {
    (b ? x : y) = x;                     
    (lb ? i : j) = x;                       // expected-warning-re 2 {{Write to the variable '.*' with static duration}}
    *(lb ? &i : &j) = x;                    // expected-warning-re 2 {{Write to the variable '.*' with static duration}}
    (lb ? (lb ? i : j) : (lb ? k : l)) = x; // expected-warning-re 4 {{Write to the variable '.*' with static duration}}
    (lb ? (lb ? i : i) : (lb ? i : i)) = x; // expected-warning-re 4 {{Write to the variable '.*' with static duration}}
  }

}

namespace operator_call {

  void f1(int x) 
  {
    e(1);       
    er(1);      
    (*ep)(1);   
    ep[2](1);   

    (*(const E*)ep)(1);     
    ((const E*)ep)[2](1);   

    e("%d %d", 1, 2);       
    er("%d %d", 1, 2);      
    (*ep)("%d %d", 1, 2);   
    ep[2]("%d %d", 1, 2);   

    (*(const E*)ep)("%d %d", 1, 2);     
    ((const E*)ep)[2]("%d %d", 1, 2);   
  }

}

namespace ptr_to_mem {

  void f1(int x) 
  {
    (c.*&C::set)(x);            
    (c.*&C::operator=)(C(x));   
    x += (c.*&C::get)();        
    c.*&C::d = 1;               // expected-warning-re{{Write to the variable '.*' with static duration}}
    x += c.*&C::d;              

    (cr.*&C::set)(x);           
    (cr.*&C::operator=)(C(x));  
    x += (cr.*&C::get)();       
    cr.*&C::d = 1;              // expected-warning-re{{Write to the variable '.*' with static duration}}
    x += cr.*&C::d;             

    (cp->*&C::set)(x);          
    (cp->*&C::operator=)(C(x)); 
    x += (cp->*&C::get)();      
    cp->*&C::d = 1;             // expected-warning-re{{Write to the variable '.*' with static duration}}
    x += cp->*&C::d;            


    (d.*&D::set)(x);            
    (d.*&D::operator=)(D(x));   
    x += (d.*&D::get)();        
    d.*&D::d = 1;               // expected-warning-re{{Write to the variable '.*' with static duration}}
    x += d.*&D::d;              

    (dr.*&D::set)(x);           
    (dr.*&D::operator=)(D(x));  
    x += (dr.*&D::get)();       
    dr.*&D::d = 1;              // expected-warning-re{{Write to the variable '.*' with static duration}}
    x += dr.*&D::d;             

    (dp->*&D::set)(x);          
    (dp->*&D::operator=)(D(x)); 
    x += (dp->*&D::get)();      
    dp->*&D::d = 1;             // expected-warning-re{{Write to the variable '.*' with static duration}}
    x += dp->*&D::d;            


    // We need a typedef because the assignement operator is overloaded in E.
    typedef E& (E::*Assign)(const E&);
    Assign assign = &E::operator=;
    
    (e.*&E::set)(x);            
    (e.*assign)(E(x));          
    x += (e.*&E::get)();        
    e.*&E::d = 1;               // expected-warning-re{{Write to the variable '.*' with static duration}}
    x += e.*&E::d;              

    (er.*&E::set)(x);           
    (er.*assign)(E(x));         
    x += (er.*&E::get)();       
    er.*&E::d = 1;              // expected-warning-re{{Write to the variable '.*' with static duration}}
    x += er.*&E::d;             

    (ep->*&E::set)(x);          
    (ep->*assign)(E(x));        
    x += (ep->*&E::get)();      
    ep->*&E::d = 1;             // expected-warning-re{{Write to the variable '.*' with static duration}}
    x += ep->*&E::d;            
  }
}

namespace ptr_to_static_members {

  typedef void (*FuncPtr)();
  
  void fooFunc(FuncPtr);
  void fooData(int*);
  void fooData(const int*);

  struct A 
  {
    static void func();
    static int i;
    static const int i_cst = 4;
  };
  
  void f1() 
  {
    fooFunc(A::func);
    fooData(&A::i);                 
    fooData((const int*)(&A::i));   
    fooData(&A::i_cst);             
  }

  void f2(A* a) 
  {
    fooFunc(a->func);
    fooData(&a->i);                 
    fooData((const int*)(&a->i));   
    fooData(&a->i_cst);             
  }

  void f3(A& a) 
  {
    fooFunc(a.func);
    fooData(&a.i);                  
    fooData((const int*)(&a.i));    
    fooData(&a.i_cst);              
  }
}

namespace new_delete {

  void f1() 
  {
    C* local_c = new C(1);
    delete local_c;
    
    D* local_d = new D(1);
    delete local_d;
    
    E* local_e = new E(1);
    delete local_e;
  }

  void f2() 
  {
    C* local_c = new C[4];
    delete [] local_c;
    
    D* local_d = new D[4];
    delete [] local_d;
    
    E* local_e = new E[4];
    delete [] local_e;
  }

  void f3() 
  {
    cp = new C(1);  // expected-warning-re{{Write to the variable '.*' with static duration}}
    delete cp;      // expected-warning-re{{Write to the variable '.*' with static duration}}
    
    dp = new D(1);  // expected-warning-re{{Write to the variable '.*' with static duration}}
    delete dp;      // expected-warning-re{{Write to the variable '.*' with static duration}}
    
    ep = new E(1);  // expected-warning-re{{Write to the variable '.*' with static duration}}
    delete ep;      // expected-warning-re{{Write to the variable '.*' with static duration}}
  }

  void f4() 
  {
    cp = new C[4];  // expected-warning-re{{Write to the variable '.*' with static duration}}
    delete [] cp;   // expected-warning-re{{Write to the variable '.*' with static duration}}
    
    dp = new D[4];  // expected-warning-re{{Write to the variable '.*' with static duration}}
    delete [] dp;   // expected-warning-re{{Write to the variable '.*' with static duration}}
    
    ep = new E[4];  // expected-warning-re{{Write to the variable '.*' with static duration}}
    delete [] ep;   // expected-warning-re{{Write to the variable '.*' with static duration}}
  }

}


namespace writes_to_static_variable_init_bindings {

  void f1()
  {
    const int* lp_cst = p;  
    lp_cst = p;             
    int* lp = p;            
    lp = p;                 

    const C* lcp_cst = cp;  
    lcp_cst = cp;           
    C* lcp = cp;            
    lcp = cp;               
    
    const D* ldp_cst = dp;  
    ldp_cst = dp;           
    D* ldp = dp;            
    ldp = dp;               
    
    const E* lep_cst = ep;  
    lep_cst = ep;           
    E* lep = ep;            
    lep = ep;               

    const int* lfcst = &c.d;
    lfcst = &c.d;           
    int* lf = &c.d;         
    lf = &c.d;              

  }
  
  void f2()
  {
    const int& li_cst = i;  
    int& li = i;            
    li = i;                 

    const C& lc_cst = c;    
    C& lc = c;              
    lc = c;                 
                            
    const D& ld_cst = d;    
    D& ld = d;              
    ld = d;                 
                            
    const E& le_cst = e;    
    E& le = e;              
    le = e;                 

    const int& lfcst = c.d; 
    int& lf = c.d;          
    lf = c.d;               
  }

  void f3()
  {
    const int* lp_cst(p);   
    lp_cst = p;             
    int* lp(p);             
    lp = p;                 

    const C* lcp_cst(cp);   
    lcp_cst = cp;           
    C* lcp(cp);             
    lcp = cp;               
    
    const D* ldp_cst(dp);   
    ldp_cst = dp;           
    D* ldp(dp);             
    ldp = dp;               
    
    const E* lep_cst(ep);   
    lep_cst = ep;           
    E* lep(ep);             
    lep = ep;               

    const int* lfcst(&c.d); 
    lfcst = &c.d;           
    int* lf(&c.d);          
    lf = &c.d;              
  }
  
  void f4()
  {
    const int& li_cst(i);   
    int& li(i);             
    li = i;                 

    const C& lc_cst(c);     
    C& lc(c);               
    lc = c;                 
                            
    const D& ld_cst(d);     
    D& ld(d);               
    ld = d;                 
                            
    const E& le_cst(e);     
    E& le(e);               
    le = e;                 

    const int& lfcst(c.d);  
    int& lf(c.d);           
    lf = c.d;               
  }
}


namespace writes_to_static_variable_constructor_bindings {

  class A 
  {
  public:
    A(C&);
    A(int, C&);
    A(C&, const C&, int, C&);
  };

  void foo(const A&);

  void f1()
  {
    A a1(c);      
    A a2(2,       
         c);      
    A a3(c,       
         c,       
         2,       
         c);      

    A a4 = A(c);  
    A a5 = A(2,
             c);  
    A a6 = A(c,   
             c,   
             2,
             c);  

    foo(A(c));    
    foo(A(2,       
          c));    
    foo(A(c,      
          c,      
          2,       
          c));    
  }
}


namespace writes_to_static_variable_multi_level_pointer_bindings {
    
  template <typename T>
  void func(T t);

  static int**** v;
  static C**** x;
  static D**** y;
  static E**** z;
  
  void f1()
  {
    func<int       *       *      *      *       >(v); 
    func<int       *       *      *const *       >(v); 
    func<int       *       *const *const *       >(v); 
    func<int       * const *const *const *       >(v); 
    func<int const * const *const *const *       >(v); 

    func<int       *       *      *      * const >(v); 
    func<int       *       *      *const * const >(v); 
    func<int       *       *const *const * const >(v); 
    func<int       * const *const *const * const >(v); 
    func<int const * const *const *const * const >(v); 

    func<C         *       *      *      *       >(x); 
    func<C         *       *      *const *       >(x); 
    func<C         *       *const *const *       >(x); 
    func<C         * const *const *const *       >(x); 
    func<C   const * const *const *const *       >(x); 

    func<C         *       *      *      * const >(x); 
    func<C         *       *      *const * const >(x); 
    func<C         *       *const *const * const >(x); 
    func<C         * const *const *const * const >(x); 
    func<C   const * const *const *const * const >(x); 

    func<D         *       *      *      *       >(y); 
    func<D         *       *      *const *       >(y); 
    func<D         *       *const *const *       >(y); 
    func<D         * const *const *const *       >(y); 
    func<D   const * const *const *const *       >(y); 

    func<D         *       *      *      * const >(y); 
    func<D         *       *      *const * const >(y); 
    func<D         *       *const *const * const >(y); 
    func<D         * const *const *const * const >(y); 
    func<D   const * const *const *const * const >(y); 

    func<E         *       *      *      *       >(z); 
    func<E         *       *      *const *       >(z); 
    func<E         *       *const *const *       >(z); 
    func<E         * const *const *const *       >(z); 
    func<E   const * const *const *const *       >(z); 

    func<E         *       *      *      * const >(z); 
    func<E         *       *      *const * const >(z); 
    func<E         *       *const *const * const >(z); 
    func<E         * const *const *const * const >(z); 
    func<E   const * const *const *const * const >(z); 
  }
  
  void f2()
  {
    func<int       *       *      *      *      &>(v); 
    func<int       *       *      *      * const&>(v); 
    func<int       *       *      *const * const&>(v); 
    func<int       *       *const *const * const&>(v); 
    func<int       * const *const *const * const&>(v); 
    func<int const * const *const *const * const&>(v); 

    func<C         *       *      *      *      &>(x); 
    func<C         *       *      *      * const&>(x); 
    func<C         *       *      *const * const&>(x); 
    func<C         *       *const *const * const&>(x); 
    func<C         * const *const *const * const&>(x); 
    func<C   const * const *const *const * const&>(x); 

    func<D         *       *      *      *      &>(y); 
    func<D         *       *      *      * const&>(y); 
    func<D         *       *      *const * const&>(y); 
    func<D         *       *const *const * const&>(y); 
    func<D         * const *const *const * const&>(y); 
    func<D   const * const *const *const * const&>(y); 

    func<E         *       *      *      *      &>(z); 
    func<E         *       *      *      * const&>(z); 
    func<E         *       *      *const * const&>(z); 
    func<E         *       *const *const * const&>(z); 
    func<E         * const *const *const * const&>(z); 
    func<E   const * const *const *const * const&>(z); 
  }
  
  void f3()
  {
    func<int       *      *      *       >(*v); 
    func<int       *      *const *       >(*v); 
    func<int       *const *const *       >(*v); 
    func<int const *const *const *       >(*v); 

    func<int       *      *      * const >(*v); 
    func<int       *      *const * const >(*v); 
    func<int       *const *const * const >(*v); 
    func<int const *const *const * const >(*v); 

    func<C         *      *      *       >(*x); 
    func<C         *      *const *       >(*x); 
    func<C         *const *const *       >(*x); 
    func<C   const *const *const *       >(*x); 

    func<C         *      *      * const >(*x); 
    func<C         *      *const * const >(*x); 
    func<C         *const *const * const >(*x); 
    func<C   const *const *const * const >(*x); 

    func<D         *      *      *       >(*y); 
    func<D         *      *const *       >(*y); 
    func<D         *const *const *       >(*y); 
    func<D   const *const *const *       >(*y); 

    func<D         *      *      * const >(*y); 
    func<D         *      *const * const >(*y); 
    func<D         *const *const * const >(*y); 
    func<D   const *const *const * const >(*y); 

    func<E         *      *      *       >(*z); 
    func<E         *      *const *       >(*z); 
    func<E         *const *const *       >(*z); 
    func<E   const *const *const *       >(*z); 

    func<E         *      *      * const >(*z); 
    func<E         *      *const * const >(*z); 
    func<E         *const *const * const >(*z); 
    func<E   const *const *const * const >(*z); 
  }
  
  void f4()
  {
    func<int       *       *       *      *      *       >(&v); 
    func<int       *       *       *      *const *       >(&v); 
    func<int       *       *       *const *const *       >(&v); 
    func<int       *       * const *const *const *       >(&v); 
    func<int       * const * const *const *const *       >(&v); 
    func<int const * const * const *const *const *       >(&v); 
                     
    func<int       *       *       *      *      * const >(&v); 
    func<int       *       *       *      *const * const >(&v); 
    func<int       *       *       *const *const * const >(&v); 
    func<int       *       * const *const *const * const >(&v); 
    func<int       * const * const *const *const * const >(&v); 
    func<int const * const * const *const *const * const >(&v); 
                     
    func<C         *       *       *      *      *       >(&x); 
    func<C         *       *       *      *const *       >(&x); 
    func<C         *       *       *const *const *       >(&x); 
    func<C         *       * const *const *const *       >(&x); 
    func<C         * const * const *const *const *       >(&x); 
    func<C   const * const * const *const *const *       >(&x); 
                     
    func<C         *       *       *      *      * const >(&x); 
    func<C         *       *       *      *const * const >(&x); 
    func<C         *       *       *const *const * const >(&x); 
    func<C         *       * const *const *const * const >(&x); 
    func<C         * const * const *const *const * const >(&x); 
    func<C   const * const * const *const *const * const >(&x); 
                     
    func<D         *       *       *      *      *       >(&y); 
    func<D         *       *       *      *const *       >(&y); 
    func<D         *       *       *const *const *       >(&y); 
    func<D         *       * const *const *const *       >(&y); 
    func<D         * const * const *const *const *       >(&y); 
    func<D   const * const * const *const *const *       >(&y); 
                     
    func<D         *       *       *      *      * const >(&y); 
    func<D         *       *       *      *const * const >(&y); 
    func<D         *       *       *const *const * const >(&y); 
    func<D         *       * const *const *const * const >(&y); 
    func<D         * const * const *const *const * const >(&y); 
    func<D   const * const * const *const *const * const >(&y); 
                     
    func<E         *       *       *      *      *       >(&z); 
    func<E         *       *       *      *const *       >(&z); 
    func<E         *       *       *const *const *       >(&z); 
    func<E         *       * const *const *const *       >(&z); 
    func<E         * const * const *const *const *       >(&z); 
    func<E   const * const * const *const *const *       >(&z); 
                     
    func<E         *       *       *      *      * const >(&z); 
    func<E         *       *       *      *const * const >(&z); 
    func<E         *       *       *const *const * const >(&z); 
    func<E         *       * const *const *const * const >(&z); 
    func<E         * const * const *const *const * const >(&z); 
    func<E   const * const * const *const *const * const >(&z); 
  }
}


namespace writes_to_static_variable_statement_bindings {
  
  void f1()
  {
    for (int* i = p               ;;) {}
    for (int *i = 0, *j = p       ;;) {}
    for (const int *i = 0, *j = p ;;) {}

    for (C* lcp = cp;;) {}              
    for (D* ldp = dp;;) {}              
    for (E* lep = ep;;) {}              

    if (      int* i = p) {}            
    if (const int *i = p) {}            
                                         
    if (C* lcp = cp) {}                 
    if (D* ldp = dp) {}                 
    if (E* lep = ep) {}                 

    while (      int* i = p) {}         
    while (const int *i = p) {}         

    while (C* lcp = cp) {}              
    while (D* ldp = dp) {}              
    while (E* lep = ep) {}              
  }

}


namespace writes_to_static_variable_return_bindings {
  
  int* f1() 
  {
    return p; 
  }
  
  const int* f2() 
  {
    return p; 
  }
  
  const int f3() 
  {
    return i; 
  }
  
  int& f4() 
  {
    return i; 
  }
  
  const int& f5() 
  {
    return i; 
  }
  
}


namespace writes_to_static_member_variable_init_bindings {
  
struct Foo {
  Foo();
  Foo(C&);
  Foo(const C&, C&);
  Foo(int, C&, const C&, C&);
};

struct TestMembers {
  TestMembers()
    : f_i(i),   
      f_p(p),   
      f_pc(p),  
      f_r(i),   
      f_rc(i),  
      f_c1(c),  
      f_c2(c,   
           c),  
      f_c3(2,
           c,   
           c,   
           c)   
      
  {}

  int           f_i;
  int*          f_p;
  const int*    f_pc;
  int&          f_r;
  const int&    f_rc;
  Foo           f_c1;
  Foo           f_c2;
  Foo           f_c3;
};

struct TestBase1 : public Foo {
  TestBase1() 
    : Foo(c)    
  {}
};

struct TestBase2 : public Foo {
  TestBase2() 
    : Foo(c,    
          c)    
  {}
};

struct TestBase3 : public Foo {
  TestBase3() 
    : Foo(2,
          c,    
          c,    
          c)    
  {}
};

}

namespace writes_to_static_default_argument {

  void foo(int i, C& arg = c) 
  {}

  void f1()
  {
    C lc;
    
    foo(1);     
    foo(2, c);  
    foo(3, lc);
  }

}


namespace writes_to_static_template_default_argument {

  template <typename T, T& t = c>
  struct A
  {
    static void foo(int i, T& arg = t);
  };

  template <typename T, T& t>
  void foo(int i, T& arg = t);

  void f1()
  {
    C lc;
    
    A<C>::foo(1);       
    A<C>::foo(2, c);    
    A<C>::foo(3, lc);

    A<C,c>::foo(1);      \
                         
    A<C,c>::foo(2, c);   \
                        
    A<C,c>::foo(3, lc);  

    foo<C,c>(1);         \
                        
    foo<C,c>(2, c);      \
                        
    foo<C,c>(3, lc);    
  }
  
}
