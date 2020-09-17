#pragma once

#include <stdbool.h>

typedef enum { BOOL, NUMBER, SYMBOL, STRING, PAIR, LAMBDA, BUILTIN } Type;
typedef struct Value Value;

#include "env.h"

typedef struct Pair {
    Value *fst;
    Value *snd;
} Pair;

typedef struct Lambda {
    Value *arguments;
    Value *body;
    Environment *env;
} Lambda;

struct Value {
    Type t;
    union {
        bool bool_value;  // #t or #f
        float number_value;
        char *symbol_value;
        Pair *pair_value;  // NULL == #nil
        Lambda *lambda_value;
        Value *(*builtin_value)(Value *);
    } v;
};

void init_heap();
void destroy_heap();
Value *make_nil();
Value *make_bool(bool x);
Value *make_number(double x);
Value *make_symbol(char *name);
Value *make_string(char *s);
Value *make_pair(Value *fst, Value *snd);
Value *make_lambda(Value *arguments, Value *body, Environment *env);
Value *make_builtin(Value *(*func)(Value *));
