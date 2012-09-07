// RUN: %clang_cc1  -analyze -analyzer-checker=alpha.StaticVariableAccessChecker -verify %s

//==============================================================================
// Objects with static duration
//==============================================================================

struct C
{
  C(int i=0) : d(i) {}
  
  int d;
  static int sd;
};

C o_global;

C o_global_init = C(3);

extern C o_extern;

extern "C" C o_extern_c;

static C o_static = C(2);

const C o_const = C(4);

static const C o_static_const = C(5);

namespace {
C o_unnamed;
}


namespace static_objects_read_access {

int f1(int x, int* p) 
{
  x += o_global.d;        
  x += o_global_init.d;   
  x += o_extern.d;        
  x += o_extern_c.d;      
  x += o_static.d;        
  x += o_const.d;         
  x += o_static_const.d;  
  x += o_unnamed.d;       

  x += o_global.sd;       
  x += o_global_init.sd;  
  x += o_extern.sd;       
  x += o_extern_c.sd;     
  x += o_static.sd;       
  x += o_const.sd;        
  x += o_static_const.sd; 
  x += o_unnamed.sd;      

  // These are all read access!!!
  p[o_global.d] = x;      
  p[o_global_init.d] = x; 
  p[o_extern.d] = x;      
  p[o_extern_c.d] = x;    
  p[o_static.d] = x;      
  p[o_unnamed.d] = x;     

  p[o_global.sd] = x;     
  p[o_global_init.sd] = x;
  p[o_extern.sd] = x;     
  p[o_extern_c.sd] = x;   
  p[o_static.sd] = x;     
  p[o_unnamed.sd] = x;    

  return x;
}

}

namespace static_objects_write_access {

void f1(int x) 
{
  o_global.d = x;         // expected-warning-re{{Write to the variable '.*' with static duration}}
  o_global_init.d = x;    // expected-warning-re{{Write to the variable '.*' with static duration}}
  o_extern.d = x;         // expected-warning-re{{Write to the variable '.*' with static duration}}
  o_extern_c.d = x;       // expected-warning-re{{Write to the variable '.*' with static duration}}
  o_static.d = x;         // expected-warning-re{{Write to the variable '.*' with static duration}}
  o_unnamed.d = x;        // expected-warning-re{{Write to the variable '.*' with static duration}}
                                                                      
  o_global.sd = x;        // expected-warning-re{{Write to the variable '.*' with static duration}} 
  o_global_init.sd = x;   // expected-warning-re{{Write to the variable '.*' with static duration}}
  o_extern.sd = x;        // expected-warning-re{{Write to the variable '.*' with static duration}}
  o_extern_c.sd = x;      // expected-warning-re{{Write to the variable '.*' with static duration}}
  o_static.sd = x;        // expected-warning-re{{Write to the variable '.*' with static duration}}
  o_unnamed.sd = x;       // expected-warning-re{{Write to the variable '.*' with static duration}}
}

}

namespace static_objects_read_access_parameter {

int g1(int i);

int f1(int x) 
{
  x += g1(o_global.d);        
  x += g1(o_global_init.d);   
  x += g1(o_extern.d);        
  x += g1(o_extern_c.d);      
  x += g1(o_static.d);        
  x += g1(o_const.d);         
  x += g1(o_static_const.d);  
  x += g1(o_unnamed.d);       

  x += g1(o_global.sd);        
  x += g1(o_global_init.sd);   
  x += g1(o_extern.sd);        
  x += g1(o_extern_c.sd);      
  x += g1(o_static.sd);        
  x += g1(o_const.sd);         
  x += g1(o_static_const.sd);  
  x += g1(o_unnamed.sd);       

  return x;
}

}

namespace static_objects_address_access {

int g(const int* i);

int f1(int x) 
{
  x += g(&o_global.d);        
  x += g(&o_global_init.d);   
  x += g(&o_extern.d);        
  x += g(&o_extern_c.d);      
  x += g(&o_static.d);        
  x += g(&o_const.d);         
  x += g(&o_static_const.d);  
  x += g(&o_unnamed.d);       

  x += g(&o_global.sd);       
  x += g(&o_global_init.sd);  
  x += g(&o_extern.sd);       
  x += g(&o_extern_c.sd);     
  x += g(&o_static.sd);       
  x += g(&o_const.sd);        
  x += g(&o_static_const.sd); 
  x += g(&o_unnamed.sd);      

  return x;
}

}
