/*
 * The tokeniser reads an input string containing a Lilith expression and
 * splits it up in to typed tokens. Characters from the input string are read
 * one at a time. As each character of a token is read a finite state machine
 * is used to keep track of and improve upon the inferred type. By the time the
 * token is read completely the type is decided.
 */

#include <ctype.h>

#include "lilith_int.h"
#include "tokeniser.h"

#define NEXT_BUF_START 64
#define WHITESPACE "\n\r\t\v "
#define TERMINAL_CHARS ")}\n\r\t\v "

bool is_unescapable(char x);
char char_unescape(char x);

typedef enum
{
    CHAR_NUMBER      = 0x0001,
    CHAR_LETTER      = 0x0002,
    CHAR_DOT         = 0x0004,
    CHAR_QUOTE       = 0x0008,
    CHAR_ADD_SUB     = 0x0010,
    CHAR_WHITESPACE  = 0x0020,
    CHAR_OPEN_PAREN  = 0x0040,
    CHAR_CLOSE_PAREN = 0x0080,
    CHAR_ENDINGS     = 0x00E0,
    CHAR_OTHER       = 0x0100,
    CHAR_ANY         = 0xFFFF
} CHAR_TYPE;

struct tokeniser
{
    const char *input; // original input expression
    const char *head;  // current place to read next token from
    char *next;        // buffer containing the next token
    size_t next_size;  // size of the next buffer
    unsigned line;     // current line number being tokenised
    unsigned position; // position on the current line
};

/**
 * A node in the FSM transition graph.
 */
typedef struct
{
    TOKEN_TYPE start_type;      // if the token is currently this type...
    unsigned transition_chars;  // ...and one of these characters is read...
    TOKEN_TYPE end_type;        // ...change the inferred token type to this
} graph_el;

/**
 * The FSM state transition graph.
 */
static graph_el state_machine[] =
{
    // We always start from TOK_NONE
    { TOK_NONE, CHAR_OPEN_PAREN, TOK_LIST_BEGIN },
    { TOK_NONE, CHAR_NUMBER, TOK_LONG },
    { TOK_NONE, CHAR_CLOSE_PAREN, TOK_LIST_END },
    { TOK_NONE, CHAR_QUOTE, TOK_STRING_BEGIN },
    { TOK_NONE, CHAR_DOT, TOK_DOUBLE },
    { TOK_NONE, CHAR_ADD_SUB, TOK_ADD_SUB },
    { TOK_NONE, CHAR_OTHER | CHAR_LETTER, TOK_SYMBOL },

    { TOK_LIST_BEGIN, CHAR_ANY, TOK_END },
    { TOK_LIST_END, CHAR_ANY, TOK_END },

    { TOK_ADD_SUB, CHAR_NUMBER, TOK_LONG },
    { TOK_ADD_SUB, CHAR_DOT, TOK_DOUBLE },
    { TOK_ADD_SUB, CHAR_ENDINGS, TOK_END },
    { TOK_ADD_SUB, CHAR_ANY, TOK_SYMBOL },

    { TOK_LONG, CHAR_LETTER | CHAR_ADD_SUB | CHAR_OTHER, TOK_SYMBOL },
    { TOK_LONG, CHAR_DOT, TOK_DOUBLE },
    { TOK_LONG, CHAR_QUOTE, TOK_ERROR },
    { TOK_LONG, CHAR_ENDINGS, TOK_END },

    { TOK_DOUBLE, CHAR_LETTER | CHAR_ADD_SUB | CHAR_OTHER, TOK_SYMBOL },
    { TOK_DOUBLE, CHAR_QUOTE, TOK_ERROR },
    { TOK_DOUBLE, CHAR_ENDINGS, TOK_END },

    { TOK_SYMBOL, CHAR_ENDINGS, TOK_END },

    { TOK_STRING_BEGIN, CHAR_QUOTE, TOK_STRING },

    { TOK_STRING, CHAR_ANY, TOK_END }
};

/**
 * Nunber of lines in the state machine graph.
 */
static const size_t state_machine_rows = 23;

/**
 * Classifies a character.
 * 
 * @param c the character to classify
 * @returns the character classification
 */
static CHAR_TYPE get_char_type(char c)
{
    if (isdigit(c))
    {
        return CHAR_NUMBER;
    }
    else if (isalpha(c))
    {
        return CHAR_LETTER;
    }
    else if (strchr(WHITESPACE, c))
    {
        return CHAR_WHITESPACE;
    }

    switch (c)
    {
    case '"':
        return CHAR_QUOTE;
    case '.':
        return CHAR_DOT;
    case '-':
    case '+':
        return CHAR_ADD_SUB;
    case '(':
    case '{':
        return CHAR_OPEN_PAREN;
    case ')':
    case '}':
        return CHAR_CLOSE_PAREN;
    }
    
    return CHAR_OTHER;
}

/**
 * Make sure the buffer is big enough to contain the token. Realloc it if not.
 */
static void check_next_buff(tokeniser *tok, const char *ptr)
{
    if (ptr - tok->next >= (long)tok->next_size)
    {
        tok->next = realloc(tok->next, tok->next_size * 2);
        tok->next_size *= 2;
    }
}

/**
 * Frees the tokeniser's next buffer.
 */
