/*
 * Built-in functions providing core functionality.
 */

#include <stdarg.h>
#include <math.h>
#include "lilith_int.h"
#include "builtin_symbols.h"

char *lookup_load_file(const char *filename);
static lval *builtin_eval(lenv* env, lval *args);

/**
 * Built-in function for defining new symbols. First argument in val's list
 * is a q-expression with one or more symbols. Additional arguments are values
 * that map to those symbols.
 * 
 * @param env   the environment to add to
 * @param val   q-expression in the first element, symbols in the subsequent elements
 * @param adder pointer to a function to add to the environment
 * @returns     an empty s-expression on success, an error otherwise
 */
static lval *builtin_assign(lenv *env, lval *val, size_t expected, bool (*adder)(lenv*, lval*, lval*))
{
    LASSERT_ENV(val, env, BUILTIN_SYM_DEF);
    LASSERT_NO_ERROR(val);
    LASSERT_TYPE_ARG(val, LVAL_EXPR_FIRST(val), LVAL_QEXPRESSION, BUILTIN_SYM_DEF);

    // First argument is a symbol list
    lval *syms = lval_pop(val);
    for (pair *ptr = syms->value.list.head; ptr; ptr = ptr->next)
    {
        LASSERT(val, ptr->data->type == LVAL_SYMBOL,
            "function '%s' type mismatch - expected %s, received %s",
            BUILTIN_SYM_DEF, ltype_name(LVAL_SYMBOL), ltype_name(ptr->data->type));
    }

    LASSERT(val, LVAL_EXPR_CNT(syms) == expected,
        "function '%s' argument mismatch - %d symbols, %d values",
        BUILTIN_SYM_DEF, LVAL_EXPR_CNT(syms), expected);

    // Assign symbols to values
    size_t i = 0;
    for (pair *ptr = syms->value.list.head; ptr; ptr = ptr->next)
    {
        lval *to_add = lval_pop(val);
        LASSERT(val, !adder(env, ptr->data, to_add),
            "symbol '%s' is a built-in", ptr->data->value.str_val);
        lval_del(to_add);
        i++;
    }

    lval_del(syms);
    return LVAL_EXPR_CNT(val) ? val : lval_sexpression();
}

static lval *builtin_def(lenv *env, lval *val)
{
    lval *rv = builtin_assign(env, val, LVAL_EXPR_CNT(val) - 1, lenv_def);
    if (rv->type != LVAL_ERROR)
    {
        lval_del(val);
    }

    return rv;
}

static lval *builtin_let(lenv *env, lval *val)
{
    lenv *nenv = lenv_new();
    lenv_set_parent(nenv, env);

    lval *rv = builtin_assign(nenv, val, LVAL_EXPR_CNT(val) - 2, lenv_put);
    if (rv->type != LVAL_ERROR)
    {
        // val now contains the final q-expr
        rv = builtin_eval(nenv, val);
    }

    lenv_del(nenv);
    return rv;
}

/**
 * Built-in function to convert an s-expression in to a q-expression.
 */
static lval *builtin_list(lenv *env, lval *args)
{
    LASSERT_ENV(args, env, BUILTIN_SYM_LIST);
    LASSERT_NO_ERROR(args);

    args->type = LVAL_QEXPRESSION;
    return args;
}

static lval *head_qexpr(lval *args)
{
    LASSERT(args, LVAL_EXPR_CNT(LVAL_EXPR_FIRST(args)) != 0, "empty q-expression passed to '%s'", BUILTIN_SYM_HEAD);

    lval *rv = lval_take(args, 0);
    rv = lval_add(lval_qexpression(), lval_take(rv, 0));
    return rv;
}

static lval *head_string(lval *args)
{
    char head[2];
    head[0] = LVAL_EXPR_FIRST(args)->value.str_val[0];
    head[1] = 0;
    lval *rv = lval_string(head);
    lval_del(args);
    return rv;
}

/**
 * Built-in function to return the first element of a q-expression.
 */
