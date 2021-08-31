#pragma once

#include "lilith_int.h"

// Symbols used to bind the built-in functions.
#define BUILTIN_SYM_DEF "def"
#define BUILTIN_SYM_LIST "list"
#define BUILTIN_SYM_HEAD "head"
#define BUILTIN_SYM_TAIL "tail"
#define BUILTIN_SYM_EVAL "eval"
#define BUILTIN_SYM_JOIN "join"
#define BUILTIN_SYM_LEN "len"
#define BUILTIN_SYM_CONS "cons"
#define BUILTIN_SYM_INIT "init"
#define BUILTIN_SYM_LAMBDA "\\"
#define BUILTIN_SYM_PUT "let"
#define BUILTIN_SYM_IF "if"
#define BUILTIN_SYM_EQ "="
#define BUILTIN_SYM_AND "and"
#define BUILTIN_SYM_OR "or"
#define BUILTIN_SYM_NOT "not"

/**
 * Utility function to call built-in functions from elsewhere in the code base.
 */
lval *call_builtin(lenv *env, char *symbol, lval *val);
