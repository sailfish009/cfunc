#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "show.h"
#include "value.h"
#include "env.h"

size_t allocated = 0;
bool debug_gc = false;

void mark_env(Environment *env, int depth);
void mark_value(Value *val, int depth) {
    if (debug_gc) { printf("%*s ", 2*depth+1, "*"); print_value(val, true); puts(""); }
    assert(val != NULL);
    if (val->marked) return;
    val->marked = true;
    switch (val->t) {
        case BOOL:
        case NUMBER:
        case SYMBOL:
        case STRING:
        case BUILTIN:
            break;
        case PAIR:
            if (val->v.pair_value != NULL) {
                mark_value(val->v.pair_value->fst, depth + 1);
                mark_value(val->v.pair_value->snd, depth + 1);
            }
            break;
        case LAMBDA:
            mark_value(val->v.lambda_value->arguments, depth + 1);
            mark_value(val->v.lambda_value->body, depth + 1);
            mark_env(val->v.lambda_value->env, depth + 1);
            break;
    }
}

void mark_env(Environment *env, int depth) {
    if (debug_gc) { printf("%*s %s %p\n", 2*depth+1, "*", "Environment", env); }
    if (env->marked) return;
    env->marked = true;
    for (int i = 0; i < env->entries; i++) {
        mark_value(env->table[i].value, depth + 1);
    }
}

void print_heap() {
    for (int i = 0; i <= max_heap_index; i++) {
        if (heap[i] == NULL) {
            printf("%3d: -\n", i);
        } else {
            printf("%3d: %d ", i, heap[i]->marked);
            print_value(heap[i], true);
            puts("");
        }
    }

}

void sweep() {
    int old_heap_values = heap_values;
    int max_marked = 0;
    for (int i = 0; i <= max_heap_index; i++) {
        if (heap[i] == NULL) continue;
        if (heap[i]->marked) {
            if (i > max_marked) max_marked = i;
            heap[i]->marked = false;            
        } else {
            // bye bye
            free_value(heap[i]);
            heap[i] = NULL;
            heap_values--;
        }
    }
    heap_index = 0;
    max_heap_index = max_marked;
    printf("Swept heap down from %d to %d values.\n", old_heap_values, heap_values);
}

void mark_and_sweep() {
    mark_env(global_environment, 0);
    if (debug_gc) print_heap();
    sweep();
    if (debug_gc) print_heap();
}