static lval *builtin_head(lenv *env, lval *args)
{
    LASSERT_ENV(args, env, BUILTIN_SYM_HEAD);
    LASSERT_NO_ERROR(args);
    LASSERT_NUM_ARGS(args, 1, BUILTIN_SYM_HEAD);
    LASSERT(args, LVAL_EXPR_FIRST(args)->type == LVAL_QEXPRESSION || LVAL_EXPR_FIRST(args)->type == LVAL_STRING,
        "function '%s' type mismatch - expected String or Q-Expression, received %s",
        BUILTIN_SYM_HEAD, ltype_name(LVAL_EXPR_FIRST(args)->type));
    
    if (LVAL_EXPR_FIRST(args)->type == LVAL_QEXPRESSION)
    {
        return head_qexpr(args);
    }
    
    return head_string(args);
}

static lval *tail_string(lval *args)
{
    const char *str = LVAL_EXPR_FIRST(args)->value.str_val;
    if (strlen(str) > 1)
    {
        lval *rv = lval_string(LVAL_EXPR_FIRST(args)->value.str_val + 1);
        lval_del(args);
        return rv;
    }

    return lval_string("");
}

/**
 * Built-in function to return all elements of a q-expression except the first.
 */
static lval *builtin_tail(lenv *env, lval *args)
{
    LASSERT_ENV(args, env, BUILTIN_SYM_TAIL);
    LASSERT_NO_ERROR(args);
    LASSERT_NUM_ARGS(args, 1, BUILTIN_SYM_TAIL);
    LASSERT(args, LVAL_EXPR_FIRST(args)->type == LVAL_QEXPRESSION || LVAL_EXPR_FIRST(args)->type == LVAL_STRING,
        "function '%s' type mismatch - expected String or Q-Expression, received %s",
        BUILTIN_SYM_TAIL, ltype_name(LVAL_EXPR_FIRST(args)->type));

    if (LVAL_EXPR_FIRST(args)->type == LVAL_QEXPRESSION)
    {
        LASSERT(args, LVAL_EXPR_CNT(LVAL_EXPR_FIRST(args)) != 0, "empty q-expression passed to '%s'", BUILTIN_SYM_TAIL);

        lval *rv = lval_take(args, 0);
        lval_del(lval_pop(rv));
        return rv;
    }

    return tail_string(args);
}

/**
 * Built-in function to evaluate a q-expression.
 */
static lval *builtin_eval(lenv* env, lval *args)
{
    LASSERT_ENV(args, env, BUILTIN_SYM_EVAL);
    LASSERT_NO_ERROR(args);
    LASSERT_NUM_ARGS(args, 1, BUILTIN_SYM_EVAL);
    LASSERT_TYPE_ARG(args, LVAL_EXPR_FIRST(args), LVAL_QEXPRESSION, BUILTIN_SYM_EVAL);

    lval *x = lval_take(args, 0);
    x->type = LVAL_SEXPRESSION;
    return lilith_eval_expr(env, x);
}

/**
 * Add all elements of the second q-expression to the first.
 */
static lval *lval_join_qexpr(lval *x, lval* y)
{
    while (LVAL_EXPR_CNT(y))
    {
        x = lval_add(x, lval_pop(y));
    }

    lval_del(y);
    return x;
}

static lval *lval_join_string(lval *x, lval *y)
{
    size_t l = strlen(x->value.str_val) + strlen(y->value.str_val) + 1;
    char str[l];
    sprintf(str, "%s%s", x->value.str_val, y->value.str_val);
    lval_del(x);
    lval_del(y);
    return lval_string(str);
}

/**
 * Built-in function to join q-expressions together.
 */
static lval *builtin_join(lenv *env, lval *args)
{
    LASSERT_ENV(args, env, BUILTIN_SYM_JOIN);
    LASSERT_NO_ERROR(args);

    lval *x = lval_pop(args);
    for (pair *ptr = args->value.list.head; ptr; ptr = ptr->next)
    {
        LASSERT(args, ptr->data->type == LVAL_QEXPRESSION || ptr->data->type == LVAL_STRING,
            "function '%s' type mismatch - expected String or Q-Expression, received %s",
            BUILTIN_SYM_JOIN, ltype_name(ptr->data->type));

        LASSERT(args, x->type == ptr->data->type,
            "function '%s' type mismatch - inconsistent argument types %s vs %s",
            BUILTIN_SYM_JOIN, ltype_name(x->type), ltype_name(ptr->data->type));
    }

    while (LVAL_EXPR_CNT(args))
    {
        if (LVAL_EXPR_FIRST(args)->type == LVAL_QEXPRESSION)
        {
            x = lval_join_qexpr(x, lval_pop(args));
        }
        else
        {
            x = lval_join_string(x, lval_pop(args));
        }
    }

    lval_del(args);
    return x;
}

