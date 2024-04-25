#include "../parse.h"
#include "./scope_analysis.h"
#include "lib/log.h"

#include <assert.h>

// get type id from the default type
// TODO: generate avaliable types from the import statements in global trees and
// check against those if we have imported those. currently this approach only
// support default types
static bool
get_type();

// get the type info from local scope
// if we don't have it in local scope, do get_type_info on the previous scope
// (if it exist)
static bool
get_typeinfo()
{
    return false;
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
    // old parse tree anyways?? at least it will be easier
    struct parse_tree* cur_tree = blk_tree;
    while (cur_tree->type == LVL_STMTS) {
#define SWITCH_BLOCK_STMTS                                                     \
    switch (cur_tree->level_stmts_tree.statement->type) {                      \
    case STMT_ASSIGNMENT: {                                                    \
        log_debug("found assignment");                                         \
    }                                                                          \
    default: {                                                                 \
        log_error("what!!!");                                                  \
        return false;                                                          \
    }                                                                          \
    }
        SWITCH_BLOCK_STMTS;
    }
    // one statement left
    SWITCH_BLOCK_STMTS;
#undef SWITCH_BLOCK_STMTS
    return false;
}
