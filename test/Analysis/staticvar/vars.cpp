// RUN: %clang_cc1  -analyze -analyzer-checker=alpha.StaticVariableAccessChecker -verify %s

//==============================================================================
// Variables with static duration
//==============================================================================

// First, a whole bunch of variables with static duration are
// declared. We also declare some functions because functions all have
// static duration and we should not complained about them.

int i_global;

int i_global_init = 3;

extern int i_extern;

extern "C" int i_extern_c;

static int i_static = 7;

const int i_const = 6;

static const int i_static_const = 6;

int f_global(int x);

static int f_static(int x) { return x; };

namespace {
  int i_unnamed;
  int f_unnamed(int x) { return x; };
}

struct A 
{
  int d;
  static int sd;

  int mf(int);
  static int smf(int);
};


namespace static_vars_write_access {

void f1(int x, A& a) 
{
  i_global = x;         // expected-warning-re{{Write to the variable '.*' with static duration}}
  i_global_init = x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
  i_extern = x;         // expected-warning-re{{Write to the variable '.*' with static duration}}
  i_extern_c = x;       // expected-warning-re{{Write to the variable '.*' with static duration}}
  i_static = x;         // expected-warning-re{{Write to the variable '.*' with static duration}}
  i_unnamed = x;        // expected-warning-re{{Write to the variable '.*' with static duration}}
  a.d = x;                                                          
  a.sd = x;             // expected-warning-re{{Write to the variable '.*' with static duration}}
  A::sd = x;            // expected-warning-re{{Write to the variable '.*' with static duration}}
                                                                    
  i_global += x;        // expected-warning-re{{Write to the variable '.*' with static duration}}
  i_global_init += x;   // expected-warning-re{{Write to the variable '.*' with static duration}}
  i_extern += x;        // expected-warning-re{{Write to the variable '.*' with static duration}}
  i_extern_c += x;      // expected-warning-re{{Write to the variable '.*' with static duration}}
  i_static += x;        // expected-warning-re{{Write to the variable '.*' with static duration}}
  i_unnamed += x;       // expected-warning-re{{Write to the variable '.*' with static duration}}
  a.d += x;                                                         
  a.sd += x;            // expected-warning-re{{Write to the variable '.*' with static duration}}
  A::sd += x;           // expected-warning-re{{Write to the variable '.*' with static duration}}
                                                                    
  x = i_global = x;     // expected-warning-re{{Write to the variable '.*' with static duration}}
  x = i_global_init = x;// expected-warning-re{{Write to the variable '.*' with static duration}}
  x = i_extern = x;     // expected-warning-re{{Write to the variable '.*' with static duration}}
  x = i_extern_c = x;   // expected-warning-re{{Write to the variable '.*' with static duration}}
  x = i_static = x;     // expected-warning-re{{Write to the variable '.*' with static duration}}
  x = i_unnamed = x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
  x = a.d = x;                                                      
  x = a.sd = x;         // expected-warning-re{{Write to the variable '.*' with static duration}}
  x = A::sd = x;        // expected-warning-re{{Write to the variable '.*' with static duration}}
}

}

namespace static_vars_read_access {

int f1(int x, A& a) 
{
  x += i_global;       
  x += i_global_init;  
  x += i_extern;       
  x += i_extern_c;     
  x += i_static;       
  x += i_const;        
  x += i_static_const; 
  x += i_unnamed;      
  x += a.d;
  x += a.sd;           
  x += A::sd;          

  return x;
}

}


namespace static_vars_read_access_parameter {

int g(int i);

int f1(int x, A& a) 
{
  x += g(i_global);       
  x += g(i_global_init);  
  x += g(i_extern);       
  x += g(i_extern_c);     
  x += g(i_static);       
  x += g(i_const);        
  x += g(i_static_const); 
  x += g(i_unnamed);      
  x += g(a.d);
  x += g(a.sd);           
  x += g(A::sd);          

  return x;
}

}

namespace static_vars_address_access {

int g(const int* i);

int f1(int x, A& a) 
{
  x += g(&i_global);       
  x += g(&i_global_init);  
  x += g(&i_extern);       
  x += g(&i_extern_c);     
  x += g(&i_static);       
  x += g(&i_const);        
  x += g(&i_static_const); 
  x += g(&i_unnamed);      
  x += g(&a.d);
  x += g(&a.sd);           
  x += g(&A::sd);          

  return x;
}

}

namespace static_vars_member_func {
  
struct A 
{
  int d;
  static int sd;

  int read(int x) 
  {
    x += d;
    x += sd;           
    x += A::sd;        
    x += this->d;
    x += this->sd;     
    x += this->A::sd;  

    return x;
  }
  
  void write(int x) 
  {
    d           += x;                                                         
    sd          += x;   // expected-warning-re{{Write to the variable '.*' with static duration}}
    A::sd       += x;   // expected-warning-re{{Write to the variable '.*' with static duration}}
    this->d     += x;                                                         
    this->sd    += x;   // expected-warning-re{{Write to the variable '.*' with static duration}}
    this->A::sd += x;   // expected-warning-re{{Write to the variable '.*' with static duration}}
  }

};
  
}

namespace static_local_vars {

void f1() 
{
  static int i = 3; 
  static A a;       
}

}

namespace static_functions {

// Although all functions have static duration, we should not
// complained about them.
int f1(int x, A& a) 
{
  x += f_global(x);
  x += f_static(x);
  x += f_unnamed(x);
  x += a.mf(x);
  x += a.smf(x);
  x += A::smf(x);

  return x;
}

}