/**
 * Built-in function to return the number of items in a q-expression.
 */
static lval *builtin_len(lenv *env, lval *args)
{
    LASSERT_ENV(args, env, BUILTIN_SYM_LEN);
    LASSERT_NO_ERROR(args);
    LASSERT_NUM_ARGS(args, 1, BUILTIN_SYM_LEN);
    LASSERT(args, LVAL_EXPR_FIRST(args)->type == LVAL_QEXPRESSION || LVAL_EXPR_FIRST(args)->type == LVAL_STRING,
        "function '%s' type mismatch - expected String or Q-Expression, received %s",
        BUILTIN_SYM_LEN, ltype_name(LVAL_EXPR_FIRST(args)->type));

    lval *x = lval_take(args, 0);
    lval *rv = x->type == LVAL_QEXPRESSION ? lval_long(LVAL_EXPR_CNT(x)) : lval_long(strlen(x->value.str_val));
    lval_del(x);
    return rv;
}

/**
 * Built-in function to add an element to the start of a q-expression.
 */
static lval *builtin_cons(lenv *env, lval *args)
{
    LASSERT_ENV(args, env, BUILTIN_SYM_CONS);
    LASSERT_NO_ERROR(args);
    LASSERT_NUM_ARGS(args, 2, BUILTIN_SYM_CONS);
    LASSERT(args,
        LVAL_EXPR_FIRST(args)->type == LVAL_LONG || LVAL_EXPR_FIRST(args)->type == LVAL_DOUBLE ||
            LVAL_EXPR_FIRST(args)->type == LVAL_BUILTIN_FUN || LVAL_EXPR_FIRST(args)->type == LVAL_USER_FUN,
        "first '%s' parameter should be a value or a function", BUILTIN_SYM_CONS);
    LASSERT(args, lval_expr_item(args, 1)->type == LVAL_QEXPRESSION,
        "second '%s' parameter should be a q-expression", BUILTIN_SYM_CONS);

    lval *rv = lval_qexpression();
    rv = lval_add(rv, lval_pop(args));
    while (LVAL_EXPR_CNT(LVAL_EXPR_FIRST(args)))
    {
        rv = lval_add(rv, lval_pop(LVAL_EXPR_FIRST(args)));
    }
    
    lval_del(args);
    return rv;
}

/**
 * Built-in function to return all elements in a q-expression except the last.
 */
static lval *builtin_init(lenv *env, lval *args)
{
    LASSERT_ENV(args, env, BUILTIN_SYM_INIT);
    LASSERT_NO_ERROR(args);
    LASSERT_NUM_ARGS(args, 1, BUILTIN_SYM_INIT);
    LASSERT_TYPE_ARG(args, LVAL_EXPR_FIRST(args), LVAL_QEXPRESSION, BUILTIN_SYM_INIT);
    LASSERT(args, LVAL_EXPR_CNT(LVAL_EXPR_FIRST(args)) != 0, "empty q-expression passed to '%s'", BUILTIN_SYM_INIT);

    lval *list = lval_take(args, 0);
    lval *rv = lval_qexpression();
    while (LVAL_EXPR_CNT(list) > 1)
    {
        lval_add(rv, lval_pop(list));
    }

    lval_del(args);
    return rv;
}

/**
 * Built-in function to check the structure of a lambda
 * expression and read off the relevant arguments.
 */
