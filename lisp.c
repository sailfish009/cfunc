#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "show.h"
#include "parse.h"
#include "env.h"
#include "gc.h"

bool from_bool(Value *x) { assert(x->t == BOOL); return x->v.bool_value; }
bool is_nil(Value *x) { assert(x->t == PAIR); return x->v.pair_value == NULL; }
Value *car(Value *x) { assert(x->t == PAIR); return x->v.pair_value->fst; }
Value *cdr(Value *x) { assert(x->t == PAIR); return x->v.pair_value->snd; }

Value *builtin_begin(Value *args) {
    // The args have all been evaluated in order.
    // Just return the final one:
    while (!is_nil(cdr(args))) args = cdr(args);
    return car(args);
}

Value *builtin_car(Value *args) {
    // Return the car of the first argument.
    return car(car(args));
}

Value *builtin_cdr(Value *args) {
    // Return the cdr of the first argument.
    return cdr(car(args));
}

Value *builtin_cons(Value *args) {
    return make_pair(car(args), car(cdr(args)));
}

Value *builtin_is_nil(Value *args) {
    Value *x = car(args);
    return make_bool(x->t == PAIR && x->v.pair_value == NULL);
}

Value *builtin_list(Value *args) {
    return args;
}

Value *builtin_plus(Value *args) {
    double sum = 0.0;
    while (!is_nil(args)) {
        assert(car(args)->t == NUMBER);
        sum += car(args)->v.number_value;
        args = cdr(args);
    }
    return make_number(sum);
}

Value *builtin_minus(Value *args) {
    double sum = 0.0;
    bool first = true;
    while (!is_nil(args)) {
        assert(car(args)->t == NUMBER);
        double v = car(args)->v.number_value;
        sum = first ? v : sum - v;
        first = false;
        args = cdr(args);
    }
    return make_number(sum);
}

Value *builtin_times(Value *args) {
    double product = 1.0;
    while (!is_nil(args)) {
        assert(car(args)->t == NUMBER);
        product *= car(args)->v.number_value;
        args = cdr(args);
    }
    return make_number(product);
}

Value *builtin_divide(Value *args) {
    double product = 1.0;
    bool first = true;
    while (!is_nil(args)) {
        assert(car(args)->t == NUMBER);
        double v = car(args)->v.number_value;
        product = first ? v : product / v;
        first = false;
        args = cdr(args);
    }
    return make_number(product);
}

#define BuiltinCompare(name, op) \
    Value *name(Value *args) { \
        assert(car(args)->t == NUMBER); \
        assert(car(cdr(args))->t == NUMBER); \
        bool equal = car(args)->v.number_value op car(cdr(args))->v.number_value; \
        return make_bool(equal); \
    }

BuiltinCompare(builtin_number_eq, ==)
BuiltinCompare(builtin_number_ne, !=)
BuiltinCompare(builtin_number_lt, <)
BuiltinCompare(builtin_number_gt, >)
BuiltinCompare(builtin_number_le, <=)
BuiltinCompare(builtin_number_ge, >=)

#undef BuiltinCompare

Value *builtin_number_equal(Value *args) {
    assert(car(args)->t == NUMBER);
    assert(car(cdr(args))->t == NUMBER);
    bool equal = car(args)->v.number_value == car(cdr(args))->v.number_value;
    return make_bool(equal);
}

Environment *make_global_env() {
    Environment *e = make_env(NULL);
    define_env(e, "begin", make_builtin(builtin_begin));
    define_env(e, "car", make_builtin(builtin_car));
    define_env(e, "cdr", make_builtin(builtin_cdr));
    define_env(e, "cons", make_builtin(builtin_cons));
    define_env(e, "nil?", make_builtin(builtin_is_nil));
    define_env(e, "list", make_builtin(builtin_list));
    define_env(e, "+", make_builtin(builtin_plus));
    define_env(e, "-", make_builtin(builtin_minus));
    define_env(e, "*", make_builtin(builtin_times));
    define_env(e, "/", make_builtin(builtin_divide));
    define_env(e, "=", make_builtin(builtin_number_eq));
    define_env(e, "!=", make_builtin(builtin_number_ne));
    define_env(e, "<", make_builtin(builtin_number_lt));
    define_env(e, ">", make_builtin(builtin_number_gt));
    define_env(e, "<=", make_builtin(builtin_number_le));
    define_env(e, ">=", make_builtin(builtin_number_ge));
    return e;
}

