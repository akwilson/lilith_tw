#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>

int main(int argc, char* argv[])
{
    printf("Lilith Lisp v0.0.1\n");
    printf("Ctrl+C to exit\n\n");

    while (1)
    {
        char* input = readline("lilith> ");
        add_history(input);
        printf("No, you're a %s\n\n", input);
        free(input);
    }

    return 0;
}
