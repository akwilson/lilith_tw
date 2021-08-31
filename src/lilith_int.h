#pragma once

#include <stdbool.h>
#include "mpc.h"

#define LVAL_EXPR_CNT(arg) arg->value.list.count
#define LVAL_EXPR_LST(arg) arg->value.list.cell
#define LVAL_EXPR_ITEM(arg, i) arg->value.list.cell[i]

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
    LVAL_BOOL,
    LVAL_SYMBOL,
    LVAL_BUILTIN_FUN,
    LVAL_SEXPRESSION,
    LVAL_QEXPRESSION,
    LVAL_USER_FUN
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
        bool bval;
        char *error;
        char *symbol;

        // s-expressions or q-expressions
        struct
        {
            int count;
            lval **cell;
        } list;

        // functions
        lbuiltin builtin;
        struct
        {
            lenv *env;
            lval *formals;
            lval *body;
        } user_fun;
    } value;
};

/**
 * Removes an item from an lval list.
 */
lval *lval_pop(lval *val, int i);

/**
 * Removes an item from an lval list and deletes everything else.
 */
lval *lval_take(lval *val, int i);

/**
 * Generates a new lval for an s-expression. The returned value
 * contains no data and represents the start of an lval hierarchy.
 */
lval *lval_sexpression();

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
 * Generates a nw lval for a boolean.
 */
lval *lval_bool(bool bval);

/**
 * Generates a new lval with an error message.
 */
lval *lval_error(const char *fmt, ...);

/**
 * Generates a new lval for a q-expression. The returned value
 * contains no data and represents the start of an lval hierarchy.
 */
lval *lval_qexpression();

/**
 * Generates a new lval for a lambda expression.
 */
lval *lval_lambda(lval *formals, lval* body);

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
 * Sets an environment's parens.
 */
void lenv_set_parent(lenv *env, lenv *parent);

/**
 * Frees up an lenv.
 */
void lenv_del(lenv *e);

/**
 * Looks up a symbol from the environment.
 */
lval *lenv_get(lenv *e, lval *k);

/**
 * Adds a built-in symbol to the environment. Replaces it if already present.
 */
bool lenv_put_builtin(lenv *e, lval *k, lval *v);

/**
 * Adds a symbol to the environment. Replaces it if already present.
 */
bool lenv_put(lenv *e, lval *k, lval *v);

/**
 * Adds a symbol to the top-most environment. Replaces it if already present.
 */
bool lenv_def(lenv *e, lval *k, lval *v);

/**
 * Add built-in functions to the environment.
 */
void lenv_add_builtins(lenv *e);

/**
 * Print the environment.
 */
void lenv_print(lenv *e);

/**
 * Performs a deep copy of the environment.
 */
lenv *lenv_copy(lenv *e);

/**
 * Convert a type in to a user-friendly name.
 */
char *ltype_name(int type);

/**
 * Evaluates an lval expression.
 * 
 * @param input an lval expression
 * @returns     an lval node with the evaluated result
 */
lval *lval_eval(lenv *env, lval *input);
