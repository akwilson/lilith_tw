#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "mpc.h"
#include "lilith_int.h"

//#define IOPS $(SUB, -) $(MUL, *) $(DIV, /) $(MOD, %) $(ADD, +) $(AND, &) $(OR, |) $(XOR, ^) $(SR, >>) $(SL, <<)
#define IOPS $(SUB, -) $(MUL, *) $(DIV, /) $(ADD, +)

enum iops_enum
{
#define $(x, op)   IOPSENUM_##x,
  IOPS
  IOPSENUM_COUNT
#undef $
};

static lval *do_calc(enum iops_enum iop, lval *xval, lval *yval)
{
    lval *rv;
    static void *ll_jump_table[] =
    {
#define $(x, op)   &&LL_##x,
        IOPS
#undef $
    };

    goto *(ll_jump_table[iop]);

#define $(x, op) LL_##x:                                            \
    if (xval->type == LVAL_LONG && yval->type == LVAL_LONG)         \
    {                                                               \
        rv = lval_long(xval->value.num_l op yval->value.num_l);     \
    }                                                               \
    else if (xval->type == LVAL_LONG)                               \
    {                                                               \
        rv = lval_double(xval->value.num_l op yval->value.num_d);   \
    }                                                               \
    else if (yval->type == LVAL_LONG)                               \
    {                                                               \
        rv = lval_double(xval->value.num_d op yval->value.num_l);   \
    }                                                               \
    else                                                            \
    {                                                               \
        rv = lval_double(xval->value.num_d op yval->value.num_d);   \
    }                                                               \
    lval_del(xval);                                                 \
    lval_del(yval);                                                 \
    return rv;
    IOPS
#undef $
}

static lval *lval_pop(lval *val, int i)
{
    lval *x = val->value.list.cell[i];

    // Shift memory after the item at "i" over the top
    memmove(&val->value.list.cell[i], &val->value.list.cell[i + 1],
        sizeof(lval*) * (val->value.list.count - i - 1));

    // Decrease the count of items in the list
    val->value.list.count--;

    // Reallocate the memory used
    val->value.list.cell =
        realloc(val->value.list.cell, sizeof(lval*) * val->value.list.count);
    return x;
}

static lval *lval_take(lval *val, int i)
{
    lval *x = lval_pop(val, i);
    lval_del(val);
    return x;
}

static lval *builtin_op(lval *a, const char *op)
{
    // Confirm that all arguments are numeric values
    for (int i = 0; i < a->value.list.count; i++)
    {
        if (a->value.list.cell[i]->type != LVAL_LONG && a->value.list.cell[i]->type != LVAL_DOUBLE)
        {
            lval_del(a);
            return lval_error("Cannot operate on non-numeric value");
        }
    }

    // Get the first value
    lval *x = lval_pop(a, 0);

    // If single arument subtraction, negate value
    if (a->value.list.count == 0 && (strcmp(op, "-") == 0))
    {
        if (x->type == LVAL_LONG)
        {
            x->value.num_l = -x->value.num_l;
        }
        else
        {
            x->value.num_d = -x->value.num_d;
        }
    }

    // While elements remain
    while (a->value.list.count > 0)
    {
        lval *y = lval_pop(a, 0);
        if (strcmp(op, "+") == 0)
        {
            x = do_calc(IOPSENUM_ADD, x, y);
        }
        else if (strcmp(op, "-") == 0)
        {
            x = do_calc(IOPSENUM_SUB, x, y);
        }
        else if (strcmp(op, "*") == 0)
        {
            x = do_calc(IOPSENUM_MUL, x, y);
        }
        else if (strcmp(op, "/") == 0)
        {
            if (y->type == LVAL_LONG ? y->value.num_l == 0 : y->value.num_d == 0.0)
            {
                lval_del(x);
                lval_del(y);
                x = lval_error("Division By Zero");
                break;
            }

            x = do_calc(IOPSENUM_DIV, x, y);
        }
    }

    lval_del(a);
    return x;
}

static lval *lval_eval_sexpr(lval *val)
{
    // Evaluate children
    for (int i = 0; i < val->value.list.count; i++)
    {
        val->value.list.cell[i] = lval_eval(val->value.list.cell[i]);
    }

    // Check for errors
    for (int i = 0; i < val->value.list.count; i++)
    {
        if (val->value.list.cell[i]->type == LVAL_ERROR)
        {
            return lval_take(val, i);
        }
    }

    // Empty expressions
    if (val->value.list.count == 0)
    {
        return val;
    }

    // Single expression
    if (val->value.list.count == 1)
    {
        return lval_take(val, 0);
    }

    // First element must be a symbol
    lval *first = lval_pop(val, 0);
    if (first->type != LVAL_SYMBOL)
    {
        lval_del(first);
        lval_del(val);
        return lval_error("S-expression does not start with symbol");
    }

    // Call built-in with operator
    lval *result = builtin_op(val, first->value.symbol);
    lval_del(first);
    return result;
}

lval *lval_eval(lval *val) {
    // Evaluate Sexpressions
    if (val->type == LVAL_SEXPRESSION)
    {
        return lval_eval_sexpr(val);
    }

    // All other lval types remain the same
    return val;
}
