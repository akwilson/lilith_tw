#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mpc.h"
#include "lilith_int.h"

//#define IOPS $(SUB,-) $(MUL,*) $(DIV,/) $(MOD,%) $(ADD,+) $(AND,&) $(OR,|) $(XOR,^) $(SR,>>) $(SL,<<)
#define IOPS $(SUB,-) $(MUL,*) $(DIV,/) $(ADD,+)

enum iops_enum {
#define $(x,op)   IOPSENUM_##x,
  IOPS
  IOPSENUM_COUNT
#undef $
};

static lval do_calc(enum iops_enum iop, lval xval, lval yval)
{
    static void *ll_jump_table[] = {
#define $(x,op)   &&LL_##x,
        IOPS
#undef $
    };

    static void *ld_jump_table[] = {
#define $(x,op)   &&LD_##x,
        IOPS
#undef $
    };

    static void *dl_jump_table[] = {
#define $(x,op)   &&DL_##x,
        IOPS
#undef $
    };

    static void *dd_jump_table[] = {
#define $(x,op)   &&DD_##x,
        IOPS
#undef $
    };

    if (xval.type == LVAL_LONG && yval.type == LVAL_LONG)
    {
        goto *(ll_jump_table[iop]);
#define $(x, op) LL_##x: return lval_long(xval.value.num_l op yval.value.num_l);
        IOPS
#undef $
    }

    if (xval.type == LVAL_LONG)
    {
        goto *(ld_jump_table[iop]);
#define $(x, op) LD_##x: return lval_double(xval.value.num_l op yval.value.num_d);
        IOPS
#undef $
    }

    if (yval.type == LVAL_LONG)
    {
        goto *(dl_jump_table[iop]);
#define $(x, op) DL_##x: return lval_double(xval.value.num_d op yval.value.num_l);
        IOPS
#undef $
    }

    goto *(dd_jump_table[iop]);
#define $(x, op) DD_##x: return lval_double(xval.value.num_d op yval.value.num_d);
    IOPS
#undef $
}

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
        return do_calc(IOPSENUM_ADD, x, y);
    }

    if (strcmp(operator, "-") == 0)
    {
        return do_calc(IOPSENUM_SUB, x, y);
    }

    if (strcmp(operator, "*") == 0)
    {
        return do_calc(IOPSENUM_MUL, x, y);
    }

    if (strcmp(operator, "/") == 0)
    {
        if (y.value.num_l == 0)
        {
            return lval_error(LERR_DIV_ZERO);
        }

        return do_calc(IOPSENUM_DIV, x, y);
    }

    if (x.type == LVAL_LONG && y.type == LVAL_LONG)
    {
        if (strcmp(operator, "%") == 0)
        {
            return lval_long(x.value.num_l % y.value.num_l);
        }

        if (strcmp(operator, "^") == 0)
        {
            return lval_long(powl(x.value.num_l, y.value.num_l));
        }

        if (strcmp(operator, "max") == 0)
        {
            return lval_long(x.value.num_l > y.value.num_l ? x.value.num_l : y.value.num_l);
        }

        if (strcmp(operator, "min") == 0)
        {
            return lval_long(x.value.num_l < y.value.num_l ? x.value.num_l : y.value.num_l);
        }
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
        return errno != ERANGE ? lval_long(num) : lval_error(LERR_BAD_NUM);
    }

    if (strstr(tree->tag, "decimal"))
    {
        errno = 0;
        double num = strtod(tree->contents, NULL);
        return errno != ERANGE ? lval_double(num) : lval_error(LERR_BAD_NUM);
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

