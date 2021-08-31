/*
 * Built-in functions providing core functionality.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lilith_int.h"
#include "builtin_symbols.h"

/*
 * Error checking macros.
 */
#define LASSERT(args, cond, fmt, ...)                   \
    do {                                                \
        if (!(cond))                                    \
        {                                               \
            lval *err = lval_error(fmt, ##__VA_ARGS__); \
            lval_del(args);                             \
            return err;                                 \
        }                                               \
    } while (0)

#define LASSERT_ENV(arg, arg_env, arg_symbol) \
    LASSERT(arg, arg_env != 0, "environment not set for '%s'", arg_symbol)

#define LASSERT_NUM_ARGS(arg, expected, arg_symbol)       \
    LASSERT(arg, LVAL_EXPR_CNT(arg) == expected,          \
        "function '%s' expects %d argument, received %d", \
        arg_symbol, expected, LVAL_EXPR_CNT(arg))

#define LASSERT_TYPE_ARG(arg, idx, expected, arg_symbol)          \
    LASSERT(arg, LVAL_EXPR_ITEM(arg, idx)->type == expected,      \
        "function '%s' type mismatch - expected %s, received %s", \
        arg_symbol, ltype_name(expected), ltype_name(LVAL_EXPR_ITEM(arg, idx)->type))

/*
 * A macro defining the available operations. The first argument is the goto label; the second the name
 * of the function when both params are longs; the third the function for doubles.
 */
#define IOPS $(SUB, sub_l, sub_d, "-") $(MUL, mul_l, mul_d, "*") $(DIV, div_l, div_d, "/") $(ADD, add_l, add_d, "+") \
    $(POW, pow_l, pow_d, "^") $(MAX, max_l, max_d, "max") $(MIN, min_l, min_d, "min") $(MOD, mod_l, mod_d, "%") \
    $(GT, gt, gt, ">") $(LT, lt, lt, "<") $(GTE, gte, gte, ">=") $(LTE, lte, lte, "<=")

// Arithmetic operations
static lval *add_l(long x, long y) { return lval_long(x + y); }
static lval *add_d(double x, double y) { return lval_double(x + y); }
static lval *sub_l(long x, long y) { return lval_long(x - y); }
static lval *sub_d(double x, double y) { return lval_double(x - y); }
static lval *mul_l(long x, long y) { return lval_long(x * y); }
static lval *mul_d(double x, double y) { return lval_double(x * y); }
static lval *div_l(long x, long y) { return y == 0 ? lval_error("divide by zero") : lval_long(x / y); }
static lval *div_d(double x, double y) { return y == 0.0 ? lval_error("divide by zero") : lval_double(x / y); }
static lval *max_l(long x, long y) { return lval_long(x > y ? x : y); }
static lval *max_d(double x, double y) { return lval_double(x > y ? x : y); }
static lval *min_l(long x, long y) { return lval_long(x < y ? x : y); }
static lval *min_d(double x, double y) { return lval_double(x < y ? x : y); }
static lval *mod_l(long x, long y) { return lval_long(x % y); }
static lval *mod_d(double x, double y) { return lval_double(fmod(x, y)); }
static lval *pow_l(long x, long y) { return lval_long(powl(x, y)); }
static lval *pow_d(double x, double y) { return lval_double(pow(x, y)); }
static lval *gt(double x, double y) { return lval_bool(x > y); }
static lval *lt(double x, double y) { return lval_bool(x < y); }
static lval *gte(double x, double y) { return lval_bool(x >= y); }
static lval *lte(double x, double y) { return lval_bool(x <= y); }

/**
 * The available operations. Generated by the X macro.
 */
enum iops_enum
{
#define $(X, LOP, DOP, SYM) IOPSENUM_##X,
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
#define $(X, LOP, DOP, SYM) &&JT_##X,
        IOPS
#undef $
    };

    goto *(jump_table[iop]);

#define $(X, LOP, DOP, SYM) JT_##X:                         \
    if (xval->type == LVAL_LONG && yval->type == LVAL_LONG) \
    {                                                       \
        rv = LOP(xval->value.num_l, yval->value.num_l);     \
    }                                                       \
    else if (xval->type == LVAL_LONG)                       \
    {                                                       \
        rv = DOP(xval->value.num_l, yval->value.num_d);     \
    }                                                       \
    else if (yval->type == LVAL_LONG)                       \
    {                                                       \
        rv = DOP(xval->value.num_d, yval->value.num_l);     \
    }                                                       \
    else                                                    \
    {                                                       \
        rv = DOP(xval->value.num_d, yval->value.num_d);     \
    }                                                       \
    lval_del(xval);                                         \
    lval_del(yval);                                         \
    return rv;
    IOPS
#undef $
}

