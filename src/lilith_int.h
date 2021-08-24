#pragma once

#include "mpc.h"

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

typedef lval*(*lbuiltin)(lenv*, lval*);

/**
 * Lisp Value types.
 */
enum
{
    LVAL_ERROR,
    LVAL_LONG,
    LVAL_DOUBLE,
    LVAL_SYMBOL,
    LVAL_FUN,
    LVAL_SEXPRESSION,
    LVAL_QEXPRESSION
};

/**
 * Lisp Value -- a node in an expression.
 */
struct lval
{
    int type;
    union
    {
        long num_l;
        double num_d;
        const char *error;
        char *symbol;
        struct
        {
            int count;
            struct lval **cell;
        } list;
        lbuiltin fun;
    } value;
};

/**
 * Genereates a new lval for a symbol.
 */
lval *lval_symbol(char *symbol);

/**
 * Generates a new lval for a function call.
 */
lval *lval_fun(lbuiltin function);

/**
 * Generates a new lval for a long integer.
 */
lval *lval_long(long num);

/**
 * Generates a new lval for a double.
 */
lval *lval_double(double num);

/**
 * Generates a new lval with an error message.
 */
lval *lval_error(const char *error);

/**
 * Generates a new lval for a q-expression. The returned value
 * contains no data and represents the start of an lval hierarchy.
 */
lval *lval_qexpression();

/**
 * Adds an lval to an s-expression.
 */
lval *lval_add(lval *v, lval *x);

/**
 * Converts an AST in to a hierarchy of lval nodes.
 */
lval *lval_read(const mpc_ast_t *tree);

/**
 * Prints the contents of an lval to the screen.
 */
void lval_print(const lval *v);

/**
 * Prints the contents of an lval to the screen with a newline.
 */
void lval_println(const lval *v);

/**
 * Frees up an lval.
 */
void lval_del(lval *v);

/**
 * Perform a deep copy on an lval.
 */
lval *lval_copy(lval *v);

/**
 * Initialises a new instance of lenv;
 */
lenv *lenv_new(void);

/**
 * Frees up an lenv.
 */
void lenv_del(lenv *e);

/**
 * Looks up a symbol from the environment.
 */
lval *lenv_get(lenv *e, lval *k);

/**
 * Adds a symbol to the environment. Replaces it if already present.
 */
void lenv_put(lenv *e, lval *k, lval *v);

/**
 * Add built-in functions to the environment.
 */
void lenv_add_builtins(lenv *e);

/**
 * Evaluates an lval expression.
 * 
 * @param input an lval expression
 * @returns     an lval node with the evaluated result
 */
lval *lval_eval(lenv *env, lval *input);
