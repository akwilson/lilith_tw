/*
 * Useful utility functions.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>

/**
 * Read contents of file in to a string.
 */
static char *load_file(const char *filename, struct stat *fn)
{
    char *contents = malloc(fn->st_size + 1);
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        return 0;
    }

    fread(contents, 1, fn->st_size, file);
    *(contents + fn->st_size) = 0;
    return contents;
}

/**
 * Search the local directory and the LILITH_PATH for the given file name.
 * 
 * @param filename the filename to search for
 * @returns        the contents of the file
 */
char *lookup_load_file(const char *filename)
{
    struct stat fn;
    if (stat(filename, &fn) == 0)
    {
        return load_file(filename, &fn);
    }

    const char *lp = getenv("LILITH_PATH");
    if (lp)
    {
        char buf[strlen(lp + 1)];
        char *next = buf;
        strcpy(buf, lp);

        char *end;
        char *check = malloc(strlen(lp) + strlen(filename) + 3);
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
                char *contents = load_file(check, &fn);
                free(check);
                return contents;
            }

            next = end + 1;
        } while (end);
    }

    return 0;
}

/**
 * Identifies an un-escapable character.
 */
bool is_unescapable(char x)
{
    static char *unescapable = "abfnrtv\\\'\"";
    return strchr(unescapable, x);
}

/**
 * Unescapes a given character. Shrinks an escaped '\t, '\n' etc into a single character.
 */
char char_unescape(char x)
{
    switch (x)
    {
        case 'a':  return '\a';
        case 'b':  return '\b';
        case 'f':  return '\f';
        case 'n':  return '\n';
        case 'r':  return '\r';
        case 't':  return '\t';
        case 'v':  return '\v';
        case '\\': return '\\';
        case '\'': return '\'';
        case '\"': return '\"';
    }

    return 0;
}

/**
 * Identifies an escapable character.
 */
bool is_escapable(char x)
{
    static char *escapable = "\a\b\f\n\r\t\v\\\'\"";
    return strchr(escapable, x);
}

/**
 * Escape given character. Expands '\t', '\n' etc to two character string of the expanded value.
 */
char *char_escape(char x)
{
    switch (x)
    {
        case '\a': return "\\a";
        case '\b': return "\\b";
        case '\f': return "\\f";
        case '\n': return "\\n";
        case '\r': return "\\r";
        case '\t': return "\\t";
        case '\v': return "\\v";
        case '\\': return "\\\\";
        case '\'': return "\\\'";
        case '\"': return "\\\"";
    }

    return "";
}
