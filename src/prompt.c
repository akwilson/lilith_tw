#include <editline/readline.h>

#include "lilith_int.h"
#include "builtin_symbols.h"

#ifdef __linux
#include <editline/history.h>
extern char _stdlib_llth_start;
#else
extern char stdlib_llth_start;
#endif

lval *multi_eval(lenv *env, lval *expr);
lval *read_from_string(const char *input);

/**
 * Loads the statically linked Lilith standard library in to the environment.
 */
static lval *load_std_lib(lenv *env)
{
#ifdef __linux
    char *stdlib = &_stdlib_llth_start;
#else
    char *stdlib = &stdlib_llth_start;
#endif

    lval *expr = read_from_string(stdlib);
    return multi_eval(env, expr);
}

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
    lenv *env = lenv_new();

    lenv_add_builtins(env);
    lval *x = load_std_lib(env);
    if (x->type == LVAL_ERROR)
    {
        printf("Error reading standard library. ");
        lval_println(x);
        return 1;
    }

    lval_del(x);
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

            if (strcmp(input, "exit") == 0)
            {
                running = 0;
            }
            else
            {
                lval *result = lval_eval(env, read_from_string(input));
                lval_println(result);
                lval_del(result);
            }

            free(input);
        }
        else
        {
            putchar('\n');
            running = 0;
        }
    }

    lenv_del(env);
    return 0;
}
