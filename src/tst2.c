#include <stdio.h>

#define IOPS $(SUB,-) $(MUL,*) $(DIV,/) $(MOD,%) $(ADD,+) $(AND,&) $(OR,|) \
  $(XOR,^) $(SR,>>) $(SL,<<)

enum iops_enum {
#define $(x,op)   IOPSENUM_##x,
  IOPS
  IOPSENUM_COUNT
#undef $
};
int opi(int a, enum iops_enum b, int c){
  static void* array[] = { //you may get better results with short or int
//#define $(x,op)   &&x - &&ADD,
#define $(x,op)   &&AKW_##x,
    IOPS
#undef $
  };
  if (b >= IOPSENUM_COUNT) return a;
  goto *(array[b]);
  //else should give a warning here.
#define $(x,op)   AKW_##x: return a op c;
  IOPS
#undef $
}

int main()
{
    printf("%d\n", opi(10, IOPSENUM_SUB, 8));
    return 0;
}
