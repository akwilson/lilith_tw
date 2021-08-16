#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <editline/readline.h>
#include "mpc.h"

/**
 * Evaluates a single expression
 */
static long eval_op(char *operator, long x, long y)
{
    if (strcmp(operator, "+") == 0)
    {
        return x + y;
    }

    if (strcmp(operator, "-") == 0)
    {
        return x - y;
    }

    if (strcmp(operator, "*") == 0)
    {
        return x * y;
    }

    if (strcmp(operator, "/") == 0)
    {
        return x / y;
    }

    if (strcmp(operator, "%") == 0)
    {
        return x % y;
    }

    if (strcmp(operator, "^") == 0)
    {
        return pow(x, y);
    }

    if (strcmp(operator, "max") == 0)
    {
        return x > y ? x : y;
    }

    if (strcmp(operator, "min") == 0)
    {
        return x < y ? x : y;
    }

    return 0;
}

/**
 * Evaluates the parsed abstract syntax tree.
 */
static long eval(mpc_ast_t *tree)
{
    // If it's a number it's a leaf node and can just be returned
    if (strstr(tree->tag, "number"))
    {
        return atoi(tree->contents);
    }

    // The second child of the expression must be the operator
    char *operator = tree->children[1]->contents;

    // The first number of the expression
    long x = eval(tree->children[2]);

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

            long result = eval(parse_result.output);
            printf("%li\n", result);
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