static bool is_equal(lval *x, lval *y)
{
    if (x->type != y->type)
    {
        if (x->type == LVAL_LONG && y->type == LVAL_DOUBLE)
        {
            return x->value.num_l == y->value.num_d;
        }
        else if (x->type == LVAL_DOUBLE && y->type == LVAL_LONG)
        {
            return x->value.num_d == y->value.num_l;
        }

        return 0;
    }

    switch (x->type)
    {
    case LVAL_LONG:
        return x->value.num_l == y->value.num_l;
    case LVAL_DOUBLE:
        return x->value.num_d == y->value.num_d;
    case LVAL_BOOL:
        return x->value.bval == y->value.bval;
    case LVAL_ERROR:
        return (strcmp(x->value.error, y->value.error) == 0);
    case LVAL_SYMBOL:
        return (strcmp(x->value.symbol, y->value.symbol) == 0);
    case LVAL_BUILTIN_FUN:
        return x->value.builtin == y->value.builtin;
    case LVAL_USER_FUN:
        return is_equal(x->value.user_fun.formals, y->value.user_fun.formals) &&
            is_equal(x->value.user_fun.body, y->value.user_fun.body);
    case LVAL_QEXPRESSION:
    case LVAL_SEXPRESSION:
        if (LVAL_EXPR_CNT(x) != LVAL_EXPR_CNT(y))
        {
            return false;
        }

        for (int i = 0; i < LVAL_EXPR_CNT(x); i++)
        {
            if (!is_equal(LVAL_EXPR_ITEM(x, i), LVAL_EXPR_ITEM(y, i)))
            {
                return 0;
            }
        }
        return true;
    }

    return false; 
}

static bool type_check(lval *x, lval *y)
{
    if (x->type == LVAL_LONG || x->type == LVAL_DOUBLE)
    {
        return y->type == LVAL_LONG || y->type == LVAL_DOUBLE;
    }

    return x->type == y->type;
}

/**
 * Built-in function to compare for equality.
 */
static lval *builtin_eq(lenv *env, lval *val)
{
    LASSERT_ENV(val, env, BUILTIN_SYM_EQ);

    // Get the first value
    lval *x = lval_pop(val, 0);

    // Confirm that all arguments are the same type
    for (int i = 0; i < LVAL_EXPR_CNT(val); i++)
    {
        LASSERT(val, type_check(x, LVAL_EXPR_ITEM(val, i)),
            "function '%s' type mismatch - inconsistent argument types %s vs %s",
            BUILTIN_SYM_EQ, ltype_name(x->type), ltype_name(LVAL_EXPR_ITEM(val, i)->type));
    }

    bool rv = true;
    // While elements remain
    while (LVAL_EXPR_CNT(val) > 0)
    {
        if (!is_equal(x, lval_pop(val, 0)))
        {
            rv = false;
            break;
        }
    }

    lval_del(val);
    return lval_bool(rv);
}

