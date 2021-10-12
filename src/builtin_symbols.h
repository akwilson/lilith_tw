#pragma once

#include "lilith_int.h"

/*
 * Symbols used to bind the built-in functions.
 */

// Core list operations
#define BUILTIN_SYM_DEF "def"
#define BUILTIN_SYM_LIST "list"
#define BUILTIN_SYM_HEAD "head"
#define BUILTIN_SYM_TAIL "tail"
#define BUILTIN_SYM_EVAL "eval"
#define BUILTIN_SYM_JOIN "join"
#define BUILTIN_SYM_LEN "len"
#define BUILTIN_SYM_CONS "cons"
#define BUILTIN_SYM_INIT "init"
#define BUILTIN_SYM_PUT "put"
#define BUILTIN_SYM_LAMBDA "\\"

// Comparison / sequencing
#define BUILTIN_SYM_IF "if"
#define BUILTIN_SYM_EQ "="
#define BUILTIN_SYM_AND "and"
#define BUILTIN_SYM_OR "or"
#define BUILTIN_SYM_NOT "not"

// Utilities
#define BUILTIN_SYM_LOAD "load"
#define BUILTIN_SYM_READ "read"
#define BUILTIN_SYM_ENV "env"
#define BUILTIN_SYM_PRINT "print"
#define BUILTIN_SYM_ERROR "error"

// Type checking
#define BUILTIN_SYM_IS_STRING "string?"
#define BUILTIN_SYM_IS_LONG "number?"
#define BUILTIN_SYM_IS_DOUBLE "decimal?"
#define BUILTIN_SYM_IS_BOOL "boolean?"
#define BUILTIN_SYM_IS_QEXPR "q-expression?"
#define BUILTIN_SYM_IS_SEXPR "s-expression?"

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

#define LASSERT_TYPE_ARG(arg, val, expected, arg_symbol)                                          \
    LASSERT(arg, val->type == expected, "function '%s' type mismatch - expected %s, received %s", \
        arg_symbol, ltype_name(expected), ltype_name(val->type))

/**
 * Utility function to call built-in functions from elsewhere in the code base.
 */
lval *call_builtin(lenv *env, char *symbol, lval *val);

/**
 * Adds a built-in function to the environment with the given name.
 */
void lenv_add_builtin(lenv *env, char *name, lbuiltin func);
