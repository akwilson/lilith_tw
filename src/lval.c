#include <stdio.h>
#include <lilith_int.h>

lval lval_num(long num)
{
    lval rv;
    rv.type = LVAL_NUM;
    rv.value.num = num;
    return rv;
}

lval lval_error(int error)
{
    lval rv;
    rv.type = LVAL_ERROR;
    rv.value.error = error;
    return rv;
}

void lval_print(lval v)
{
    switch (v.type)
    {
    case LVAL_NUM:
        printf("%li", v.value.num);
        break;
    case LVAL_ERROR:
        if (v.value.error == LERR_DIV_ZERO)
        {
            printf("Error: Division By Zero");
        }
        else if (v.value.error == LERR_BAD_OP)
        {
            printf("Error: Invalid Operator");
        }
        else if (v.value.error == LERR_BAD_NUM)
        {
            printf("Error: Invalid Number");
        }
        break;
    }
}

void lval_println(lval v)
{
    lval_print(v);
    putchar('\n');
}

