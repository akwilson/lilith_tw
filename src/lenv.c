/*
 * Maintains the Lisp Environment -- the function lookup table.
 */

#include <stdio.h>
#include <stdlib.h>
#include "lilith_int.h"

typedef struct env_entry
{
    bool is_builtin;
    lval *value;
} env_entry;

struct lenv
{
    lenv *parent;
    int count;
    char **symbols;
    env_entry **values;
};

static bool lenv_put_internal(lenv *e, lval *k, env_entry ee)
{
    // Iterate over all items in environment
    // This is to see if variable already exists
    for (int i = 0; i < e->count; i++)
    {
        // If variable is found delete item at that position
        // And replace with variable supplied by user
        if (strcmp(e->symbols[i], k->value.symbol) == 0)
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
    
    e->symbols[e->count - 1] = malloc(strlen(k->value.symbol) + 1);
    strcpy(e->symbols[e->count - 1], k->value.symbol);
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
    for (int i = 0; i < e->count; i++)
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
    for (int i = 0; i < e->count; i++)
    {
        if (strcmp(e->symbols[i], k->value.symbol) == 0)
        {
            return lval_copy(e->values[i]->value);
        }
    }

    if (e->parent)
    {
        return lenv_get(e->parent, k);
    }

    return lval_error("unbound symbol '%s'", k->value.symbol);
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

void lenv_print(lenv *e)
{
    for (int i = 0; i < e->count; i++)
    {
        printf("%s : ", e->symbols[i]);
        lval_print(e->values[i]->value);
        putchar('\n');
    }
}

lenv *lenv_copy(lenv *e)
{
    lenv *rv = malloc(sizeof(lenv));
    rv->parent = e->parent;
    rv->count = e->count;
    rv->symbols = malloc(sizeof(char*) * rv->count);
    rv->values = malloc(sizeof(env_entry*) * rv->count);

    for (int i = 0; i < rv->count; i++)
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
