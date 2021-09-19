/*
 * Useful utility functions.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>

/**
 * Search the local directory and the LILITH_PATH for the given file name.
 * 
 * @param filename the filename to search for
 * @returns        the full name of the found filename, null if
 *                 not found. Needs to be freed by the caller.
 */
char *lookup_load_file(const char *filename)
{
    char *check = malloc(strlen(filename) + 1);

    struct stat fn;
    if (stat(filename, &fn) == 0)
    {
        strcpy(check, filename);
        return check;
    }

    free(check);
    const char *lp = getenv("LILITH_PATH");
    if (lp)
    {
        char buf[strlen(lp + 1)];
        char *next = buf;
        strcpy(buf, lp);

        char *end;
        check = malloc(strlen(lp) + strlen(filename) + 3);
        do
        {
            end = strchr(next, ':');
            if (end)
            {
                *end = 0;
            }

            sprintf(check, "%s/%s", next, filename);
            if (stat(check, &fn) == 0)
            {
                return check;
            }

            next = end + 1;
        } while (end);
    }

    return 0;
}
