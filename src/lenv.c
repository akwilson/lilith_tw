/*
 * Maintains the Lisp Environment -- the function lookup table.
 */

#include "lilith_int.h"

typedef struct env_entry
{
    bool is_builtin;
    lval *value;
} env_entry;

struct lenv
{
    lenv *parent;
    size_t count;
    char **symbols;
    env_entry **values;
};

static bool lenv_put_internal(lenv *e, lval *k, env_entry ee)
{
    // Iterate over all items in environment
    // This is to see if variable already exists
    for (size_t i = 0; i < e->count; i++)
    {
        // If variable is found delete item at that position
        // And replace with variable supplied by user
        if (strcmp(e->symbols[i], k->value.str_val) == 0)
        {
            if (e->values[i]->is_builtin)
            {
                return true;
            }

            lval_del(e->values[i]->value);
            free(e->values[i]);
            env_entry *v = malloc(sizeof(env_entry));
            v->is_builtin = ee.is_builtin;
            v->value = lval_copy(ee.value);
            e->values[i] = v;
            return false;
        }
    }

    // If no existing entry found allocate space for new entry
    e->count++;
    e->values = realloc(e->values, sizeof(env_entry*) * e->count);
    e->symbols = realloc(e->symbols, sizeof(char*) * e->count);

    // Copy contents of lval and symbol string into new location
    env_entry *v = malloc(sizeof(env_entry));
    v->is_builtin = ee.is_builtin;
    v->value = lval_copy(ee.value);
    e->values[e->count - 1] = v;
    
    e->symbols[e->count - 1] = malloc(strlen(k->value.str_val) + 1);
    strcpy(e->symbols[e->count - 1], k->value.str_val);
    return false;
}

lenv *lenv_new()
{
    lenv *rv = malloc(sizeof(lenv));
    rv->parent = 0;
    rv->count = 0;
    rv->symbols = 0;
    rv->values = 0;
    return rv;
}

void lenv_set_parent(lenv *env, lenv *parent)
{
    env->parent = parent;
}

void lenv_del(lenv *e)
{
    for (size_t i = 0; i < e->count; i++)
    {
        free(e->symbols[i]);
        lval_del(e->values[i]->value);
        free(e->values[i]);
    }

    free(e->symbols);
    free(e->values);
    free(e);
}

lval *lenv_get(lenv *e, lval *k)
{
    for (size_t i = 0; i < e->count; i++)
    {
        if (strcmp(e->symbols[i], k->value.str_val) == 0)
        {
            return lval_copy(e->values[i]->value);
        }
    }

    if (e->parent)
    {
        return lenv_get(e->parent, k);
    }

    return lval_error("unbound symbol '%s'", k->value.str_val);
}

bool lenv_put_builtin(lenv *e, lval *k, lval *v)
{
    env_entry ee = { true, v };
    return lenv_put_internal(e, k, ee);
}

bool lenv_put(lenv *e, lval *k, lval *v)
{
    env_entry ee = { false, v };
    return lenv_put_internal(e, k, ee);
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
    rv->count = e->count;
    rv->symbols = malloc(sizeof(char*) * rv->count);
    rv->values = malloc(sizeof(env_entry*) * rv->count);

    for (size_t i = 0; i < rv->count; i++)
    {
        rv->symbols[i] = malloc(strlen(e->symbols[i]) + 1);
        strcpy(rv->symbols[i], e->symbols[i]);

        env_entry *ee = malloc(sizeof(env_entry));
        ee->is_builtin = e->values[i]->is_builtin;
        ee->value = lval_copy(e->values[i]->value);
        rv->values[i] = ee;
    }

    return rv; 
}

lval *lenv_to_lval(lenv *env)
{
    lval *rv = lval_qexpression();
    for (size_t i = 0; i < env->count; i++)
    {
        lval *pair = lval_qexpression();
        lval_add(pair, lval_string(env->symbols[i]));
        lval_add(pair, lval_copy(env->values[i]->value));
        lval_add(rv, pair);
    }

    return rv;
}
