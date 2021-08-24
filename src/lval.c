/*
 * Functions for reading, constructing and printing Lisp Values.
 */

#include <stdio.h>
#include <stdlib.h>
#include "lilith_int.h"

static void lval_expr_print(const lval *v, char open, char close)
{
    putchar(open);
    for (int i = 0; i < v->value.list.count; i++)
    {
        // Print Value contained within
        lval_print(v->value.list.cell[i]);

        // Don't print trailing space if last element
        if (i != (v->value.list.count - 1))
        {
            putchar(' ');
        }
    }

    putchar(close);
}

/**
 * Reads a long value from an AST.
 */
static lval *lval_read_long(const mpc_ast_t *tree)
{
    errno = 0;
    long num = strtol(tree->contents, NULL, 10);
    return errno != ERANGE ? lval_long(num) : lval_error("invalid number");
}

/**
 * Reads a double value from an AST.
 */
static lval *lval_read_double(const mpc_ast_t *tree)
{
    errno = 0;
    double num = strtod(tree->contents, NULL);
    return errno != ERANGE ? lval_double(num) : lval_error("invalid decimal");
}

lval *lval_sexpression()
{
    lval *rv = malloc(sizeof(lval));
    rv->type = LVAL_SEXPRESSION;
    rv->value.list.count = 0;
    rv->value.list.cell = 0;
    return rv;
}

lval *lval_symbol(char *symbol)
{
    lval *rv = malloc(sizeof(lval));
    rv->type = LVAL_SYMBOL;
    rv->value.symbol = malloc(strlen(symbol) + 1);
    strcpy(rv->value.symbol, symbol);
    return rv;
}

lval *lval_fun(lbuiltin function)
{
    lval *rv = malloc(sizeof(lval));
    rv->type = LVAL_FUN;
    rv->value.fun = function;
    return rv;
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

lval *lval_qexpression()
{
    lval *rv = malloc(sizeof(lval));
    rv->type = LVAL_QEXPRESSION;
    rv->value.list.count = 0;
    rv->value.list.cell = 0;
    return rv;
}

lval *lval_add(lval *v, lval *x)
{
    v->value.list.count++;
    v->value.list.cell = realloc(v->value.list.cell, sizeof(lval*) * v->value.list.count);
    v->value.list.cell[v->value.list.count - 1] = x;
    return v;
}

lval *lval_read(const mpc_ast_t *tree)
{
    // If Symbol or Number return conversion to that type
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

    // If root (>), sexpression or qexpression then create empty list
    lval *x = 0;
    if ((strcmp(tree->tag, ">") == 0) || (strstr(tree->tag, "sexpression")))
    {
        x = lval_sexpression();
    }
    else if (strstr(tree->tag, "qexpression"))
    {
        x = lval_qexpression();
    }

    // Fill this list with any valid expression contained within
    for (int i = 0; i < tree->children_num; i++)
    {
        if ((strcmp(tree->children[i]->contents, "(") == 0) ||
            (strcmp(tree->children[i]->contents, ")") == 0) ||
            (strcmp(tree->children[i]->contents, "{") == 0) ||
            (strcmp(tree->children[i]->contents, "}") == 0) ||
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
    case LVAL_FUN:
        printf("<function>");
        break;
    case LVAL_SEXPRESSION:
        lval_expr_print(v, '(', ')');
        break;
    case LVAL_QEXPRESSION:
        lval_expr_print(v, '{', '}');
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
    case LVAL_FUN:
        break;
    case LVAL_SYMBOL:
        free(v->value.symbol);
        break;
    case LVAL_SEXPRESSION:
    case LVAL_QEXPRESSION:
        for (int i = 0; i < v->value.list.count; i++)
        {
            lval_del(v->value.list.cell[i]);
        }

        free(v->value.list.cell);
        break;
    }

    free(v);
}

lval *lval_copy(lval *v)
{
    lval *rv = malloc(sizeof(lval));
    rv->type = v->type;

    switch (v->type)
    {
    case LVAL_LONG:
        rv->value.num_l = v->value.num_l;
        break;
    case LVAL_DOUBLE:
        rv->value.num_d = v->value.num_d;
        break;
    case LVAL_FUN:
        rv->value.fun = v->value.fun;
        break;
    case LVAL_ERROR:
        rv->value.error = v->value.error;
        break;
    case LVAL_SYMBOL:
        rv->value.symbol = malloc(strlen(rv->value.symbol + 1));
        strcpy(rv->value.symbol, v->value.symbol);
        break;
    case LVAL_QEXPRESSION:
    case LVAL_SEXPRESSION:
        rv->value.list.count = v->value.list.count;
        rv->value.list.cell = malloc(sizeof(lval) * rv->value.list.count);
        for (int i = 0; i < rv->value.list.count; i++)
        {
            rv->value.list.cell[i] = lval_copy(v->value.list.cell[i]);
        }
        break;
    }

    return rv;
}
