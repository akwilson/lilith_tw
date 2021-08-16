#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <editline/readline.h>
#include "mpc.h"

/**
 * LVal types
 */
enum { LVAL_NUM, LVAL_ERROR };

/**
 * Error codes
 */
enum { LERR_DIV_ZERO, LERR_BAD_NUM, LERR_BAD_OP };

/**
 * Lisp value
 */
typedef struct lval
{
    int type;
    long num;
    int error;
} lval;

static lval lval_num(long num)
{
    lval rv;
    rv.type = LVAL_NUM;
    rv.num = num;
    return rv;
}

static lval lval_error(int error)
{
    lval rv;
    rv.type = LVAL_ERROR;
    rv.error = error;
    return rv;
}

static void lval_print(lval v)
{
    switch (v.type)
    {
    case LVAL_NUM:
        printf("%li", v.num);
        break;
    case LVAL_ERROR:
        if (v.error == LERR_DIV_ZERO) {
            printf("Error: Division By Zero");
        }
        else if (v.error == LERR_BAD_OP)   {
            printf("Error: Invalid Operator");
        }
        else if (v.error == LERR_BAD_NUM)  {
            printf("Error: Invalid Number");
        }
        break;
    }
}

static void lval_println(lval v)
{
    lval_print(v);
    putchar('\n');
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
        return lval_num(x.num + y.num);
    }

    if (strcmp(operator, "-") == 0)
    {
        return lval_num(x.num - y.num);
    }

    if (strcmp(operator, "*") == 0)
    {
        return lval_num(x.num * y.num);
    }

    if (strcmp(operator, "/") == 0)
    {
        if (y.num == 0)
        {
            return lval_error(LERR_DIV_ZERO);
        }

        return lval_num(x.num / y.num);
    }

    if (strcmp(operator, "%") == 0)
    {
        return lval_num(x.num % y.num);
    }

    if (strcmp(operator, "^") == 0)
    {
        return lval_num(pow(x.num, y.num));
    }

    if (strcmp(operator, "max") == 0)
    {
        return lval_num(x.num > y.num ? x.num : y.num);
    }

    if (strcmp(operator, "min") == 0)
    {
        return lval_num(x.num < y.num ? x.num : y.num);
    }

    return lval_error(LERR_BAD_OP);
}

/**
 * Evaluates the parsed abstract syntax tree.
 */
static lval eval(mpc_ast_t *tree)
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

/**
 * Counts the number of leaves on an AST.
 */
static int leaf_count(mpc_ast_t *tree, int sofar)
{
    if (strstr(tree->tag, "number") || strstr(tree->tag, "operator"))
    {
        return sofar + 1;
    }

    int x = 0;
    for (int i = 0; i < tree->children_num; i++)
    {
        x += leaf_count(tree->children[i], sofar);
    }

    return x;
}

int main(int argc, char *argv[])
{
    bool ast_print = false;
    if (argc > 1)
    {
        if (strcmp(argv[1], "-a") == 0)
        {
            ast_print = true;
        }
    }

    mpc_parser_t *number = mpc_new("number");
    mpc_parser_t *decimal = mpc_new("decimal");
    mpc_parser_t *operator = mpc_new("operator");
    mpc_parser_t *expression = mpc_new("expression");
    mpc_parser_t *lilith = mpc_new("lilith");

    mpca_lang(MPCA_LANG_DEFAULT,
            "                                                                          \
                number     : /-?[0-9]+/ ;                                              \
                decimal    : /-?[0-9]+\\.[0-9]+/ ;                                     \
                operator   : '+' | '-' | '*' | '/' | '%' | '^' | \"max\" | \"min\" ;   \
                expression : <decimal> | <number> | '(' <operator> <expression>+ ')' ; \
                lilith     : /^/ <operator> <expression>+ /$/ ;                        \
            ",
            number, decimal, operator, expression, lilith);

    printf("Lilith Lisp v0.0.1\n");
    printf("Ctrl+C to exit\n\n");

    while (1)
    {
        char* input = readline("lilith> ");
        add_history(input);

        mpc_result_t parse_result;
        if (mpc_parse("<stdin>", input, lilith, &parse_result))
        {
            if (ast_print)
            {
                mpc_ast_print(parse_result.output);
                printf("Leaf count: %d\n", leaf_count(parse_result.output, 0));
            }

            lval result = eval(parse_result.output);
            lval_println(result);
            mpc_ast_delete(parse_result.output);
        }
        else
        {
            mpc_err_print(parse_result.error);
            mpc_err_delete(parse_result.error);
        }

        free(input);
    }

    mpc_cleanup(5, number, decimal, operator, expression, lilith);
    return 0;
}