static lval *builtin_lambda(lenv *env, lval *args)
{
    LASSERT_ENV(args, env, BUILTIN_SYM_LAMBDA);
    LASSERT_NO_ERROR(args);
    LASSERT_NUM_ARGS(args, 2, BUILTIN_SYM_LAMBDA);
    LASSERT_TYPE_ARG(args, LVAL_EXPR_FIRST(args), LVAL_QEXPRESSION, BUILTIN_SYM_LAMBDA);
    LASSERT_TYPE_ARG(args, args->value.list.head->next->data, LVAL_QEXPRESSION, BUILTIN_SYM_LAMBDA);

    // Check first q-expression contains only symbols
    lval *syms = LVAL_EXPR_FIRST(args);
    for (pair *ptr = syms->value.list.head; ptr; ptr = ptr->next)
    {
        LASSERT(args, ptr->data->type == LVAL_SYMBOL,
            "function '%s' type mismatch - expected %s, received %s",
            BUILTIN_SYM_LAMBDA, ltype_name(LVAL_SYMBOL), ltype_name(ptr->data->type));
    }

    // Pop first two arguments and pass them to lval_lambda
    lval *formals = lval_pop(args);
    lval *body = lval_pop(args);
    lval_del(args);

    return lval_lambda(formals, body);
}

/**
 * Built-in function to evaluate an if expression.
 */