static lval *builtin_and(lenv *env, lval *val)
{
    LASSERT_ENV(val, env, BUILTIN_SYM_EQ);

    // Confirm that all arguments are boolean
    for (int i = 0; i < LVAL_EXPR_CNT(val); i++)
    {
        LASSERT(val, LVAL_EXPR_ITEM(val, i)->type == LVAL_BOOL,
            "function '%s' type mismatch - expected %s, received %s",
            BUILTIN_SYM_AND, LVAL_BOOL, ltype_name(LVAL_EXPR_ITEM(val, i)->type));
    }

    bool rv = true;
    while (LVAL_EXPR_CNT(val) > 0)
    {
        lval *x = lval_pop(val, 0);
        if (!x->value.bval)
        {
            rv = false;
            break;
        }
    }

    lval_del(val);
    return lval_bool(rv);
}

static lval *builtin_or(lenv *env, lval *val)
{
    LASSERT_ENV(val, env, BUILTIN_SYM_EQ);

    // Confirm that all arguments are boolean
    for (int i = 0; i < LVAL_EXPR_CNT(val); i++)
    {
        LASSERT(val, LVAL_EXPR_ITEM(val, i)->type == LVAL_BOOL,
            "function '%s' type mismatch - expected %s, received %s",
            BUILTIN_SYM_OR, LVAL_BOOL, ltype_name(LVAL_EXPR_ITEM(val, i)->type));
    }

    bool rv = false;
    while (LVAL_EXPR_CNT(val) > 0)
    {
        lval *x = lval_pop(val, 0);
        if (x->value.bval)
        {
            rv = true;
            break;
        }
    }

    lval_del(val);
    return lval_bool(rv);
}

static lval *builtin_not(lenv *env, lval *val)
{
    LASSERT_ENV(val, env, BUILTIN_SYM_NOT);
    LASSERT_NUM_ARGS(val, 1, BUILTIN_SYM_NOT);
    LASSERT_TYPE_ARG(val, 0, LVAL_BOOL, BUILTIN_SYM_NOT);

    lval *x = lval_take(val, 0);
    return lval_bool(!x->value.bval);
}

/**
 * Built-in function to return the first element of a q-expression.
 */
static lval *builtin_head(lenv *env, lval *val)
{
    LASSERT_ENV(val, env, BUILTIN_SYM_HEAD);
    LASSERT_NUM_ARGS(val, 1, BUILTIN_SYM_HEAD);
    LASSERT_TYPE_ARG(val, 0, LVAL_QEXPRESSION, BUILTIN_SYM_HEAD);
    LASSERT(val, LVAL_EXPR_CNT(LVAL_EXPR_ITEM(val, 0)) != 0, "empty q-expression passed to '%s'", BUILTIN_SYM_HEAD);

    lval *rv = lval_take(val, 0);
    while (rv->value.list.count > 1)
    {
        lval_del(lval_pop(rv, 1));
    }

    return rv;
}

/**
 * Built-in function to return all elements of a q-expression except the first.
 */
static lval *builtin_tail(lenv *env, lval *val)
{
    LASSERT_ENV(val, env, BUILTIN_SYM_TAIL);
    LASSERT_NUM_ARGS(val, 1, BUILTIN_SYM_TAIL);
    LASSERT_TYPE_ARG(val, 0, LVAL_QEXPRESSION, BUILTIN_SYM_TAIL);
    LASSERT(val, LVAL_EXPR_CNT(LVAL_EXPR_ITEM(val, 0)) != 0, "empty q-expression passed to '%s'", BUILTIN_SYM_TAIL);

    lval *rv = lval_take(val, 0);
    lval_del(lval_pop(rv, 0));
    return rv;
}

/**
 * Add all elements of the second q-expression to the first.
 */
static lval *lval_join(lval *x, lval* y)
{
    while (LVAL_EXPR_CNT(y))
    {
        x = lval_add(x, lval_pop(y, 0));
    }

    lval_del(y);
    return x;
}

/**
 * Built-in function to join q-expressions together.
 */
static lval *builtin_join(lenv *env, lval *val)
{
    LASSERT_ENV(val, env, BUILTIN_SYM_JOIN);

    for (int i = 0; i < LVAL_EXPR_CNT(val); i++)
    {
        LASSERT_TYPE_ARG(val, i, LVAL_QEXPRESSION, BUILTIN_SYM_JOIN);
    }

    lval *x = lval_pop(val, 0);
    while (LVAL_EXPR_CNT(val))
    {
        x = lval_join(x, lval_pop(val, 0));
    }

    lval_del(val);
    return x;
}

