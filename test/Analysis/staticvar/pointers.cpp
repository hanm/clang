// RUN: %clang_cc1  -analyze -analyzer-checker=alpha.StaticVariableAccessChecker -verify %s

//==============================================================================
// Pointers with static duration
//==============================================================================

// First, a whole bunch of variables with static duration are
// declared. 

int array[] = {0,1,2,3};
  
int *p_global;

int *p_global_init = array;

extern int *p_extern;

extern "C" int *p_extern_c;

static int *p_static = array;

const int *p_const = array;

static const int *p_static_const = array;

namespace {
  int *p_unnamed;
}

struct C
{
  int *d;
  static int *sd;
};

namespace static_pointers_write_access {

 void f1(int x, C* c) 
{
  *p_global = x;         // expected-warning-re{{Write to the variable '.*' with static duration}}
  *p_global_init = x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
  *p_extern = x;         // expected-warning-re{{Write to the variable '.*' with static duration}}
  *p_extern_c = x;       // expected-warning-re{{Write to the variable '.*' with static duration}}
  *p_static = x;         // expected-warning-re{{Write to the variable '.*' with static duration}}
  *p_unnamed = x;        // expected-warning-re{{Write to the variable '.*' with static duration}}
  *c->d = x;                                                         
  *c->sd = x;            // expected-warning-re{{Write to the variable '.*' with static duration}}
  *C::sd = x;            // expected-warning-re{{Write to the variable '.*' with static duration}}
}

}

namespace static_pointers_read_access {

 int f1(int x, C* c, int* p) 
{
  x += *p_global;        
  x += *p_global_init;   
  x += *p_extern;        
  x += *p_extern_c;      
  x += *p_static;        
  x += *p_const;         
  x += *p_static_const;  
  x += *p_unnamed;       
  x += *c->d;
  x += *c->sd;           
  x += *C::sd;           

  // These are all read access!!!
  p[*p_global] = x;      
  p[*p_global_init] = x; 
  p[*p_extern] = x;      
  p[*p_extern_c] = x;    
  p[*p_static] = x;      
  p[*p_unnamed] = x;     
  p[*c->d] = x;              
  p[*c->sd] = x;         
  p[*C::sd] = x;         

  return x;
}

}

namespace static_pointers_read_access_parameter {

 int g1(int i);
 int g2(const int* i);

 int f1(int x, C* c) 
{
  x += g1(*p_global);        
  x += g1(*p_global_init);   
  x += g1(*p_extern);        
  x += g1(*p_extern_c);      
  x += g1(*p_static);        
  x += g1(*p_const);         
  x += g1(*p_static_const);  
  x += g1(*p_unnamed);       
  x += g1(*c->d);
  x += g1(*c->sd);           
  x += g1(*C::sd);           

  x += g2(p_global);        
  x += g2(p_global_init);   
  x += g2(p_extern);        
  x += g2(p_extern_c);      
  x += g2(p_static);        
  x += g2(p_const);         
  x += g2(p_static_const);  
  x += g2(p_unnamed);       
  x += g2(c->d);
  x += g2(c->sd);           
  x += g2(C::sd);           

  return x;
}

}

namespace static_pointers_address_access {

 int g(const int* const* i);

 int f1(int x, C* c) 
{
  x += g(&p_global);        
  x += g(&p_global_init);   
  x += g(&p_extern);        
  x += g(&p_extern_c);      
  x += g(&p_static);        
  x += g(&p_const);         
  x += g(&p_static_const);  
  x += g(&p_unnamed);       
  x += g(&c->d);
  x += g(&c->sd);           
  x += g(&C::sd);           

  return x;
}

}

