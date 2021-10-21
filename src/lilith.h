#pragma once

/*
 * Lilith -- a Lisp interpreter.
 */

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

/**
 * Initialises a new Lilith environment.
 */
lenv *lilith_init();

/**
 * Evaluates a Lilith value, consumes input in the process.
 * 
 * @param env   the environment
 * @param input an lval expression
 * @returns     an lval node with the evaluated result
 */
lval *lilith_eval_expr(lenv *env, lval *input);

/**
 * Reads one or more Lilith values from a string.
 *
 * @param input a string containing the Lilith expression(s)
 * @returns     a Lilith expression
 */
lval *lilith_read_from_string(const char *input);

/**
 * Loads one or more Lilith values from a file and evaluates them.
 * 
 * @param env      the Lilith environment
 * @param filename a string containing the filename
 */
void lilith_eval_file(lenv *env, const char *filename);

/**
 * Prints the contents of a Lilith value to the screen.
 * 
 * @param val the Lilith value to print
 */
void lilith_println(const lval *val);

/**
 * Frees up an lval.
 * 
 * @param val the Lilith value to free
 */
void lilith_lval_del(lval *val);

/**
 * Frees up the Lilith environment.
 */
void lilith_cleanup(lenv *env);
