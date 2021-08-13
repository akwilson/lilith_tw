#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>
#include "mpc.h"

int main(int argc, char *argv[])
{
    mpc_parser_t *number = mpc_new("number");
    mpc_parser_t *operator = mpc_new("operator");
    mpc_parser_t *expression = mpc_new("expression");
    mpc_parser_t *lilith = mpc_new("lilith");

    mpca_lang(MPCA_LANG_DEFAULT,
            "                                                              \
                number   : /-?[0-9]+/ ;                                    \
                operator : '+' | '-' | '*' | '/' ;                         \
                expression : <number> | '(' <operator> <expression>+ ')' ; \
                lilith     : /^/ <operator> <expression>+ /$/ ;            \
            ",
            number, operator, expression, lilith);

    printf("Lilith Lisp v0.0.1\n");
    printf("Ctrl+C to exit\n\n");

    while (1)
    {
        char* input = readline("lilith> ");
        add_history(input);

        mpc_result_t parse_result;
        if (mpc_parse("<stdin>", input, lilith, &parse_result))
        {
            mpc_ast_print(parse_result.output);
            mpc_ast_delete(parse_result.output);
        }
        else
        {
            mpc_err_print(parse_result.error);
            mpc_err_delete(parse_result.error);
        }

        free(input);
    }

    mpc_cleanup(4, number, operator, expression, lilith);
    return 0;
}
