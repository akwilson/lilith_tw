#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <editline/readline.h>

#ifdef __linux
#include <editline/history.h>
#endif

#include "mpc.h"
#include "lilith_int.h"

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
    mpc_parser_t *symbol = mpc_new("symbol");
    mpc_parser_t *sexpression = mpc_new("sexpression");
    mpc_parser_t *qexpression = mpc_new("qexpression");
    mpc_parser_t *expression = mpc_new("expression");
    mpc_parser_t *lilith = mpc_new("lilith");

    mpca_lang(MPCA_LANG_DEFAULT,
            "                                                                                   \
                number      : /-?[0-9]+/ ;                                                      \
                decimal     : /-?[0-9]+\\.[0-9]+/ ;                                             \
                symbol      : '+' | '-' | '*' | '/' | '%' | '^' | \"max\" | \"min\" |           \
                              \"head\" | \"tail\" | \"join\" | \"list\" | \"eval\" | \"len\" |  \
                              \"cons\";                                                         \
                sexpression : '(' <expression>* ')' ;                                           \
                qexpression : '{' <expression>* '}' ;                                           \
                expression  : <decimal> | <number> | <symbol> | <sexpression> | <qexpression> ; \
                lilith      : /^/ <expression>* /$/ ;                                           \
            ",
            number, decimal, symbol, sexpression, qexpression, expression, lilith);

    printf("Lilith Lisp v0.0.1\n");
    printf("Ctrl+C to exit\n\n");

    while (1)
    {
        char *input = readline("lilith> ");
        add_history(input);

        mpc_result_t parse_result;
        if (mpc_parse("<stdin>", input, lilith, &parse_result))
        {
            if (ast_print)
            {
                mpc_ast_print(parse_result.output);
                printf("Leaf count: %d\n", leaf_count(parse_result.output, 0));
            }

            lval *result = lval_eval(lval_read(parse_result.output));
            lval_println(result);
            lval_del(result);
            mpc_ast_delete(parse_result.output);
        }
        else
        {
            mpc_err_print(parse_result.error);
            mpc_err_delete(parse_result.error);
        }

        free(input);
    }

    mpc_cleanup(7, number, decimal, symbol, sexpression, qexpression, expression, lilith);
    return 0;
}
