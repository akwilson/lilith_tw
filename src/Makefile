BIN1 = lilith
BIN1_SRCS = lval.c builtins_funcs.c builtins_sums.c eval.c lenv.c repl.c utils.c tokeniser.c reader.c
BIN1_BLOBS = stdlib.llth

INCLUDE_PATH = -I../lib/collections/src
LIB_PATH = -L../lib/collections/build
LIBS = -ledit -lm ../lib/collections/build/libclxns.a

# Dynamically link to collections
# LIBS = -Wl,-rpath,$(shell pwd)/../lib/collections/build -ledit -lm -lclxns

include ../lib/simplified-make/simplified.mk
