#pragma once

/*
 * The tokeniser reads a Lilith expression and converts it in to a stream of tokens.
 */

/**
 * The type of token read from the expression string.
 */
typedef enum
{
    TOK_NONE,
    TOK_LIST_BEGIN,
    TOK_LIST_END,
    TOK_STRING_BEGIN,
    TOK_STRING,
    TOK_LONG,
    TOK_DOUBLE,
    TOK_SYMBOL,
    TOK_ERROR,
    TOK_ADD_SUB,
    TOK_END
} TOKEN_TYPE;

/**
 * A token read from the input expression string.
 */
typedef struct
{
    const char *token;
    TOKEN_TYPE type;
} token;

/**
 * Maintains the state of a tokeniser.
 */
typedef struct tokeniser tokeniser;

/**
 * Create a new tokeniser.
 * 
 * @param input a string containing the Lilith expression to tokenise.
 */
tokeniser *new_tokeniser(const char *input);

/**
 * Reads the next token from the input string.
 * 
 * @param tok   pointer to the tokeniser
 * @param token pointer to the token, populated with the next token if found
 * @returns     true if a token is found, false otherwise
 */
bool get_next_token(tokeniser *tok, token *token);

/**
 * Gets the current line number of the string being tokenised.
 */
unsigned get_line_number(const tokeniser *tok);

/**
 * Gets the position on the current line being tokenised.
 */
unsigned get_position(const tokeniser *tok);

/**
 * Frees the tokeniser and all memory allocated by it.
 */
void free_tokeniser(tokeniser *tok);
