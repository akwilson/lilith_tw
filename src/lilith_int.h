#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define LVAL_EXPR_CNT(arg) arg->value.list.count
#define LVAL_EXPR_FIRST(arg) arg->value.list.head->data

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

/**
 * Pointer to a built-in function.
 */
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
    LVAL_STRING,
    LVAL_SYMBOL,
    LVAL_BUILTIN_FUN,
    LVAL_SEXPRESSION,
    LVAL_QEXPRESSION,
    LVAL_USER_FUN
};

/**
 * A node in an lval linked list.
 */
typedef struct _pair
{
    lval *data;
    struct _pair *next;
} pair;

/**
 * Lisp Value -- a node in an expression.
 */
struct lval
{
    union
    {
        long num_l;
        double num_d;
        bool bval;
        char *str_val;

        // s-expressions or q-expressions
        struct
        {
            size_t count;
            pair *head;
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
    unsigned type;
};

/**
 * Return an item from the list.
 */
lval *lval_expr_item(lval *val, unsigned i);

/**
 * Remove and return the first item in the list.
 */
lval *lval_pop(lval *val);

/**
 * Remove an item from a list and delete the list and remaining items.
 */
lval *lval_take(lval *val, unsigned i);

/**
 * Generates a new lval with an error message.
 */
lval *lval_error(const char *fmt, ...);

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
 * Generates a new lval for a string.
 */
lval *lval_string(const char *string);

/**
 * Genereates a new lval for a symbol.
 */
lval *lval_symbol(const char *symbol);

/**
 * Generates a new lval for an s-expression. The returned value
 * contains no data and represents the start of an lval hierarchy.
 */
lval *lval_sexpression();

/**
 * Generates a new lval for a q-expression. The returned value
 * contains no data and represents the start of an lval hierarchy.
 */
lval *lval_qexpression();

/**
 * Generates a new lval for a function call.
 */
lval *lval_fun(lbuiltin function);

/**
 * Generates a new lval for a lambda expression.
 */
lval *lval_lambda(lval *formals, lval* body);

/**
 * Adds an lval to an s-expression.
 */
lval *lval_add(lval *v, lval *x);

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
 * Check two lvals for equality.
 */
bool lval_is_equal(lval *x, lval *y);

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
 * Add built-in arithmetic functions to the environment.
 */
void lenv_add_builtins_sums(lenv *e);

/**
 * Add built-in functions to the environment.
 */
void lenv_add_builtins_funcs(lenv *e);

/**
 * Performs a deep copy of the environment.
 */
lenv *lenv_copy(lenv *e);

/**
 * Converts an lenv to an lval.
 */
lval *lenv_to_lval(lenv *env);

/**
 * Convert a type in to a user-friendly name.
 */
char *ltype_name(unsigned type);

/**
 * Evaluates an lval expression, consumes input.
 * 
 * @param env   the environment
 * @param input an lval expression
 * @returns     an lval node with the evaluated result
 */
lval *lval_eval(lenv *env, lval *input);
