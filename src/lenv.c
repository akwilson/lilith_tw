/*
 * Maintains the Lisp Environment -- the function lookup table.
 */

#include <stdio.h>
#include <stdlib.h>
#include "lilith_int.h"

struct lenv
{
    int count;
    char **symbols;
    lval **values;
};

lenv *lenv_new()
{
    lenv *rv = malloc(sizeof(lenv));
    rv->count = 0;
    rv->symbols = 0;
    rv->values = 0;
    return rv;
}

void lenv_del(lenv *e)
{
    for (int i = 0; i < e->count; i++)
    {
        free(e->symbols[i]);
        lval_del(e->values[i]);
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
            return lval_copy(e->values[i]);
        }
    }

    return lval_error("unbound symbol '%s'", k->value.symbol);
}

void lenv_put(lenv *e, lval *k, lval *v)
{
    // Iterate over all items in environment
    // This is to see if variable already exists
    for (int i = 0; i < e->count; i++)
    {
        // If variable is found delete item at that position
        // And replace with variable supplied by user
        if (strcmp(e->symbols[i], k->value.symbol) == 0)
        {
            lval_del(e->values[i]);
            e->values[i] = lval_copy(v);
            return;
        }
    }

    // If no existing entry found allocate space for new entry
    e->count++;
    e->values = realloc(e->values, sizeof(lval*) * e->count);
    e->symbols = realloc(e->symbols, sizeof(char*) * e->count);

    // Copy contents of lval and symbol string into new location
    e->values[e->count - 1] = lval_copy(v);
    e->symbols[e->count - 1] = malloc(strlen(k->value.symbol) + 1);
    strcpy(e->symbols[e->count - 1], k->value.symbol);
}

void lenv_print(lenv *e)
{
    for (int i = 0; i < e->count; i++)
    {
        printf("%s : ", e->symbols[i]);
        lval_print(e->values[i]);
        putchar('\n');
    }
}
