#include "lilith_int.h"
#include "builtin_symbols.h"

char *lookup_load_file(const char *filename);

#define BUILTIN_SYM_FTS "file->string"

/**
 * Built-in function to load a file into a string.
 */
static lval *builtin_file_to_string(lenv *env, lval *args)
{
    LASSERT_ENV(args, env, BUILTIN_SYM_FTS);
    LASSERT_NO_ERROR(args);
    LASSERT_NUM_ARGS(args, 1, BUILTIN_SYM_FTS);
    LASSERT_TYPE_ARG(args, LVAL_EXPR_FIRST(args), LVAL_STRING, BUILTIN_SYM_FTS);

    lval *rv = lval_string(lookup_load_file(LVAL_EXPR_FIRST(args)->value.str_val));
    lval_del(args);
    return rv;
}

void lenv_add_builtin_os(lenv *e)
{
    lenv_add_builtin(e, BUILTIN_SYM_FTS, builtin_file_to_string);
}
