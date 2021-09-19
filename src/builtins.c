/*
 * Built-in functions providing core functionality. Uses an X macro to generate
 * a computed goto to dispatch the operation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lilith_int.h"
#include "builtin_symbols.h"

char *lookup_load_file(const char *filename);
extern mpc_parser_t *lilith_p;

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
#define IOPS $(SUB, sub_l, sub_d, "-") $(MUL, mul_l, mul_d, "*") $(DIV, div_l, div_d, "/")      \
    $(ADD, add_l, add_d, "+") $(POW, pow_l, pow_d, "^") $(MAX, max_l, max_d, "max")             \
    $(MIN, min_l, min_d, "min") $(MOD, mod_l, mod_d, "%") $(GT, gt, gt, ">") $(LT, lt, lt, "<") \
    $(GTE, gte, gte, ">=") $(LTE, lte, lte, "<=")

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

/**
 * Check two types for equality.
 */
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
        if (!lval_is_equal(x, lval_pop(val, 0)))
        {
            rv = false;
            break;
        }
    }

    lval_del(val);
    return lval_bool(rv);
}

/**
 * Built-in function to 'and' a series of boolean expressions.
 */
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

/**
 * Built-in function to 'or' a series of boolean expressions.
 */
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

/**
 * Built-in function to flip a boolean expression.
 */
static lval *builtin_not(lenv *env, lval *val)
{
    LASSERT_ENV(val, env, BUILTIN_SYM_NOT);
    LASSERT_NUM_ARGS(val, 1, BUILTIN_SYM_NOT);
    LASSERT_TYPE_ARG(val, 0, LVAL_BOOL, BUILTIN_SYM_NOT);

    lval *x = lval_take(val, 0);
    return lval_bool(!x->value.bval);
}

static lval *head_qexpr(lval *val)
{
    LASSERT(val, LVAL_EXPR_CNT(LVAL_EXPR_ITEM(val, 0)) != 0, "empty q-expression passed to '%s'", BUILTIN_SYM_HEAD);

    lval *rv = lval_take(val, 0);
    while (rv->value.list.count > 1)
    {
        lval_del(lval_pop(rv, 1));
    }

    return rv;
}

static lval *head_string(lval *val)
{
    char head[2];
    head[0] = LVAL_EXPR_ITEM(val, 0)->value.string[0];
    head[1] = 0;
    lval *rv = lval_string(head);
    lval_del(val);
    return rv;
}

/**
 * Built-in function to return the first element of a q-expression.
 */
static lval *builtin_head(lenv *env, lval *val)
{
    LASSERT_ENV(val, env, BUILTIN_SYM_HEAD);
    LASSERT_NUM_ARGS(val, 1, BUILTIN_SYM_HEAD);
    LASSERT(val, LVAL_EXPR_ITEM(val, 0)->type == LVAL_QEXPRESSION || LVAL_EXPR_ITEM(val, 0)->type == LVAL_STRING,
        "function '%s' type mismatch - expected String or Q-Expression, received %s",
        BUILTIN_SYM_HEAD, ltype_name(LVAL_EXPR_ITEM(val, 0)->type));
    
    if (LVAL_EXPR_ITEM(val, 0)->type == LVAL_QEXPRESSION)
    {
        return head_qexpr(val);
    }
    
    return head_string(val);
}

static lval *tail_string(lval *val)
{
    const char *str = LVAL_EXPR_ITEM(val, 0)->value.string;
    if (strlen(str) > 1)
    {
        lval *rv = lval_string(LVAL_EXPR_ITEM(val, 0)->value.string + 1);
        lval_del(val);
        return rv;
    }

    return lval_string("");
}

/**
 * Built-in function to return all elements of a q-expression except the first.
 */
static lval *builtin_tail(lenv *env, lval *val)
{
    LASSERT_ENV(val, env, BUILTIN_SYM_TAIL);
    LASSERT_NUM_ARGS(val, 1, BUILTIN_SYM_TAIL);
    LASSERT(val, LVAL_EXPR_ITEM(val, 0)->type == LVAL_QEXPRESSION || LVAL_EXPR_ITEM(val, 0)->type == LVAL_STRING,
        "function '%s' type mismatch - expected String or Q-Expression, received %s",
        BUILTIN_SYM_TAIL, ltype_name(LVAL_EXPR_ITEM(val, 0)->type));

    if (LVAL_EXPR_ITEM(val, 0)->type == LVAL_QEXPRESSION)
    {
        LASSERT(val, LVAL_EXPR_CNT(LVAL_EXPR_ITEM(val, 0)) != 0, "empty q-expression passed to '%s'", BUILTIN_SYM_TAIL);

        lval *rv = lval_take(val, 0);
        lval_del(lval_pop(rv, 0));
        return rv;
    }

    return tail_string(val);
}

