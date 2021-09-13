/*
 * Functions to evaluate an s-expression.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mpc.h"
#include "lilith_int.h"
#include "builtin_symbols.h"

/**
 * Calls a function. Binds each parameter to its environment and evaluates
 * the function with that environment. If too few arguments are passed it
 * returns a new, partially evaluated function.
 * 
 * @param env  the top-level environment
 * @param func the function to call
 * @param args the arguments to pass to the function
 * @returns    a result, or a partially evaluated function
 */
static lval *lval_call(lenv *env, lval *func, lval *args)
{
    if (func->type == LVAL_BUILTIN_FUN)
    {
        return func->value.builtin(env, args);
    }

    // Argument counts
    int given = LVAL_EXPR_CNT(args);
    int expected = LVAL_EXPR_CNT(func->value.user_fun.formals);

    // Bind the parameters to the formal symbols
    while (LVAL_EXPR_CNT(args))
    {
        if (LVAL_EXPR_CNT(func->value.user_fun.formals) == 0)
        {
            lval_del(args);
            return lval_error("Too many aruments passed to function - expected %d, received %d",
                    expected, given);
        }

        // Consume arguments -- bind argument to formal
        lval *sym = lval_pop(func->value.user_fun.formals, 0);

        // Handle special case & - bind varargs as a q-expression
        if (strcmp(sym->value.symbol, "&") == 0)
        {
            if (LVAL_EXPR_CNT(func->value.user_fun.formals) != 1)
            {
                lval_del(args);
                return lval_error("function format invalid - symbol '&' not followed by single symbol");
            }

            // Next formal should be bound to remaining arguments
            lval *nsym = lval_pop(func->value.user_fun.formals, 0);
            lenv_put(func->value.user_fun.env, nsym, call_builtin(env, BUILTIN_SYM_LIST, args));
            lval_del(nsym);
            lval_del(sym);
            break;
        }

        lval *param = lval_pop(args, 0);
        lenv_put(func->value.user_fun.env, sym, param);

        lval_del(sym);
        lval_del(param);
    }

    // Argument list os bound so can be cleaned up
    lval_del(args);

    // If '&' remains in formal list bind to empty list
    if (LVAL_EXPR_CNT(func->value.user_fun.formals) > 0 &&
        strcmp(LVAL_EXPR_ITEM(func->value.user_fun.formals, 0)->value.symbol, "&") == 0)
    {
        // Check to ensure that & is not passed invalidly
        if (LVAL_EXPR_CNT(func->value.user_fun.formals) != 2)
        {
            return lval_error("function format invalid - symbol '&' not followed by single symbol");
        }

        // Pop and delete '&' symbol
        lval_del(lval_pop(func->value.user_fun.formals, 0));

        // Pop next symbol and create empty list
        lval *sym = lval_pop(func->value.user_fun.formals, 0);
        lval *val = lval_qexpression();

        // Bind to environment and delete
        lenv_put(func->value.user_fun.env, sym, val);
        lval_del(sym);
        lval_del(val);
    }

    if (LVAL_EXPR_CNT(func->value.user_fun.formals) == 0)
    {
        // All arguments are bound so call function
        lenv_set_parent(func->value.user_fun.env, env);
        return call_builtin(func->value.user_fun.env, BUILTIN_SYM_EVAL,
                            lval_add(lval_sexpression(), lval_copy(func->value.user_fun.body)));
    }

    /*
     * Return the partially evaluated function. At this stage all of the passed-in params
     * have been bound to the function's local environment and the corresponding formals
     * have been removed.
     */
    return lval_copy(func);
}

static lval *lval_eval_sexpr(lenv *env, lval *val)
{
    // Evaluate children
    for (int i = 0; i < LVAL_EXPR_CNT(val); i++)
    {
        LVAL_EXPR_ITEM(val, i) = lval_eval(env, LVAL_EXPR_ITEM(val, i));
    }

    // Check for errors
    for (int i = 0; i < LVAL_EXPR_CNT(val); i++)
    {
        if (LVAL_EXPR_ITEM(val, i)->type == LVAL_ERROR)
        {
            return lval_take(val, i);
        }
    }

    // Empty expressions
    if (LVAL_EXPR_CNT(val) == 0)
    {
        return val;
    }

    // Single expression
    if (LVAL_EXPR_CNT(val) == 1 && (LVAL_EXPR_ITEM(val, 0)->type != LVAL_BUILTIN_FUN))
    {
        return lval_take(val, 0);
    }

    // First element must be a function
    lval *first = lval_pop(val, 0);
    if (first->type != LVAL_BUILTIN_FUN && first->type != LVAL_USER_FUN)
    {
        lval_del(first);
        lval_del(val);
        return lval_error("s-expression does not start with function, '%s'", ltype_name(first->type));
    }

    // Call function
    lval *result = lval_call(env, first, val);
    lval_del(first);
    return result;
}

lval *lval_eval(lenv *env, lval *val)
{
    // Lookup the function and return
    if (val->type == LVAL_SYMBOL)
    {
        lval *x = lenv_get(env, val);
        lval_del(val);
        return x;
    }

    // Evaluate Sexpressions
    if (val->type == LVAL_SEXPRESSION)
    {
        return lval_eval_sexpr(env, val);
    }

    // All other lval types remain the same
    return val;
}