static void free_next_buf(tokeniser *tok)
{
    if (tok->next)
    {
        free(tok->next);
    }

    tok->next = 0;
}

/**
 * Moves the tokeniser's head to the next character and keeps track of line
 * number and position.
 */
static void increment_head(tokeniser *tok)
{
    if (*tok->head == '\n')
    {
        tok->line++;
        tok->position = 1;
    }
    else
    {
        tok->position++;
    }

    tok->head++;
}

/**
 * Moves the tokeniser's head to the end of the current line and tracks position.
 */
static void move_head_to_eol(tokeniser *tok)
{
    const char *ptr = strchr(tok->head, '\n');
    tok->position += ptr - tok->head;
    tok->head = ptr;
}

/**
 * Skips over whitespace and comments at the head of the tokeniser.
 */
static void skip_whitespace_and_comments(tokeniser *tok)
{
    while (*tok->head && strchr(WHITESPACE, *tok->head))
    {
        increment_head(tok);
    }
    
    if (*tok->head == ';')
    {
        move_head_to_eol(tok);
        skip_whitespace_and_comments(tok);
    }
}

/**
 * Improve the inferred token type.
 * 
 * @param current   the best token type inference so far
 * @param next_char the next character type encountered
 * @returns         the new best inference for the token
 */
static TOKEN_TYPE infer_token_type(TOKEN_TYPE current, CHAR_TYPE next_char)
{
    for (size_t i = 0; i < state_machine_rows; i++)
    {
        if (state_machine[i].start_type == current && state_machine[i].transition_chars & next_char)
        {
            return state_machine[i].end_type;
        }
    }

    return current;
}

/**
 * Copy character from the input expression string to the tokeniser's next
 * buffer. Increments the pointer in the next buffer.
 */
static void copy_char(char **ptr, tokeniser *tok, TOKEN_TYPE current_type)
{
    if (current_type == TOK_STRING_BEGIN || current_type == TOK_STRING)
    {
        if (*tok->head == '\\')
        {
            // Handle escape characters
            increment_head(tok);
            if (is_unescapable(*tok->head))
            {
                *(*ptr++) = char_unescape(*tok->head);
                return;
            }
        }
        else if (*tok->head == '"')
        {
            // Skip over '"'
            return;
        }
    }

    *(*ptr)++ = *tok->head;
}

tokeniser *new_tokeniser(const char *input)
{
    tokeniser *tok = malloc(sizeof(tokeniser));
    tok->input = input;
    tok->head = tok->input;
    tok->next = malloc(NEXT_BUF_START);
    tok->next_size = NEXT_BUF_START;
    tok->line = 1;
    tok->position = 1;

    skip_whitespace_and_comments(tok);
    return tok;
}

bool get_next_token(tokeniser *tok, token *token)
{
    if (*tok->head == 0)
    {
        free_next_buf(tok);
        return false;
    }

    TOKEN_TYPE current_type = TOK_NONE;
    TOKEN_TYPE best_type;
    char *ptr = tok->next;
    while (*tok->head != 0 &&
           current_type != TOK_ERROR &&
           (best_type = infer_token_type(current_type, get_char_type(*tok->head))) != TOK_END)
    {
        current_type = best_type;
        copy_char(&ptr, tok, current_type);
        check_next_buff(tok, ptr);
        increment_head(tok);
    }

    *ptr = 0;
    token->type = current_type == TOK_ADD_SUB ? TOK_SYMBOL : current_type;
    token->token = tok->next;

    skip_whitespace_and_comments(tok);
    return true;
}

unsigned get_line_number(const tokeniser *tok)
{
    return tok->line;
}

unsigned get_position(const tokeniser *tok)
{
    return tok->position;
}

void free_tokeniser(tokeniser *tok)
{
    free_next_buf(tok);
    free(tok);
}

/*
static const char *token_type_names[] =
{
    "NONE",
    "LIST_BEGIN",
    "LIST_END",
    "STRING_BEGIN",
    "STRING",
    "LONG",
    "DOUBLE",
    "SYMBOL",
    "ERROR",
    "ADD_SUB",
    "END"
};

extern char stdlib_llth_start;

int main()
{
    //char *expr = &stdlib_llth_start;
    //char * expr = "; dafuq\n;wibble\n   {{\n{+ (#t ^tyu \"balls\") 9} ; some stuff\n   -5\t+11 +23.6 -19.76 .4 -.895}}  ";
    char *expr = "; dafuq\n;wib\tble\n   {{\n{+ (#t ^tyu \" this is a test\" \"ba\\tlls\) 9} ; some stuff\n   5\t11 23 }}  ";
    //char *expr = "; dafuq\n;wibble\n   {{\n{+ (#t ^tyu \"balls) 9} ; some stuff   5\t11 23 }}  ";
    //char *expr = "(* 10.9 \"tits\" (+ 1 3))";
    //char *expr = "(eval(cons + {45 65 56 234(+ 10 2) 75 987}))";
    //printf("Expression: %s\n", expr);
    tokeniser *tok = new_tokeniser(expr);
    token token;
    while (get_next_token(tok, &token))
    {
        printf("%s %s\n", token_type_names[token.type], token.token);
        //getchar();
    }

    printf("end line: %d - char: %d\n", tok->line, tok->position);

    free_tokeniser(tok);
    return 0;
}
*/
