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
    return errno != ERANGE ? lval_long(num) : lval_error("invalid number: %s", tree->contents);
}

/**
 * Reads a double value from an AST.
 */
static lval *lval_read_double(const mpc_ast_t *tree)
{
    errno = 0;
    double num = strtod(tree->contents, NULL);
    return errno != ERANGE ? lval_double(num) : lval_error("invalid decimal: %s", tree->contents);
}

lval *lval_pop(lval *val, int i)
{
    lval *x = LVAL_EXPR_ITEM(val, i);

    // Shift memory after the item at "i" over the top
    memmove(&LVAL_EXPR_ITEM(val, i), &LVAL_EXPR_ITEM(val, i + 1),
        sizeof(lval*) * (LVAL_EXPR_CNT(val) - i - 1));

    // Decrease the count of items in the list
    LVAL_EXPR_CNT(val)--;

    // Reallocate the memory used
    LVAL_EXPR_LST(val) = realloc(LVAL_EXPR_LST(val), sizeof(lval*) * LVAL_EXPR_CNT(val));
    return x;
}

lval *lval_take(lval *val, int i)
{
    lval *x = lval_pop(val, i);
    lval_del(val);
    return x;
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
    rv->type = LVAL_BUILTIN_FUN;
    rv->value.builtin = function;
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

lval *lval_bool(bool bval)
{
    lval *rv = malloc(sizeof(lval));
    rv->type = LVAL_BOOL;
    rv->value.bval = bval;
    return rv;
}

lval *lval_error(const char *fmt, ...)
{
    lval *v = malloc(sizeof(lval));
    v->type = LVAL_ERROR;

    // Create a va list and initialize it
    va_list va;
    va_start(va, fmt);

    // Allocate 512 bytes of space
    v->value.error = malloc(512);

    // printf the error string with a maximum of 511 characters
    vsnprintf(v->value.error, 511, fmt, va);

    // Reallocate to number of bytes actually used
    v->value.error = realloc(v->value.error, strlen(v->value.error) + 1);

    // Cleanup our va list
    va_end(va);
    return v;
}

lval *lval_qexpression()
{
    lval *rv = malloc(sizeof(lval));
    rv->type = LVAL_QEXPRESSION;
    rv->value.list.count = 0;
    rv->value.list.cell = 0;
    return rv;
}

lval *lval_lambda(lval *formals, lval* body)
{
    lval *rv = malloc(sizeof(lval));
    rv->type = LVAL_USER_FUN;

    // Build new environment
    rv->value.user_fun.env = lenv_new();

    // Set formals and body
    rv->value.user_fun.formals = formals;
    rv->value.user_fun.body = body;
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

    if (strstr(tree->tag, "boolean"))
    {
        return lval_bool(tree->contents[1] == 't');
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
    case LVAL_BOOL:
        printf("%s", v->value.bval ? "#t" : "#f");
        break;
    case LVAL_SYMBOL:
        printf("%s", v->value.symbol);
        break;
    case LVAL_ERROR:
        printf("Error: %s", v->value.error);
        break;
    case LVAL_BUILTIN_FUN:
        printf("<builtin>");
        break;
    case LVAL_SEXPRESSION:
        lval_expr_print(v, '(', ')');
        break;
    case LVAL_QEXPRESSION:
        lval_expr_print(v, '{', '}');
        break;
    case LVAL_USER_FUN:
        printf("(\\ ");
        lval_print(v->value.user_fun.formals);
        putchar(' ');
        lval_print(v->value.user_fun.body);
        putchar(')');
        break;
    }
}

void lval_println(const lval *v)
{
    lval_print(v);
    putchar('\n');
}

bool lval_is_equal(lval *x, lval *y)
{
    if (x->type != y->type)
    {
        if (x->type == LVAL_LONG && y->type == LVAL_DOUBLE)
        {
            return x->value.num_l == y->value.num_d;
        }
        else if (x->type == LVAL_DOUBLE && y->type == LVAL_LONG)
        {
            return x->value.num_d == y->value.num_l;
        }

        return 0;
    }

    switch (x->type)
    {
    case LVAL_LONG:
        return x->value.num_l == y->value.num_l;
    case LVAL_DOUBLE:
        return x->value.num_d == y->value.num_d;
    case LVAL_BOOL:
        return x->value.bval == y->value.bval;
    case LVAL_ERROR:
        return (strcmp(x->value.error, y->value.error) == 0);
    case LVAL_SYMBOL:
        return (strcmp(x->value.symbol, y->value.symbol) == 0);
    case LVAL_BUILTIN_FUN:
        return x->value.builtin == y->value.builtin;
    case LVAL_USER_FUN:
        return lval_is_equal(x->value.user_fun.formals, y->value.user_fun.formals) &&
            lval_is_equal(x->value.user_fun.body, y->value.user_fun.body);
    case LVAL_QEXPRESSION:
    case LVAL_SEXPRESSION:
        if (LVAL_EXPR_CNT(x) != LVAL_EXPR_CNT(y))
        {
            return false;
        }

        for (int i = 0; i < LVAL_EXPR_CNT(x); i++)
        {
            if (!lval_is_equal(LVAL_EXPR_ITEM(x, i), LVAL_EXPR_ITEM(y, i)))
            {
                return 0;
            }
        }
        return true;
    }

    return false; 
}

void lval_del(lval *v)
{
    switch (v->type)
    {
    case LVAL_LONG:
    case LVAL_DOUBLE:
    case LVAL_BUILTIN_FUN:
    case LVAL_BOOL:
        break;
    case LVAL_ERROR:
        free(v->value.error);
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
    case LVAL_USER_FUN:
        lenv_del(v->value.user_fun.env);
        lval_del(v->value.user_fun.formals);
        lval_del(v->value.user_fun.body);
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
    case LVAL_BOOL:
        rv->value.bval = v->value.bval;
        break;
    case LVAL_BUILTIN_FUN:
        rv->value.builtin = v->value.builtin;
        break;
    case LVAL_ERROR:
        rv->value.error = v->value.error;
        break;
    case LVAL_SYMBOL:
        rv->value.symbol = malloc(strlen(v->value.symbol + 1));
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
    case LVAL_USER_FUN:
        rv->value.user_fun.env = lenv_copy(v->value.user_fun.env);
        rv->value.user_fun.formals = lval_copy(v->value.user_fun.formals);
        rv->value.user_fun.body = lval_copy(v->value.user_fun.body);
        break;
    }

    return rv;
}

char *ltype_name(int type)
{
    switch(type)
    {
        case LVAL_BUILTIN_FUN:
        case LVAL_USER_FUN:
            return "Function";
        case LVAL_LONG:
            return "Number";
        case LVAL_DOUBLE:
            return "Decimal";
        case LVAL_BOOL:
            return "Boolean";
        case LVAL_ERROR:
            return "Error";
        case LVAL_SYMBOL:
            return "Symbol";
        case LVAL_SEXPRESSION:
            return "S-Expression";
        case LVAL_QEXPRESSION:
            return "Q-Expression";
        default:
            return "Unknown";
    }
}
