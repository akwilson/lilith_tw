#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mpc.h"
#include "lilith_int.h"

/**
 * Evaluates a single expression
 */
static lval eval_op(char *operator, lval x, lval y)
{
    if (x.type == LVAL_ERROR)
    {
        return x;
    }

    if (y.type == LVAL_ERROR)
    {
        return y;
    }

    if (strcmp(operator, "+") == 0)
    {
        return lval_num(x.value.num + y.value.num);
    }

    if (strcmp(operator, "-") == 0)
    {
        return lval_num(x.value.num - y.value.num);
    }

    if (strcmp(operator, "*") == 0)
    {
        return lval_num(x.value.num * y.value.num);
    }

    if (strcmp(operator, "/") == 0)
    {
        if (y.value.num == 0)
        {
            return lval_error(LERR_DIV_ZERO);
        }

        return lval_num(x.value.num / y.value.num);
    }

    if (strcmp(operator, "%") == 0)
    {
        return lval_num(x.value.num % y.value.num);
    }

    if (strcmp(operator, "^") == 0)
    {
        return lval_num(pow(x.value.num, y.value.num));
    }

    if (strcmp(operator, "max") == 0)
    {
        return lval_num(x.value.num > y.value.num ? x.value.num : y.value.num);
    }

    if (strcmp(operator, "min") == 0)
    {
        return lval_num(x.value.num < y.value.num ? x.value.num : y.value.num);
    }

    return lval_error(LERR_BAD_OP);
}

/**
 * Evaluates the parsed abstract syntax tree.
 */
lval eval(mpc_ast_t *tree)
{
    // If it's a number it's a leaf node and can just be returned
    if (strstr(tree->tag, "number"))
    {
        errno = 0;
        long num = strtol(tree->contents, NULL, 10);
        return errno != ERANGE ? lval_num(num) : lval_error(LERR_BAD_NUM);
    }

    // The second child of the expression must be the operator
    char *operator = tree->children[1]->contents;

    // The first number of the expression
    lval x = eval(tree->children[2]);

    // Read the rest of the expression
    for (int i = 3; strstr(tree->children[i]->tag, "expression"); i++)
    {
        x = eval_op(operator, x, eval(tree->children[i]));
    }

    return x;
}

