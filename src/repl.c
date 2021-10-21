/*
 * The entry point for the Lilith interpreter.
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <editline/readline.h>

#include "lilith.h"

#ifdef __linux
#include <editline/history.h>
#endif

static void version()
{
    printf("Lilith Lisp v0.3.0\n");
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
    bool running = true;
    lenv *env = lilith_init();
    if (!env)
    {
        printf("Error initialising Lilith environment\n");
        return 1;
    }

    if (argc > 1)
    {
        if (strcmp(argv[1], "-h") == 0)
        {
            usage();
            running = false;
        }
        else if (strcmp(argv[1], "-v") == 0)
        {
            version();
            running = false;
        }
        else
        {
            running = (strcmp(argv[1], "-l") == 0);
            for (int i = 1; i < argc; i++)
            {
                if (argv[i][0] != '-')
                {
                    lilith_eval_file(env, argv[i]);
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

            if (strcmp(input, "exit") == 0)
            {
                running = false;
            }
            else
            {
                lval *result = lilith_eval_expr(env, lilith_read_from_string(input));
                lilith_println(result);
                lilith_lval_del(result);
            }

            free(input);
        }
        else
        {
            putchar('\n');
            running = false;
        }
    }

    lilith_cleanup(env);
    return 0;
}
