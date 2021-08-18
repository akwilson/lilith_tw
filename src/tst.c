//add name of each function you want to use here:
#define UNARYFPBUILTINS \
    $(acos)  $(acosh)  $(asin)  $(asinh)  $(atan)   $(atanh)  $(cbrt)  $(ceil)  \
    $(cos)   $(erf)    $(erfc)  $(exp)    $(exp10)  $(exp2)   $(expm1) $(fabs)  \
    $(floor) $(gamma)  $(j0)    $(j1)     $(lgamma) $(log)    $(log10) $(log1p) \
    $(log2)  $(logb)   $(pow10) $(round)  $(signbit)          $(significand)  \
    $(sin)   $(sqrt)   $(tan)   $(tgamma) $(trunc)  $(y0)     $(y1)

//now define the $(x) macro for our current use case - defining enums
#define $(x) UFPOP_##x,
enum ufp_enum{ UNARYFPBUILTINS };
#undef $  //undefine the $(x) macro so we can reuse it

//feel free to remove the __builtin_## ... its just an optimization
double op(enum ufp_enum op, double f){
  switch(op){  //now we can use the same macros for our cases
#define $(x) case UFPOP_##x : f = x(f);break;
   UNARYFPBUILTINS
#undef $
  }
  return f;
}