/**
 * Add all elements of the second q-expression to the first.
 */
static lval *lval_join_qexpr(lval *x, lval* y)
{
    while (LVAL_EXPR_CNT(y))
    {
        x = lval_add(x, lval_pop(y, 0));
    }

    lval_del(y);
    return x;
}

static lval *lval_join_string(lval *x, lval *y)
{
    int l = strlen(x->value.string) + strlen(y->value.string) + 1;
    char str[l];
    sprintf(str, "%s%s", x->value.string, y->value.string);
    return lval_string(str);
}

/**
 * Built-in function to join q-expressions together.
 */
static lval *builtin_join(lenv *env, lval *val)
{
    LASSERT_ENV(val, env, BUILTIN_SYM_JOIN);

    lval *x = lval_pop(val, 0);
    for (int i = 0; i < LVAL_EXPR_CNT(val); i++)
    {
        LASSERT(val, LVAL_EXPR_ITEM(val, i)->type == LVAL_QEXPRESSION || LVAL_EXPR_ITEM(val, i)->type == LVAL_STRING,
            "function '%s' type mismatch - expected String or Q-Expression, received %s",
            BUILTIN_SYM_JOIN, ltype_name(LVAL_EXPR_ITEM(val, i)->type));

        LASSERT(val, x->type == LVAL_EXPR_ITEM(val, i)->type,
            "function '%s' type mismatch - inconsistent argument types %s vs %s",
            BUILTIN_SYM_JOIN, ltype_name(x->type), ltype_name(LVAL_EXPR_ITEM(val, i)->type));
    }

    while (LVAL_EXPR_CNT(val))
    {
        if (LVAL_EXPR_ITEM(val, 0)->type == LVAL_QEXPRESSION)
        {
            x = lval_join_qexpr(x, lval_pop(val, 0));
        }
        else
        {
            x = lval_join_string(x, lval_pop(val, 0));
        }
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
    LASSERT(val, LVAL_EXPR_ITEM(val, 0)->type == LVAL_QEXPRESSION || LVAL_EXPR_ITEM(val, 0)->type == LVAL_STRING,
        "function '%s' type mismatch - expected String or Q-Expression, received %s",
        BUILTIN_SYM_LEN, ltype_name(LVAL_EXPR_ITEM(val, 0)->type));

    lval *x = lval_take(val, 0);
    if (x->type == LVAL_QEXPRESSION)
    {
        return lval_long(LVAL_EXPR_CNT(x));
    }

    return lval_long(strlen(x->value.string));
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

/**
 * Evaluates all of the expressions in a parsed result.
 */
static void parser_eval(lenv *env, const mpc_result_t* result)
{
    // Read contents of file
    lval *expr = lval_read(result->output);
    mpc_ast_delete(result->output);

    // Evaluate each expression
    while (LVAL_EXPR_CNT(expr))
    {
        lval *x = lval_eval(env, lval_pop(expr, 0));
        if (x->type == LVAL_ERROR)
        {
            lval_println(x);
        }

        lval_del(x);
    }

    // Delete expressions and arguments
    lval_del(expr);
}

/**
 * Handle parse errors -- return as an lval_error().
 */
static lval *parser_error(const mpc_result_t* result, const char *symbol)
{
    char *err_msg = mpc_err_string(result->error);
    mpc_err_delete(result->error);

    lval *err = lval_error("function '%s' failed %s", symbol, err_msg);
    free(err_msg);
    return err;
}

/**
 * Loads and evaluates lilith code from a file. Filename specified in val.
 */
static lval *builtin_load(lenv *env, lval *val)
{
    LASSERT_ENV(val, env, BUILTIN_SYM_LOAD);
    LASSERT_NUM_ARGS(val, 1, BUILTIN_SYM_LOAD);

    lval *rv;
    char *fn = lookup_load_file(LVAL_EXPR_ITEM(val, 0)->value.string);
    if (!fn)
    {
        rv = lval_error("File not found %s", LVAL_EXPR_ITEM(val, 0)->value.string);
    }
    else
    {
        mpc_result_t result;
        if (mpc_parse_contents(fn, lilith_p, &result))
        {
            parser_eval(env, &result);
            rv = lval_sexpression();
        }
        else
        {
            rv = parser_error(&result, BUILTIN_SYM_LOAD);
        }
    }

    free(fn);
    lval_del(val);
    return rv;
}

/**
 * Reads in a string and converts it to a q-expression.
 */
static lval *builtin_read(lenv *env, lval *val)
{
    LASSERT_ENV(val, env, BUILTIN_SYM_READ);
    LASSERT_NUM_ARGS(val, 1, BUILTIN_SYM_READ);

    mpc_result_t result;
    if (mpc_parse("<string>", LVAL_EXPR_ITEM(val, 0)->value.string, lilith_p, &result))
    {
        lval *rv = lval_add(lval_qexpression(), lval_read(result.output));
        lval_del(val);
        return rv;
    }
    else
    {
        lval *err = parser_error(&result, BUILTIN_SYM_READ);
        lval_del(val);
        return err;
    }
}

/**
 * Built-in function to print an lval to the screen.
 */
static lval *builtin_print(lenv *env, lval *val)
{
    LASSERT_ENV(val, env, BUILTIN_SYM_PRINT);

    for (int i = 0; i < LVAL_EXPR_CNT(val); i++)
    {
        lval_print(LVAL_EXPR_ITEM(val, i));
        putchar(' ');
    }

    putchar('\n');
    lval_del(val);

    return lval_sexpression();
}

/**
 * Built-in function to generate an error message.
 */
static lval *builtin_error(lenv *env, lval *val)
{
    LASSERT_ENV(val, env, BUILTIN_SYM_ERROR);
    
    lval *err = lval_error(LVAL_EXPR_ITEM(val, 0)->value.string);

    /* Delete arguments and return */
    lval_del(val);
    return err;
}

/**
 * Built-in function to return the environment in a q-expression.
 */
static lval *builtin_env(lenv *env, lval *val)
{
    LASSERT_ENV(val, env, BUILTIN_SYM_ENV);

    lval_del(val);
    return lenv_to_lval(env);
}

static lval *check_type(lenv *env, lval *val, int type, const char *fname)
{
    LASSERT_ENV(val, env, fname);
    LASSERT_NUM_ARGS(val, 1, fname);

    lval *rv = lval_bool(LVAL_EXPR_ITEM(val, 0)->type == type);
    lval_del(val);
    return rv;
}

static lval *builtin_is_string(lenv *env, lval *val)
{
    return check_type(env, val, LVAL_STRING, BUILTIN_SYM_IS_STRING);
}

static lval *builtin_is_long(lenv *env, lval *val)
{
    return check_type(env, val, LVAL_LONG, BUILTIN_SYM_IS_LONG);
}

static lval *builtin_is_double(lenv *env, lval *val)
{
    return check_type(env, val, LVAL_DOUBLE, BUILTIN_SYM_IS_DOUBLE);
}

static lval *builtin_is_bool(lenv *env, lval *val)
{
    return check_type(env, val, LVAL_BOOL, BUILTIN_SYM_IS_BOOL);
}

static lval *builtin_is_qexpr(lenv *env, lval *val)
{
    return check_type(env, val, LVAL_QEXPRESSION, BUILTIN_SYM_IS_QEXPR);
}

static lval *builtin_is_sexpr(lenv *env, lval *val)
{
    return check_type(env, val, LVAL_SEXPRESSION, BUILTIN_SYM_IS_SEXPR);
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
    lenv_add_builtin(e, BUILTIN_SYM_LOAD, builtin_load);
    lenv_add_builtin(e, BUILTIN_SYM_PRINT, builtin_print);
    lenv_add_builtin(e, BUILTIN_SYM_ERROR, builtin_error);
    lenv_add_builtin(e, BUILTIN_SYM_READ, builtin_read);
    lenv_add_builtin(e, BUILTIN_SYM_ENV, builtin_env);
    lenv_add_builtin(e, BUILTIN_SYM_IS_STRING, builtin_is_string);
    lenv_add_builtin(e, BUILTIN_SYM_IS_LONG, builtin_is_long);
    lenv_add_builtin(e, BUILTIN_SYM_IS_DOUBLE, builtin_is_double);
    lenv_add_builtin(e, BUILTIN_SYM_IS_BOOL, builtin_is_bool);
    lenv_add_builtin(e, BUILTIN_SYM_IS_QEXPR, builtin_is_qexpr);
    lenv_add_builtin(e, BUILTIN_SYM_IS_SEXPR, builtin_is_sexpr);
}
