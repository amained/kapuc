#include "../parse.h"
#include "./scope_analysis.h"
#include "lib/log.h"

#include <assert.h>
#include <string.h>

// NOTE: To the unlucky one (probably me) who will read this, welcome to the
// territory where insanity met spaghetti, where dumb fucker try to write
// something, and dumb fucker try to review its own shit
// Actual useful NOTE: the design of this analyzer is like this.
// clang-format off
//             write &                                                                  
//             access datas generated from each passes                                  
//            +----------------------------------+                                      
//            |                                  |                                      
//            |                                  |                                      
//            |                                  |                                      
//            |                                  |                                      
// +----------v----------+                     (in order from top to bottom)            
// |block analyzer       +---> scope passes ---^---> type passes                  (easy)
// |(contains scope      |                     |                                        
// |variables, the       |                     +---> generate ssa form pass       (hard)
// |previous scope,      |                     |                                        
// |and the code         |                     +---> generational reference pass  (med) 
// |inside the block     |                     |                                        
// |NOTE: technically    |                     +---> dead code elimination        (med) 
// |the code isn't in    |                                                              
// |the block, but passed|                                                              
// |as argument to       |                                                              
// |the block passes)    |                                                              
// +---------------------+
// clang-format on

// utility function for the block_variables
#define T block_variables
void
block_variables_free(T* v)
{
    // do nothing
    // FIXME: do something
}
T
block_variables_copy(T* c)
{
    T* d = malloc(sizeof(T));
    memcpy(d, c, sizeof(T));
    log_debug("did it");
    return *d;
}
#undef T

#define TYPE_COMPARE(x, c, d)                                                  \
    if (strcmp(c, d) == 0) {                                                   \
        x;                                                                     \
        return true;                                                           \
    }

// get type id from the default type
// TODO: generate avaliable types from the import statements in global trees and
// check against those if we have imported those. currently this approach only
// support default types
static bool
get_default_type(sds s, struct k_trail* t)
{
    TYPE_COMPARE(return true;, s, "i8")
    return false;
}

// get the type info from local scope
// if we don't have it in local scope, do get_type_info on the previous scope
// (if it exist)
static inline bool
get_typeinfo(sds s, struct k_trail* t)
{
    return get_default_type(s, t);
}

static bool
check_dest_if_found()
{
    return false;
}

// will return false if found repeated value in scope
static bool
add_typeinfo_to_scope(struct scope_datas* cur_scope)
{
    log_debug("redefinition of %s in same scope");
    return false;
}

#define SWITCH_BLOCK_STMTS                                                     \
    switch (cur_tree->level_stmts_tree.statement->type) {                      \
    case STMT_ASSIGNMENT: {                                                    \
        log_debug("found assignment");                                         \
        block_variables* c = malloc(sizeof(block_variables));                  \
        c->dest = sdsnew("test");                                              \
        c->assign_type = 0;                                                    \
        vec_block_variables_push_back(&blk_scope.scope_variables, *c);         \
    }                                                                          \
    default: {                                                                 \
        log_error("what!!!");                                                  \
        /*return false;*/                                                      \
    }                                                                          \
    }

// previous_scopes can be NULL if we are at parent scope
bool
check_block(struct parse_tree* blk_tree, struct scope_datas* previous_scopes)
{
    assert(blk_tree->type == LVL_STMTS);
    struct scope_datas blk_scope;
    // add prev scope to the current
    blk_scope.previous_scopes = previous_scopes;
    // for each statement, add datas to the blk_scope.
    // FIXME: fucking change the block tree structure from linked list to an
    // vector?? maybe it will be faster?? slower?? since we never manipulate the
    // old parse tree anyways?? at least it will be easier. This approach also
    // won't work for block inside block (not ifs, just inline block) which is
    // god awful. If we want inline blocks, we have to use something like blocks
    // structure, which would be amazing.
    struct parse_tree* cur_tree = blk_tree;
    while (cur_tree->type == LVL_STMTS) {
        blk_scope.scope_variables = vec_block_variables_init();
        SWITCH_BLOCK_STMTS;
        log_debug("length: %d", blk_scope.scope_variables.size);
        break;
    }
    foreach (vec_block_variables, &blk_scope.scope_variables, iter)
        log_debug("%s\n", iter.ref->dest);
        // one statement left
        // SWITCH_BLOCK_STMTS;
#undef SWITCH_BLOCK_STMTS
    return false;
}