/**
 * Built-in function to convert an s-expression in to a q-expression.
 */
static lval *builtin_list(lenv *env, lval *val)
{
    LASSERT_ENV(val, env, BUILTIN_SYM_LIST);
    val->type = LVAL_QEXPRESSION;
    return val;
}

/**
 * Built-in function to evaluate a q-expression.
 */
static lval *builtin_eval(lenv* env, lval *val)
{
    LASSERT_ENV(val, env, BUILTIN_SYM_EVAL);
    LASSERT_NUM_ARGS(val, 1, BUILTIN_SYM_EVAL);
    LASSERT_TYPE_ARG(val, 0, LVAL_QEXPRESSION, BUILTIN_SYM_EVAL);

    lval *x = lval_take(val, 0);
    x->type = LVAL_SEXPRESSION;
    return lval_eval(env, x);
}

/**
 * Built-in function to return the number of items in a q-expression.
 */
static lval *builtin_len(lenv *env, lval *val)
{
    LASSERT_ENV(val, env, BUILTIN_SYM_LEN);
    LASSERT_NUM_ARGS(val, 1, BUILTIN_SYM_LEN);
    LASSERT_TYPE_ARG(val, 0, LVAL_QEXPRESSION, BUILTIN_SYM_LEN);

    lval *x = lval_take(val, 0);
    return lval_long(LVAL_EXPR_CNT(x));
}

/**
 * Built-in function to add an element to the start of a q-expression.
 */
static lval *builtin_cons(lenv *env, lval *val)
{
    LASSERT_ENV(val, env, BUILTIN_SYM_CONS);
    LASSERT_NUM_ARGS(val, 2, BUILTIN_SYM_CONS);
    LASSERT(val,
        LVAL_EXPR_ITEM(val, 0)->type == LVAL_LONG || LVAL_EXPR_ITEM(val, 0)->type == LVAL_DOUBLE ||
            LVAL_EXPR_ITEM(val, 0)->type == LVAL_BUILTIN_FUN || LVAL_EXPR_ITEM(val, 0)->type == LVAL_USER_FUN,
        "first '%s' parameter should be a value or a function", BUILTIN_SYM_CONS);
    LASSERT(val, LVAL_EXPR_ITEM(val, 1)->type == LVAL_QEXPRESSION, "second '%s' parameter should be a q-expression", BUILTIN_SYM_CONS);

    lval *rv = lval_qexpression();
    rv = lval_add(rv, lval_pop(val, 0));
    while (LVAL_EXPR_CNT(LVAL_EXPR_ITEM(val, 1)))
    {
        rv = lval_add(rv, lval_pop(LVAL_EXPR_ITEM(val, 1), 0));
    }
    
    lval_del(val);
    return rv;
}

/**
 * Built-in function to return all elements in a q-expression except the first.
 */
static lval *builtin_init(lenv *env, lval *val)
{
    LASSERT_ENV(val, env, BUILTIN_SYM_INIT);
    LASSERT_NUM_ARGS(val, 1, BUILTIN_SYM_INIT);
    LASSERT_TYPE_ARG(val, 0, LVAL_QEXPRESSION, BUILTIN_SYM_INIT);
    LASSERT(val, LVAL_EXPR_CNT(LVAL_EXPR_ITEM(val, 0)) != 0, "empty q-expression passed to '%s'", BUILTIN_SYM_INIT);

    lval *rv = lval_take(val, 0);
    lval_del(lval_pop(rv, LVAL_EXPR_CNT(rv) - 1));
    return rv;
}

