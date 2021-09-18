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

/**
 * Utility function to call built-in functions from elsewhere in the code base.
 */
lval *call_builtin(lenv *env, char *symbol, lval *val);
