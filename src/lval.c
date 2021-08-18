#include <stdio.h>
#include <lilith_int.h>

lval lval_long(long num)
{
    lval rv;
    rv.type = LVAL_LONG;
    rv.value.num_l = num;
    return rv;
}

lval lval_double(double num)
{
    lval rv;
    rv.type = LVAL_DOUBLE;
    rv.value.num_d = num;
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
    case LVAL_LONG:
        printf("%li", v.value.num_l);
        break;
    case LVAL_DOUBLE:
        printf("%f", v.value.num_d);
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

