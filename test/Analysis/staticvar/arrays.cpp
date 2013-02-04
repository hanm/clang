// RUN: %clang_cc1  -analyze -analyzer-checker=alpha.StaticVariableAccessChecker -verify %s

int a_global[4];

int a_global_init[4] = {0,1,2,3};

extern int a_extern[4];

extern "C" int a_extern_c[4];

static int a_static[4] = {0,1,2,3};

const int a_const[4] = {0,1,2,3};

static const int a_static_const[4] = {0,1,2,3};

namespace {
  int a_unnamed[4];
}

struct C
{
  int d[4];
  static int sd[4];
};

namespace static_arrays_write_access {

 void f1(int x, C& c) 
{
  a_global[3] = x;         // expected-warning-re{{Write to the variable '.*' with static duration}}
  a_global_init[3] = x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
  a_extern[3] = x;         // expected-warning-re{{Write to the variable '.*' with static duration}}
  a_extern_c[3] = x;       // expected-warning-re{{Write to the variable '.*' with static duration}}
  a_static[3] = x;         // expected-warning-re{{Write to the variable '.*' with static duration}}
  a_unnamed[3] = x;        // expected-warning-re{{Write to the variable '.*' with static duration}}
  c.d[3] = x;                                                          
  c.sd[3] = x;             // expected-warning-re{{Write to the variable '.*' with static duration}}
  C::sd[3] = x;            // expected-warning-re{{Write to the variable '.*' with static duration}}
}

}

// should not warn for read only access
namespace static_arrays_read_access {

int f1(int x, C& c, int a[4]) 
{
  x += a_global[2];        
  x += a_global_init[2];   
  x += a_extern[2];        
  x += a_extern_c[2];      
  x += a_static[2];        
  x += a_const[2];         
  x += a_static_const[2];  
  x += a_unnamed[2];       
  x += c.d[2];
  x += c.sd[2];            
  x += C::sd[2];           

  a[a_global[3]] = x;      
  a[a_global_init[3]] = x; 
  a[a_extern[3]] = x;      
  a[a_extern_c[3]] = x;    
  a[a_static[3]] = x;      
  a[a_unnamed[3]] = x;     
  a[c.d[3]] = x;           
  a[c.sd[3]] = x;          
  a[C::sd[3]] = x;         

  return x;
}

}

namespace static_arrays_read_access_parameter {

int g1(const int a[4]);
int g2(int i);

int f1(int x, C& c) 
{
  x += g1(a_global);        
  x += g1(a_global_init);   
  x += g1(a_extern);        
  x += g1(a_extern_c);      
  x += g1(a_static);        
  x += g1(a_const);         
  x += g1(a_static_const);  
  x += g1(a_unnamed);       
  x += g1(c.d);
  x += g1(c.sd);            
  x += g1(C::sd);           

  x += g2(a_global[1]);        
  x += g2(a_global_init[1]);   
  x += g2(a_extern[1]);        
  x += g2(a_extern_c[1]);      
  x += g2(a_static[1]);        
  x += g2(a_const[1]);         
  x += g2(a_static_const[1]);  
  x += g2(a_unnamed[1]);       
  x += g2(c.d[1]);
  x += g2(c.sd[1]);            
  x += g2(C::sd[1]);           

  return x;
}

}

namespace static_arrays_address_access {

int g(const int* i);

int f1(int x, C& c) 
{
  x += g(&a_global[0]);        
  x += g(&a_global_init[0]);   
  x += g(&a_extern[0]);        
  x += g(&a_extern_c[0]);      
  x += g(&a_static[0]);        
  x += g(&a_const[0]);         
  x += g(&a_static_const[0]);  
  x += g(&a_unnamed[0]);       
  x += g(&c.d[0]);
  x += g(&c.sd[0]);            
  x += g(&C::sd[0]);           

  return x;
}

}

namespace static_local_arrays {

void f1() 
{
  static int a[4] = {0,1,2,3};  
  static C c[4]; 
}

}