#define IS_KEYWORD(f, k) ((f)->t == SYMBOL && !strcmp((f)->v.symbol_value, k))

Environment *lambda_env(Lambda *f, Value *args, Environment *parent) {
    Environment *lenv;
    lenv = make_env(parent);
    Value *val;
    for (val = f->arguments; !is_nil(val); val = cdr(val)) {
        assert(car(val)->t == SYMBOL);
        define_env(lenv, car(val)->v.symbol_value, car(args));
        args = cdr(args);
    }
    return lenv;
}

Value *map_eval(Value *exp, Environment *env);

Value *eval(Value *exp, Environment *env) {
    char *s;
    // puts("Evaluating:"); print_value(exp,true); puts("");
    Value *f, *tail;

    switch (exp->t) {
    case BOOL:
    case NUMBER:
    case STRING:
    case BUILTIN:
        return exp;
    case SYMBOL:
        s = exp->v.symbol_value;
        return get_env(find_env(env, s), s);
    case PAIR:
        if (is_nil(exp)) /* nil constant */
            return exp;

        /* Apply a function. */
        f = car(exp); tail = cdr(exp);
        if (IS_KEYWORD(f, "if")) {
            return eval(from_bool(eval(car(tail), env))
                        ? car(cdr(tail))
                        : car(cdr(cdr(tail))), env);
        } else if (IS_KEYWORD(f, "lambda")) {
            return make_lambda(car(tail), car(cdr(tail)), env);
        } else if (IS_KEYWORD(f, "quote")) {
            return car(tail);
        } else if (IS_KEYWORD(f, "define")) {
            char *key = car(tail)->v.symbol_value;
            Value *val = eval(car(cdr(tail)), env);
            define_env(env, key, val);
            return make_nil();
        } else {
            Value *fe = eval(f, env);
            if (fe->t == BUILTIN) {
                assert(tail->t == PAIR);
                return (*fe->v.builtin_value)(map_eval(tail, env));
            } else if (fe->t == LAMBDA) {
                Environment *lenv = lambda_env(fe->v.lambda_value, map_eval(tail, env), env);
                Value *result = eval(fe->v.lambda_value->body, lenv);
                destroy_env(lenv);
                return result;
            } else {
                printf("Tried to call non-function: ");
                print_value(fe, true); puts("");
                exit(1);
            }
        }
    default:
        puts("???");
        exit(1);
    }
}

Value *map_eval(Value *exp, Environment *env) {
    if (is_nil(exp)) {
        return exp;
    } else {
        Value *v = eval(car(exp), env);
        return make_pair(v, map_eval(cdr(exp), env));
    }
}

int main(int argc, char **argv) {
    global_environment = make_global_env();
    char *buffer = 0;
    long length;
    FILE *f = fopen(argv[1], "rb"); if (!f) exit(1);
    fseek(f, 0, SEEK_END); length = ftell(f);
    fseek(f, 0, SEEK_SET);
    buffer = malloc(length); if (!buffer) exit(1);
    fread(buffer, 1, length, f);
    fclose(f);
    Value *code = parse_value(buffer, NULL);
    print_value(code, true); puts("");
    print_value(eval(code, global_environment), true); puts("");
    printf("Mark and sweep... %p\n", global_environment);
    mark_and_sweep();
    puts("Done.");
    destroy_env(global_environment);
    return 0;
}
