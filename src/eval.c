/*
 * Functions to evaluate an s-expression. Uses an X macro to generate
 * a computed goto to dispatch the operation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "mpc.h"
#include "lilith_int.h"

/*
 * A macro defining the available operations. The first argument is the goto label; the second the name
 * of the function when both params are longs; the third the function for doubles.
 */
#define IOPS $(SUB, sub_l, sub_d) $(MUL, mul_l, mul_d) $(DIV, div_l, div_d) $(ADD, add_l, add_d) \
    $(POW, powl, pow) $(MAX, max_l, max_d) $(MIN, min_l, min_d)

// Arithmetic operations
static long add_l(long x, long y) { return x + y; }
static double add_d(double x, double y) { return x + y; }
static long sub_l(long x, long y) { return x - y; }
static double sub_d(double x, double y) { return x - y; }
static long mul_l(long x, long y) { return x * y; }
static double mul_d(double x, double y) { return x * y; }
static long div_l(long x, long y) { return x / y; }
static double div_d(double x, double y) { return x / y; }
static long max_l(long x, long y) { return x > y ? x : y; }
static double max_d(double x, double y) { return x > y ? x : y; }
static long min_l(long x, long y) { return x < y ? x : y; }
static double min_d(double x, double y) { return x < y ? x : y; }

/**
 * The available operations. Generated by the X macro.
 */
enum iops_enum
{
#define $(X, LOP, DOP) IOPSENUM_##X,
    IOPS
#undef $
};

/**
 * Performs a calculation for two lvals.
 * 
 * @param iop  the operation to perform
 * @param xval the first argument -- freed in this function
 * @param yval the second argument -- freed in this function
 * @returns    a new lval with the result
 */
static lval *do_calc(enum iops_enum iop, lval *xval, lval *yval)
{
    lval *rv;
    static void *jump_table[] =
    {
#define $(X, LOP, DOP) &&JT_##X,
        IOPS
#undef $
    };

    goto *(jump_table[iop]);

#define $(X, LOP, DOP) JT_##X:                                       \
    if (xval->type == LVAL_LONG && yval->type == LVAL_LONG)          \
    {                                                                \
        rv = lval_long(LOP(xval->value.num_l, yval->value.num_l));   \
    }                                                                \
    else if (xval->type == LVAL_LONG)                                \
    {                                                                \
        rv = lval_double(DOP(xval->value.num_l, yval->value.num_d)); \
    }                                                                \
    else if (yval->type == LVAL_LONG)                                \
    {                                                                \
        rv = lval_double(DOP(xval->value.num_d, yval->value.num_l)); \
    }                                                                \
    else                                                             \
    {                                                                \
        rv = lval_double(DOP(xval->value.num_d, yval->value.num_d)); \
    }                                                                \
    lval_del(xval);                                                  \
    lval_del(yval);                                                  \
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
        else if (strcmp(op, "max") == 0)
        {
            x = do_calc(IOPSENUM_MAX, x, y);
        }
        else if (strcmp(op, "min") == 0)
        {
            x = do_calc(IOPSENUM_MIN, x, y);
        }
        else if (strcmp(op, "^") == 0)
        {
            x = do_calc(IOPSENUM_POW, x, y);
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
