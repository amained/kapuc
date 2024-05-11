#include "generator.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

struct PIR* create_PIR() {
    struct PIR *p = malloc(sizeof(struct PIR));
    p->main_blocks = vec_MAIN_BLOCK_init();
    if (p == NULL) return false;
    return p;
}

void
MAIN_BLOCK_free(MAIN_BLOCK* b) {
    switch(b->type) {
        case func: {
            vec_FUNC_VAR_free(&b->func.vv);
            vec_BLOCK_free(&b->func.bs);
        }
    }
    // free(b);
}
MAIN_BLOCK
MAIN_BLOCK_copy(MAIN_BLOCK* b) {
    MAIN_BLOCK* b2 = malloc(sizeof(MAIN_BLOCK));
    memcpy(b2, b, sizeof(MAIN_BLOCK));
    return *b2;
}

void
BLOCK_free(BLOCK* b) {
    // free(b);
}
BLOCK
BLOCK_copy(BLOCK* b) {
    BLOCK* b2 = malloc(sizeof(BLOCK));
    memcpy(b2, b, sizeof(BLOCK));
    return *b2;
}

void stmt_free(stmt* s) {
    // free(s);
}
stmt
stmt_copy(stmt* s) {
    stmt* s2 = malloc(sizeof(stmt));
    memcpy(s2, s, sizeof(stmt));
    return *s2;
}

void
FUNC_VAR_free(FUNC_VAR* f) {
    // free(f);
}
FUNC_VAR
FUNC_VAR_copy(FUNC_VAR* f) {
    FUNC_VAR* b2 = malloc(sizeof(FUNC_VAR));
    memcpy(b2, f, sizeof(FUNC_VAR));
    return *b2;
}

bool print_PIR(struct PIR* p) {
    if (p == NULL) return false;
    foreach(vec_MAIN_BLOCK, &p->main_blocks, iter) {
        switch(iter.ref->type) {
            case func: {
                printf("FUNC %s\n", iter.ref->func.name);
                continue;
            }
            default: {
                printf("unknown?? %d\n", iter.ref->type);
                return false;
            }
        }
    }
    return true;
}


size_t add_main_block_to_PIR(struct PIR* p, MAIN_BLOCK *b) {
    if (b != NULL && p != NULL) {
        vec_MAIN_BLOCK_push_back(&p->main_blocks, *b);
        return p->main_blocks.size - 1;
    }
    return -1;
}

size_t add_function_to_PIR(struct PIR* p, sds function_name, typing *t, bool is_external, bool is_variadic) {
    MAIN_BLOCK *b = malloc(sizeof(MAIN_BLOCK));
    if (!is_external) assert(!is_variadic);
    b->type = func;
    struct FUNC* f = malloc(sizeof(struct FUNC));
    f->name = function_name;
    if (f->is_external) {
        f->is_external = true;
        f->is_variadic = is_variadic;
    }
    else {
        f->is_external = false;
        f->is_variadic = false;
        f->bs = vec_BLOCK_init();
        f->vv = vec_FUNC_VAR_init();
    }
    f->t = *t;
    b->func = *f;
    return add_main_block_to_PIR(p, b);
}

size_t add_block_to_function(struct PIR* p, int func_index) {
    MAIN_BLOCK* big_b = vec_MAIN_BLOCK_at(&p->main_blocks, func_index);
    if (big_b != NULL && big_b->type == func) {
        BLOCK b = vec_stmt_init();
        vec_BLOCK_push_back(&big_b->func.bs, b);
        return big_b->func.bs.size - 1;
    }
    return -1;
}

size_t add_stmt_to_block(struct PIR* p, int func_index, int block_index, stmt* stmt) {
    MAIN_BLOCK* big_b = vec_MAIN_BLOCK_at(&p->main_blocks, func_index);
    if (big_b != NULL && big_b->type == func) {
        BLOCK* smol_b = vec_BLOCK_at(&big_b->func.bs, block_index);
        if (smol_b != NULL) {
            vec_stmt_push_back(smol_b, *stmt);
            return smol_b->size - 1;
        }
    }
    return -1;
}

size_t add_var_to_func(struct PIR* p, int func_index, FUNC_VAR v) {
    MAIN_BLOCK* big_b = vec_MAIN_BLOCK_at(&p->main_blocks, func_index);
    if (big_b != NULL && big_b->type == func) {
        vec_FUNC_VAR_push_back(&big_b->func.vv, v);
        return big_b->func.vv.size - 1;
    }
    return -1;
}

size_t
add_Expr_to_block(struct PIR* p,
                 int func_index,
                 int block_index,
                 expr* value,
                 FUNC_VAR v)
{
    stmt* s = malloc(sizeof(stmt));
    s->t = assignment;
    s->assignment.id = add_var_to_func(p, func_index, v);
    s->assignment.e = *value;
    add_stmt_to_block(p, func_index, block_index, s);
    return s->assignment.id;
}

size_t
add_Ret_to_block(struct PIR* p,
                 int func_index,
                 int block_index,
                 expr* value)
{
    stmt* s = malloc(sizeof(stmt));
    s->t = ret;
    s->ret_val = *value;
    add_stmt_to_block(p, func_index, block_index, s);
    return s->assignment.id;
}

size_t
add_Call_to_block(struct PIR* p,
                 int func_index,
                 int block_index,
                 int call_index)
{
    stmt* s = malloc(sizeof(stmt));
    s->t = call;
    s->call_ca.call_ids = call_index;
    return add_stmt_to_block(p, func_index, block_index, s);
}

void free_PIR(struct PIR *p) {
    vec_MAIN_BLOCK_free(&p->main_blocks);
    // free(p);
}