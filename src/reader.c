/*
 * Reads an lval from a stream of tokens.
 */

#include <errno.h>

#include "lilith_int.h"
#include "tokeniser.h"

static lval *token_symbol(const char *val)
{
    // Booleans are a special-case symbol
    if (*val == '#')
    {
        if (!strcmp(val, "#t") || !strcmp(val, "#true"))
        {
            return lval_bool(true);
        }
        else if (!strcmp(val, "#f") || !strcmp(val, "#false"))
        {
            return lval_bool(false);
        }
    }

    return lval_symbol(val);
}

static lval *token_long(const tokeniser *tok, const char *val)
{
    errno = 0;
    long num = strtol(val, 0, 10);
    return errno != ERANGE
        ? lval_long(num)
        : lval_error("at %d:%d - invalid number %s", get_line_number(tok), get_position(tok), val);
}

static lval *token_double(const tokeniser *tok, const char *val)
{
    errno = 0;
    double num = strtod(val, 0);
    return errno != ERANGE
        ? lval_double(num)
        : lval_error("at %d:%d - invalid number %s", get_line_number(tok), get_position(tok), val);
}

static lval *read_element(const tokeniser *tok, const token *t)
{
    switch (t->type)
    {
    case TOK_STRING:
        return lval_string(t->token);
    case TOK_LONG:
        return token_long(tok, t->token);
    case TOK_DOUBLE:
        return token_double(tok, t->token);
    case TOK_SYMBOL:
        return token_symbol(t->token);
    case TOK_STRING_BEGIN:
        return lval_error("at %d:%d - unterminated string", get_line_number(tok), get_position(tok));
    case TOK_ERROR:
        return lval_error("at %d:%d - unexpected character in token %s",
                          get_line_number(tok), get_position(tok), t->token);
    default:
        return lval_error("at %d:%d - unable to process token %s",
                          get_line_number(tok), get_position(tok), t->token);
    }
}

static lval *read_list(tokeniser *tok, lval *list)
{
    token t;
    lval *rv = list;
    while (get_next_token(tok, &t))
    {
        if (t.type == TOK_LIST_BEGIN)
        {
            lval *x = read_list(tok, (*t.token == '(') ? lval_sexpression() : lval_qexpression());
            if (x->type == LVAL_ERROR)
            {
                lval_del(rv);
                return x;
            }

            lval_add(rv, x);
        }
        else if (t.type == TOK_LIST_END)
        {
            if ((rv->type == LVAL_SEXPRESSION && !strcmp(t.token, "}")) ||
                (rv->type == LVAL_QEXPRESSION && !strcmp(t.token, ")")))
            {
                lval_del(rv);
                return lval_error("at %d:%d - unexpected '%s'", get_line_number(tok), get_position(tok), t.token);
            }

            return rv;
        }
        else
        {
            lval *x = read_element(tok, &t);
            if (x->type == LVAL_ERROR)
            {
                lval_del(rv);
                return x;
            }

            lval_add(rv, x);
        }
    }

    lval_del(rv);
    return lval_error("at %d:%d - missing close bracket", get_line_number(tok), get_position(tok));
}

/**
 * Converts a string containing one or more Lilith expressions in to an lval.
 */
lval *read_from_string(const char *input)
{
    lval *rv = lval_sexpression();
    tokeniser *tok = new_tokeniser(input);
    token t;
    while (get_next_token(tok, &t))
    {
        lval *next;
        if (t.type == TOK_LIST_BEGIN)
        {
            next = read_list(tok, (*t.token == '(') ? lval_sexpression() : lval_qexpression());
            if (next->type == LVAL_ERROR)
            {
                lval_del(rv);
                rv = next;
                goto END;
            }

            lval_add(rv, next);
        }
        else
        {
            next = read_element(tok, &t);
            if (next->type == LVAL_ERROR)
            {
                lval_del(rv);
                rv = next;
                goto END;
            }

            lval_add(rv, next);
        }
    }

    if (rv->value.list.count == 1)
    {
        lval *y = lval_pop(rv);
        lval_del(rv);
        rv = y;
    }

END:
    free_tokeniser(tok);
    return rv;
}

/*
extern char stdlib_llth_start;

int main()
{
    char *expr = &stdlib_llth_start;
    //char *expr = "(head{1 2 3 4 5})";
    //char *expr = "(head {(+ 1\t(* 5 2))    })";
    //char *expr = "; dafuq\n;wibble\n   {{\n{+ (#t ^tyu \"balls\") 9} ; some stuff\n   5\t11 23 }}  ";
    //char *expr = "; dafuq\n;wibble\n   {{\n{+ (#t ^tyu \"balls\") 9} ; some stuff\n   5\t11 23 }}  ";
    //char *expr = "; dafuq\n;wib\tble\n   {{\n{+ (#t ^tyu \" this is a test\" \"ba\\tlls\") 9 2} ; some stuff\n   5\t11 23 }}  ";
    //printf("Expression: %s\n", expr);
    lval *lv = read_from_string(expr);
    lval_println(lv);
    return 0;
}
*/