static lval *builtin_if(lenv *env, lval *args)
{
    LASSERT_ENV(args, env, BUILTIN_SYM_IF);
    LASSERT_NO_ERROR(args);
    LASSERT_NUM_ARGS(args, 3, BUILTIN_SYM_IF);
    LASSERT_TYPE_ARG(args, LVAL_EXPR_FIRST(args), LVAL_BOOL, BUILTIN_SYM_IF);
    LASSERT_TYPE_ARG(args, lval_expr_item(args, 1), LVAL_QEXPRESSION, BUILTIN_SYM_IF);
    LASSERT_TYPE_ARG(args, lval_expr_item(args, 2), LVAL_QEXPRESSION, BUILTIN_SYM_IF);

    lval *stmt = lval_pop(args);
    lval *br_true = lval_pop(args);
    lval *br_false = lval_pop(args);

    lval *rv;
    if (stmt->value.bval)
    {
        br_true->type = LVAL_SEXPRESSION;
        rv = lilith_eval_expr(env, br_true);
        lval_del(br_false);
    }
    else
    {
        br_false->type = LVAL_SEXPRESSION;
        rv = lilith_eval_expr(env, br_false);
        lval_del(br_true);
    }

    lval_del(args);
    lval_del(stmt);
    return rv;
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
static lval *builtin_eq(lenv *env, lval *args)
{
    LASSERT_ENV(args, env, BUILTIN_SYM_EQ);
    LASSERT_NO_ERROR(args);

    // Get the first value
    lval *x = lval_pop(args);

    // Confirm that all arguments are the same type
    for (pair *ptr = args->value.list.head; ptr; ptr = ptr->next)
    {
        LASSERT(args, type_check(x, ptr->data),
            "function '%s' type mismatch - inconsistent argument types %s vs %s",
            BUILTIN_SYM_EQ, ltype_name(x->type), ltype_name(ptr->data->type));
    }

    bool rv = true;
    // While elements remain
    while (LVAL_EXPR_CNT(args) > 0)
    {
        lval *y = lval_pop(args);
        if (!lval_is_equal(x, y))
        {
            rv = false;
            lval_del(y);
            break;
        }

        lval_del(y);
    }

    lval_del(args);
    lval_del(x);
    return lval_bool(rv);
}

/**
 * Built-in function to 'and' a series of boolean expressions.
 */
static lval *builtin_and(lenv *env, lval *args)
{
    LASSERT_ENV(args, env, BUILTIN_SYM_AND);
    LASSERT_NO_ERROR(args);

    // Confirm that all arguments are boolean
    for (pair *ptr = args->value.list.head; ptr; ptr = ptr->next)
    {
        LASSERT(args, ptr->data->type == LVAL_BOOL,
            "function '%s' type mismatch - expected %s, received %s",
            BUILTIN_SYM_AND, LVAL_BOOL, ltype_name(ptr->data->type));
    }

    bool rv = true;
    while (LVAL_EXPR_CNT(args) > 0)
    {
        lval *x = lval_pop(args);
        if (!x->value.bval)
        {
            rv = false;
            lval_del(x);
            break;
        }

        lval_del(x);
    }

    lval_del(args);
    return lval_bool(rv);
}

/**
 * Built-in function to 'or' a series of boolean expressions.
 */
static lval *builtin_or(lenv *env, lval *args)
{
    LASSERT_ENV(args, env, BUILTIN_SYM_OR);
    LASSERT_NO_ERROR(args);

    // Confirm that all arguments are boolean
    for (pair *ptr = args->value.list.head; ptr; ptr = ptr->next)
    {
        LASSERT(args, ptr->data->type == LVAL_BOOL,
            "function '%s' type mismatch - expected %s, received %s",
            BUILTIN_SYM_OR, LVAL_BOOL, ltype_name(ptr->data->type));
    }

    bool rv = false;
    while (LVAL_EXPR_CNT(args) > 0)
    {
        lval *x = lval_pop(args);
        if (x->value.bval)
        {
            rv = true;
            lval_del(x);
            break;
        }

        lval_del(x);
    }

    lval_del(args);
    return lval_bool(rv);
}

/**
 * Built-in function to flip a boolean expression.
 */
static lval *builtin_not(lenv *env, lval *args)
{
    LASSERT_ENV(args, env, BUILTIN_SYM_NOT);
    LASSERT_NO_ERROR(args);
    LASSERT_NUM_ARGS(args, 1, BUILTIN_SYM_NOT);
    LASSERT_TYPE_ARG(args, LVAL_EXPR_FIRST(args), LVAL_BOOL, BUILTIN_SYM_NOT);

    lval *x = lval_take(args, 0);
    lval *rv = lval_bool(!x->value.bval);
    lval_del(x);
    return rv;
}

/**
 * Loads and evaluates lilith code from a file. Filename specified in args.
 */
static lval *builtin_load(lenv *env, lval *args)
{
    LASSERT_ENV(args, env, BUILTIN_SYM_LOAD);
    LASSERT_NO_ERROR(args);
    LASSERT_NUM_ARGS(args, 1, BUILTIN_SYM_LOAD);

    lval *rv = 0;
    char *fn = lookup_load_file(LVAL_EXPR_FIRST(args)->value.str_val);
    if (!fn)
    {
        rv = lval_error("File not found %s", LVAL_EXPR_FIRST(args)->value.str_val);
    }
    else
    {
        lval *expr = lilith_read_from_string(fn);
        rv = multi_eval(env, expr);
        free(fn);
    }

    lval_del(args);
    return rv;
}

/**
 * Built-in function to print an lval to the screen.
 */
static lval *builtin_print(lenv *env, lval *args)
{
    LASSERT_ENV(args, env, BUILTIN_SYM_PRINT);
    LASSERT_NO_ERROR(args);

    for (pair *ptr = args->value.list.head; ptr; ptr = ptr->next)
    {
        lval_print(ptr->data, 1);
        putchar(' ');
    }

    putchar('\n');
    lval_del(args);

    return lval_sexpression();
}

/**
 * Built-in function to generate an error message.
 */
static lval *builtin_error(lenv *env, lval *args)
{
    LASSERT_ENV(args, env, BUILTIN_SYM_ERROR);
    LASSERT_NO_ERROR(args);
    lval *err = lval_error(LVAL_EXPR_FIRST(args)->value.str_val);
    lval_del(args);
    return err;
}

/**
 * Reads in a string and converts it to a q-expression.
 */
static lval *builtin_read(lenv *env, lval *args)
{
    LASSERT_ENV(args, env, BUILTIN_SYM_READ);
    LASSERT_NO_ERROR(args);
    LASSERT_NUM_ARGS(args, 1, BUILTIN_SYM_READ);

    lval *expr = lilith_read_from_string(LVAL_EXPR_FIRST(args)->value.str_val);
    if (expr->type == LVAL_ERROR)
    {
        return expr;
    }

    lval *rv = lval_add(lval_qexpression(), expr);
    lval_del(args);
    return rv;
}

/**
 * Built-in function to return the environment in a q-expression.
 */
static lval *builtin_env(lenv *env, lval *args)
{
    LASSERT_ENV(args, env, BUILTIN_SYM_ENV);
    LASSERT_NO_ERROR(args);

    lval_del(args);
    return lenv_to_lval(env);
}

/**
 * Built-in function to handle errors. If the first argument
 * is an error then eval the second expression.
 */
static lval *builtin_try(lenv *env, lval *args)
{
    LASSERT_ENV(args, env, BUILTIN_SYM_TRY);
    LASSERT_NUM_ARGS(args, 2, BUILTIN_SYM_TRY);
    LASSERT_TYPE_ARG(args, args->value.list.head->next->data, LVAL_QEXPRESSION, BUILTIN_SYM_TRY);

    lval *res = lval_pop(args);
    if (res->type == LVAL_ERROR)
    {
        lval_del(res);
        lval *handler = lval_pop(args);
        handler->type = LVAL_SEXPRESSION;
        res = lilith_eval_expr(env, handler);
    }

    lval_del(args);
    return res;
}

/**
 * FUnctions to check the type of an expression.
 */
static lval *check_type(lenv *env, lval *args, unsigned type, const char *fname)
{
    LASSERT_ENV(args, env, fname);
    LASSERT_NO_ERROR(args);
    LASSERT_NUM_ARGS(args, 1, fname);

    lval *rv = lval_bool(LVAL_EXPR_FIRST(args)->type == type);
    lval_del(args);
    return rv;
}

static lval *builtin_is_string(lenv *env, lval *args)
{
    return check_type(env, args, LVAL_STRING, BUILTIN_SYM_IS_STRING);
}

static lval *builtin_is_long(lenv *env, lval *args)
{
    return check_type(env, args, LVAL_LONG, BUILTIN_SYM_IS_LONG);
}

static lval *builtin_is_double(lenv *env, lval *args)
{
    return check_type(env, args, LVAL_DOUBLE, BUILTIN_SYM_IS_DOUBLE);
}

static lval *builtin_is_bool(lenv *env, lval *args)
{
    return check_type(env, args, LVAL_BOOL, BUILTIN_SYM_IS_BOOL);
}

static lval *builtin_is_qexpr(lenv *env, lval *args)
{
    return check_type(env, args, LVAL_QEXPRESSION, BUILTIN_SYM_IS_QEXPR);
}

static lval *builtin_is_sexpr(lenv *env, lval *args)
{
    return check_type(env, args, LVAL_SEXPRESSION, BUILTIN_SYM_IS_SEXPR);
}

void lenv_add_builtin(lenv *env, char *name, lbuiltin func)
{
    lval *k = lval_symbol(name);
    lval *v = lval_fun(func);

    lenv_put(env, k, v);
    lval_del(k);
    lval_del(v);
}

lval *call_builtin(lenv *env, char *symbol, lval *args)
{
    lval *k = lval_symbol(symbol);
    lval *f = lenv_get(env, k);
    lval_del(k);
    lval *rv = f->value.builtin(env, args);
    lval_del(f);
    return rv;
}

void lenv_add_builtin_core(lenv *e)
{
    lenv_add_builtin(e, BUILTIN_SYM_DEF, builtin_def);
    lenv_add_builtin(e, BUILTIN_SYM_LET, builtin_let);
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
    lenv_add_builtin(e, BUILTIN_SYM_TRY, builtin_try);
    lenv_add_builtin(e, BUILTIN_SYM_IS_STRING, builtin_is_string);
    lenv_add_builtin(e, BUILTIN_SYM_IS_LONG, builtin_is_long);
    lenv_add_builtin(e, BUILTIN_SYM_IS_DOUBLE, builtin_is_double);
    lenv_add_builtin(e, BUILTIN_SYM_IS_BOOL, builtin_is_bool);
    lenv_add_builtin(e, BUILTIN_SYM_IS_QEXPR, builtin_is_qexpr);
    lenv_add_builtin(e, BUILTIN_SYM_IS_SEXPR, builtin_is_sexpr);
}

void lilith_eval_file(lenv *env, const char *filename)
{
    lval *args = lval_add(lval_sexpression(), lval_string(filename));
    lval *x = builtin_load(env, args);
    if (x->type == LVAL_ERROR)
    {
        lilith_println(x);
    }

    lval_del(x);
}
