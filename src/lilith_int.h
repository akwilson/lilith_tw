#ifndef LILITH_INT_H
#define LILITH_INT_H

#include "mpc.h"

/**
 * LVal types
 */
enum { LVAL_NUM, LVAL_ERROR };

/**
 * Error codes
 */
enum { LERR_DIV_ZERO, LERR_BAD_NUM, LERR_BAD_OP };

/**
 * Lisp value
 */
typedef struct lval
{
    int type;
    union {
        long num;
        int error;
    } value;
} lval;

lval lval_num(long num);
lval lval_error(int error);
void lval_print(lval v);
void lval_println(lval v);
lval eval(mpc_ast_t *tree);

#endif