/**
 * Built-in function for defining new symbols. First argument in val's list
 * is a q-expression with one or more symbols. Additional arguments are values
 * that map to those symbols.
 * 
 * @param env       the environment to add to
 * @param val       q-expression in the first element, symbols in the subsequent elements
 * @param to_parent if true assign to the parent environment
 * @returns         an empty s-expression on success, an error otherwise
 */
static lval *builtin_assign(lenv *env, lval *val, bool (*adder)(lenv*, lval*, lval*))
{
    LASSERT_ENV(val, env, BUILTIN_SYM_DEF);
    LASSERT_TYPE_ARG(val, 0, LVAL_QEXPRESSION, BUILTIN_SYM_DEF);

    // First argument is a symbol list
    lval *syms = LVAL_EXPR_ITEM(val, 0);
    for (int i = 0; i < LVAL_EXPR_CNT(syms); i++)
    {
        LASSERT(val, LVAL_EXPR_ITEM(syms, i)->type == LVAL_SYMBOL,
            "function '%s' type mismatch - expected %s, received %s",
            BUILTIN_SYM_DEF, ltype_name(LVAL_SYMBOL), ltype_name(LVAL_EXPR_ITEM(syms, i)->type));
    }

    LASSERT(val, LVAL_EXPR_CNT(syms) == LVAL_EXPR_CNT(val) - 1,
        "function '%s' argument mismatch - %d symbols, %d values",
        BUILTIN_SYM_DEF, LVAL_EXPR_CNT(syms), LVAL_EXPR_CNT(val) - 1);
    
    // Assign symbols to values
    for (int i = 0; i < LVAL_EXPR_CNT(syms); i++)
    {
        LASSERT(val, !adder(env, LVAL_EXPR_ITEM(syms, i), LVAL_EXPR_ITEM(val, i + 1)),
            "function '%s' is a built-in", LVAL_EXPR_ITEM(syms, i)->value.symbol);
    }

    lval_del(val);
    return lval_sexpression();
}

static lval *builtin_def(lenv *env, lval *val)
{
    return builtin_assign(env, val, lenv_def);
}

static lval *builtin_put(lenv *env, lval *val)
{
    return builtin_assign(env, val, lenv_put);
}

/**
 * Built-in function to check the structure of a lambda
 * expression and read off the relevant arguments.
 */
static lval *builtin_lambda(lenv *env, lval *val)
{
    LASSERT_ENV(val, env, BUILTIN_SYM_LAMBDA);
    LASSERT_NUM_ARGS(val, 2, BUILTIN_SYM_LAMBDA);
    LASSERT_TYPE_ARG(val, 0, LVAL_QEXPRESSION, BUILTIN_SYM_LAMBDA);
    LASSERT_TYPE_ARG(val, 1, LVAL_QEXPRESSION, BUILTIN_SYM_LAMBDA);

    // Check first q-expression contains only symbols
    lval *syms = LVAL_EXPR_ITEM(val, 0);
    for (int i = 0; i < LVAL_EXPR_CNT(syms); i++)
    {
        LASSERT(val, LVAL_EXPR_ITEM(syms, i)->type == LVAL_SYMBOL,
            "function '%s' type mismatch - expected %s, received %s",
            BUILTIN_SYM_LAMBDA, ltype_name(LVAL_SYMBOL), ltype_name(LVAL_EXPR_ITEM(syms, i)->type));
    }

    // Pop first two arguments and pass them to lval_lambda
    lval *formals = lval_pop(val, 0);
    lval *body = lval_pop(val, 0);
    lval_del(val);

    return lval_lambda(formals, body);
}

static lval *builtin_op(lenv *env, lval *a, const char* symbol, enum iops_enum iop)
{
    LASSERT_ENV(a, env, symbol);

    // Confirm that all arguments are numeric values
    for (int i = 0; i < LVAL_EXPR_CNT(a); i++)
    {
        LASSERT(a, LVAL_EXPR_ITEM(a, i)->type == LVAL_LONG || LVAL_EXPR_ITEM(a, i)->type == LVAL_DOUBLE,
            "function '%s' type mismatch - expected numeric, received %s",
            symbol, ltype_name(LVAL_EXPR_ITEM(a, i)->type));
    }

    // Get the first value
    lval *x = lval_pop(a, 0);

    // If single arument subtraction, negate value
    if (LVAL_EXPR_CNT(a) == 0 && (iop == IOPSENUM_SUB))
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
    while (LVAL_EXPR_CNT(a) > 0)
    {
        lval *y = lval_pop(a, 0);
        x = do_calc(iop, x, y);
    }

    lval_del(a);
    return x;
}

