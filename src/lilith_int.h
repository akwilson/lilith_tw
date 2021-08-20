#pragma once

#include "mpc.h"

/**
 * Lisp Value types
 */
enum { LVAL_ERROR, LVAL_LONG, LVAL_DOUBLE, LVAL_SYMBOL, LVAL_SEXPRESSION };

/**
 * Lisp Value -- a node in an expression.
 */
typedef struct lval
{
    int type;
    union {
        long num_l;
        double num_d;
        const char* error;
        char* symbol;
        struct {
            int count;
            struct lval **cell;
        } list;
    } value;
} lval;

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
 * Evaluates an lval expression.
 * 
 * @param input an lval expression
 * @returns     an lval node with the evaluated result
 */
lval *lval_eval(lval *input);
