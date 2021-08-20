#include <stdio.h>
#include <stdlib.h>
#include "lilith_int.h"

static void lval_expr_print(const lval *v, char open, char close)
{
    putchar(open);
    for (int i = 0; i < v->value.list.count; i++) {
        /* Print Value contained within */
        lval_print(v->value.list.cell[i]);

        /* Don't print trailing space if last element */
        if (i != (v->value.list.count - 1)) {
            putchar(' ');
        }
    }

    putchar(close);
}


/**
 * Genereates a new lval for a symbol.
 */
static lval *lval_symbol(char *symbol)
{
    lval *rv = malloc(sizeof(lval));
    rv->type = LVAL_SYMBOL;
    rv->value.symbol = malloc(strlen(symbol) + 1);
    strcpy(rv->value.symbol, symbol);
    return rv;
}

/**
 * Generates a new lval for an s-expression. The returned value
 * contains no data and represents the start of an lval hierarchy.
 */
static lval *lval_sexpression()
{
    lval *rv = malloc(sizeof(lval));
    rv->type = LVAL_SEXPRESSION;
    rv->value.list.count = 0;
    rv->value.list.cell = 0;
    return rv;
}

static lval *lval_add(lval *v, lval *x)
{
    v->value.list.count++;
    v->value.list.cell = realloc(v->value.list.cell, sizeof(lval*) * v->value.list.count);
    v->value.list.cell[v->value.list.count - 1] = x;
    return v;
}

static lval *lval_read_long(const mpc_ast_t *tree)
{
    errno = 0;
    long num = strtol(tree->contents, NULL, 10);
    return errno != ERANGE ? lval_long(num) : lval_error("invalid number");
}

static lval *lval_read_double(const mpc_ast_t *tree)
{
    errno = 0;
    double num = strtod(tree->contents, NULL);
    return errno != ERANGE ? lval_double(num) : lval_error("invalid decimal");
}

lval *lval_long(long num)
{
    lval *rv = malloc(sizeof(lval));
    rv->type = LVAL_LONG;
    rv->value.num_l = num;
    return rv;
}

lval *lval_double(double num)
{
    lval *rv = malloc(sizeof(lval));
    rv->type = LVAL_DOUBLE;
    rv->value.num_d = num;
    return rv;
}

lval *lval_error(const char *error)
{
    lval *rv = malloc(sizeof(lval));
    rv->type = LVAL_ERROR;
    rv->value.error = error;
    return rv;
}

lval *lval_read(const mpc_ast_t *tree)
{
    /* If Symbol or Number return conversion to that type */
    if (strstr(tree->tag, "number"))
    {
        return lval_read_long(tree);
    }

    if (strstr(tree->tag, "decimal"))
    {
        return lval_read_double(tree);
    }

    if (strstr(tree->tag, "symbol"))
    {
        return lval_symbol(tree->contents);
    }

    /* If root (>) or sexpr then create empty list */
    lval *x = 0;
    if ((strcmp(tree->tag, ">") == 0) || (strstr(tree->tag, "sexpr")))
    {
        x = lval_sexpression();
    }

    /* Fill this list with any valid expression contained within */
    for (int i = 0; i < tree->children_num; i++)
    {
        if ((strcmp(tree->children[i]->contents, "(") == 0) ||
            (strcmp(tree->children[i]->contents, ")") == 0) ||
            (strcmp(tree->children[i]->tag,  "regex") == 0))
        {
            continue;
        }

        x = lval_add(x, lval_read(tree->children[i]));
    }

    return x;
}

void lval_print(const lval *v)
{
    switch (v->type)
    {
    case LVAL_LONG:
        printf("%li", v->value.num_l);
        break;
    case LVAL_DOUBLE:
        printf("%f", v->value.num_d);
        break;
    case LVAL_SYMBOL:
        printf("%s", v->value.symbol);
        break;
    case LVAL_ERROR:
        printf("Error: %s", v->value.error);
        break;
    case LVAL_SEXPRESSION:
        lval_expr_print(v, '(', ')');
        break;
    }
}

void lval_println(const lval *v)
{
    lval_print(v);
    putchar('\n');
}

void lval_del(lval *v)
{
    switch (v->type)
    {
    case LVAL_LONG:
    case LVAL_DOUBLE:
    case LVAL_ERROR:
        break;
    case LVAL_SYMBOL:
        free(v->value.symbol);
        break;
    case LVAL_SEXPRESSION:
        for (int i = 0; i < v->value.list.count; i++) {
            lval_del(v->value.list.cell[i]);
        }

        free(v->value.list.cell);
        break;
    }

    free(v);    
}
