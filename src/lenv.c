/*
 * Maintains the Lisp Environment -- the function lookup table.
 */

#include <collections.h>
#include "lilith_int.h"

#ifdef __linux
extern char _stdlib_llth_start;
#else
extern char stdlib_llth_start;
#endif

struct lenv
{
    lenv *parent;
    void *table;
};

/**
 * Loads the statically linked Lilith standard library in to the environment.
 */
static lval *load_std_lib(lenv *env)
{
#ifdef __linux
    char *stdlib = &_stdlib_llth_start;
#else
    char *stdlib = &stdlib_llth_start;
#endif

    lval *expr = lilith_read_from_string(stdlib);
    return multi_eval(env, expr);
}

lenv *lenv_new()
{
    lenv *rv = calloc(1, sizeof(lenv));
    rv->table = hash_table(31);
    return rv;
}

void lenv_set_parent(lenv *env, lenv *parent)
{
    env->parent = parent;
}

void lenv_del(lenv *e)
{
    void *iter = clxns_iter_new(e->table);
    while (clxns_iter_move_next(iter))
    {
        kvp *val = clxns_iter_get_next(iter);
        free(val->key);
        lval_del(val->value);
    }

    clxns_iter_free(iter);
    clxns_free(e->table, 0);
    free(e);
}

lval *lenv_get(lenv *e, lval *k)
{
    lval *rv;
    if (hash_table_get(e->table, k->value.str_val, (void**)&rv) == C_OK)
    {
        return lval_copy(rv);
    }

    if (e->parent)
    {
        return lenv_get(e->parent, k);
    }

    return lval_error("unbound symbol '%s'", k->value.str_val);
}

bool lenv_put(lenv *e, lval *k, lval *v)
{
    lval *ptr;
    if (hash_table_get(e->table, k->value.str_val, (void**)&ptr) == C_OK && ptr->type == LVAL_BUILTIN_FUN)
    {
        return true;
    }

    hash_table_add(e->table, strdup(k->value.str_val), lval_copy(v));
    return false;
}

bool lenv_def(lenv *e, lval *k, lval *v)
{
    while (e->parent)
    {
        e = e->parent;
    }

    return lenv_put(e, k, v);
}

lenv *lenv_copy(lenv *e)
{
    lenv *rv = malloc(sizeof(lenv));
    rv->parent = e->parent;
    rv->table = hash_table(clxns_count(e->table));

    void *iter = clxns_iter_new(e->table);
    while (clxns_iter_move_next(iter))
    {
        kvp *val = clxns_iter_get_next(iter);
        hash_table_add(rv->table, strdup(val->key), lval_copy(val->value));
    }

    clxns_iter_free(iter);
    return rv; 
}

lval *lenv_to_lval(lenv *env)
{
    lval *rv = lval_qexpression();

    void *iter = clxns_iter_new(env->table);
    while (clxns_iter_move_next(iter))
    {
        kvp *val = clxns_iter_get_next(iter);
        lval *pair = lval_qexpression();
        lval_add(pair, lval_string(val->key));
        lval_add(pair, lval_copy(val->value));
        lval_add(rv, pair);
    }

    return rv;
}

lenv *lilith_init()
{
    lenv *env = lenv_new();
    lenv_add_builtins_sums(env);
    lenv_add_builtins_funcs(env);

    lval *x = load_std_lib(env);
    if (x->type == LVAL_ERROR)
    {
        lilith_println(x);
        return 0;
    }

    lval_del(x);
    return env;
}

void lilith_cleanup(lenv *env)
{
    lenv_del(env);
}
