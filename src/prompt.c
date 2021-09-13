#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <editline/readline.h>

#ifdef __linux
#include <editline/history.h>
#endif

#include "lilith_int.h"
#include "builtin_symbols.h"

mpc_parser_t *lilith_p;

static void load_and_eval_from_file(lenv *env, const char *filename)
{
    lval *args = lval_add(lval_sexpression(), lval_string(filename));
    lval *x = call_builtin(env, BUILTIN_SYM_LOAD, args);
    if (x->type == LVAL_ERROR)
    {
        lval_println(x);
    }

    lval_del(x);
}

static void version()
{
    printf("Lilith Lisp v0.1.0\n");
}

static void usage()
{
    version();
    printf("usage: lilith [-h] [-v] [-l] file...\n");
    printf("  -h : display this help message\n");
    printf("  -v : display version number\n");
    printf("  -l : load and evaluate file(s) and enter interpreter\n");
    printf("Additional arguments read as files and evaluated\n");
}

int main(int argc, char *argv[])
{
    int running = 1;
    mpc_parser_t *number_p = mpc_new("number");
    mpc_parser_t *decimal_p = mpc_new("decimal");
    mpc_parser_t *boolean_p = mpc_new("boolean");
    mpc_parser_t *string_p = mpc_new("string");
    mpc_parser_t *comment_p = mpc_new("comment");
    mpc_parser_t *symbol_p = mpc_new("symbol");
    mpc_parser_t *sexpression_p = mpc_new("sexpression");
    mpc_parser_t *qexpression_p = mpc_new("qexpression");
    mpc_parser_t *expression_p = mpc_new("expression");
    lilith_p = mpc_new("lilith");

    mpca_lang(MPCA_LANG_DEFAULT,
            "                                                                           \
                number      : /-?[0-9]+/ ;                                              \
                decimal     : /-?[0-9]+\\.[0-9]+/ ;                                     \
                boolean     : \"#t\" | \"#f\" | \"#true\" | \"#false\" ;                \
                string      : /\"(\\\\.|[^\"])*\"/ ;                                    \
                comment     : /;[^\\r\\n]*/ ;                                           \
                symbol      : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&%?]+/ ;                      \
                sexpression : '(' <expression>* ')' ;                                   \
                qexpression : '{' <expression>* '}' ;                                   \
                expression  : <decimal> | <number> | <boolean> | <string> | <comment> | \
                              <symbol> | <sexpression> | <qexpression> ;                \
                lilith      : /^/ <expression>* /$/ ;                                   \
            ",
            number_p, decimal_p, boolean_p, string_p, comment_p, symbol_p,
            sexpression_p, qexpression_p, expression_p, lilith_p);

    lenv *env = lenv_new();
    lenv_add_builtins(env);

    if (argc > 1)
    {
        if (strcmp(argv[1], "-h") == 0)
        {
            usage();
            running = 0;
        }
        else if (strcmp(argv[1], "-v") == 0)
        {
            version();
            running = 0;
        }
        else
        {
            running = (strcmp(argv[1], "-l") == 0);
            for (int i = 1; i < argc; i++)
            {
                if (argv[i][0] != '-')
                {
                    load_and_eval_from_file(env, argv[i]);
                }
            }
        }
    }

    if (running)
    {
        version();
        printf("Ctrl+C or 'exit' to exit\n\n");
    }

    while (running)
    {
        char *input = readline("lilith> ");
        if (input)
        {
            add_history(input);
            mpc_result_t parse_result;

            if (strcmp(input, "exit") == 0)
            {
                running = 0;
            }
            else if (strcmp(input, "env") == 0)
            {
                lenv_print(env);
            }
            else if (mpc_parse("<stdin>", input, lilith_p, &parse_result))
            {
                lval *result = lval_eval(env, lval_read(parse_result.output));
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
        else
        {
            putchar('\n');
            running = 0;
        }
    }

    mpc_cleanup(10, number_p, decimal_p, boolean_p, string_p, comment_p,
        symbol_p, sexpression_p, qexpression_p, expression_p, lilith_p);
    lenv_del(env);
    return 0;
}