// Create functions to call builtin_op() for the supported arithmetic operators
#define $(X, LOP, DOP, SYM) static lval *builtin_##X(lenv *env, lval *val) { return builtin_op(env, val, SYM, IOPSENUM_##X); }
    IOPS
#undef $

/**
 * Built-in function to evaluate an if expression.
 */
static lval *builtin_if(lenv *env, lval *val)
{
    LASSERT_ENV(val, env, BUILTIN_SYM_IF);
    LASSERT_NUM_ARGS(val, 3, BUILTIN_SYM_IF);
    LASSERT_TYPE_ARG(val, 0, LVAL_BOOL, BUILTIN_SYM_IF);
    LASSERT_TYPE_ARG(val, 1, LVAL_QEXPRESSION, BUILTIN_SYM_IF);
    LASSERT_TYPE_ARG(val, 2, LVAL_QEXPRESSION, BUILTIN_SYM_IF);

    lval *stmt = lval_pop(val, 0);
    lval *br_true = lval_pop(val, 0);
    lval *br_false = lval_pop(val, 0);

    lval *rv;
    if (stmt->value.bval)
    {
        br_true->type = LVAL_SEXPRESSION;
        rv = lval_eval(env, br_true);
    }
    else
    {
        br_false->type = LVAL_SEXPRESSION;
        rv = lval_eval(env, br_false);
    }

    lval_del(val);
    return rv;
}

static void lenv_add_builtin(lenv *env, char *name, lbuiltin func)
{
    lval *k = lval_symbol(name);
    lval *v = lval_fun(func);

    lenv_put_builtin(env, k, v);
    lval_del(k);
    lval_del(v);
}

lval *call_builtin(lenv *env, char *symbol, lval *val)
{
    lval *k = lval_symbol(symbol);
    lval *f = lenv_get(env, k);
    lval_del(k);
    return f->value.builtin(env, val);
}

void lenv_add_builtins(lenv *e)
{
#define $(X, LOP, DOP, SYM) lenv_add_builtin(e, SYM, builtin_##X);
    IOPS
#undef $

    lenv_add_builtin(e, BUILTIN_SYM_DEF, builtin_def);
    lenv_add_builtin(e, BUILTIN_SYM_PUT, builtin_put);
    lenv_add_builtin(e, BUILTIN_SYM_LIST, builtin_list);
    lenv_add_builtin(e, BUILTIN_SYM_HEAD, builtin_head);
    lenv_add_builtin(e, BUILTIN_SYM_TAIL, builtin_tail);
    lenv_add_builtin(e, BUILTIN_SYM_EVAL, builtin_eval);
    lenv_add_builtin(e, BUILTIN_SYM_JOIN, builtin_join);
    lenv_add_builtin(e, BUILTIN_SYM_LEN, builtin_len);
    lenv_add_builtin(e, BUILTIN_SYM_CONS, builtin_cons);
    lenv_add_builtin(e, BUILTIN_SYM_INIT, builtin_init);
    lenv_add_builtin(e, BUILTIN_SYM_LAMBDA, builtin_lambda);
    lenv_add_builtin(e, BUILTIN_SYM_IF, builtin_if);
    lenv_add_builtin(e, BUILTIN_SYM_EQ, builtin_eq);
    lenv_add_builtin(e, BUILTIN_SYM_AND, builtin_and);
    lenv_add_builtin(e, BUILTIN_SYM_OR, builtin_or);
    lenv_add_builtin(e, BUILTIN_SYM_NOT, builtin_not);
}
