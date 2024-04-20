#ifndef KAPUC_PARSE_H
#define KAPUC_PARSE_H

#include "lib/sds.h"

#include <stdbool.h>

struct parse_tree;

enum parse_tree_type
{
    // atom
    INT,
    VARIABLE,
    // expr
    UNARY_OP,  // !: 0
    BINARY_OP, // +: 0, -: 1, *: 2, /: 3, ==: 4, !=: 5, <=: 6, >=: 7
    PTR_REF,
    PTR_DEREF,
    // stmt
    LVL_STMTS,
    EMPTY_LVL_STMTS,
    STMT_ASSIGNMENT,
    STMT_RETURN,
    IFS,
    LOOP_FOR,
    // type_expr
    TYPE_TRAIL
};

struct TREE_INT
{
    long value;
};

struct TREE_VARIABLE
{
    sds value;
};

// with no props, we could just return props
// in normal situation with just 1 props it should be cur: type, prop: type
// when multiple props, change to linked list:
// cur: type, prop: -> (type_with_props: type, props: -> ...)
struct TREE_TYPE_WITH_PROPS
{
    struct parse_tree* cur;
    struct parse_tree* prop;
};

// this is not just for type, but for using value of struct and a lot of stuff
// that is linked-list-able and relate to value
struct TREE_TYPE_TRAIL
{
    int8_t trail_type; // 0: atom.atom, 1: atom::atom, others are reserved
    struct parse_tree* current;
    struct parse_tree* next;
};

struct __attribute__((packed)) TREE_BINARY_OP
{
    int8_t type;
    struct parse_tree* left;
    struct parse_tree* right;
};

struct __attribute__((packed)) TREE_UNARY_OP
{
    int type;
    struct parse_tree* value;
};

// TODO: merge this with parse_tree, maybe we don't have to do this at all, just
// have a flag on parse_tree that says if we ref or deref.
struct __attribute__((packed)) TREE_PTR_REF
{
    struct parse_tree* value;
};

struct __attribute__((packed)) TREE_PTR_DEREF
{
    struct parse_tree* value;
};

struct __attribute__((packed)) TREE_ASSIGNMENT
{
    struct parse_tree* type;
    sds name;
    bool isConst;
    struct parse_tree* value;
};

struct __attribute__((packed)) TREE_RETURN
{
    struct parse_tree* value;
};

// i love linked list
struct __attribute__((packed)) TREE_IFS
{
    struct parse_tree* condition;
    struct parse_tree* value;
    struct parse_tree* next;
};

// all of this is a block
struct __attribute__((packed)) TREE_FOR
{
    struct parse_tree* start;
    struct parse_tree* while_cond;
    struct parse_tree* end;
    struct parse_tree* inside;
};

// NOTE: this is linked list for a lot of reasons
// - stmt can be add and remove anytime
// - stmt can be move to point at other place
// - block can cease to exist
struct __attribute__((packed)) TREE_LEVEL_STMTS
{
    struct parse_tree* statement;
    struct parse_tree* next;
};

struct __attribute__((packed)) parse_tree
{
    enum parse_tree_type type;
    union
    {
        struct TREE_INT int_tree;
        struct TREE_VARIABLE var_tree;
        struct TREE_BINARY_OP binop_tree;
        struct TREE_PTR_REF ref_tree;
        struct TREE_PTR_DEREF deref_tree;
        struct TREE_LEVEL_STMTS level_stmts_tree;
        struct TREE_ASSIGNMENT assign_tree;
        struct TREE_RETURN return_tree;
        struct TREE_IFS ifs_tree;
        struct TREE_FOR loop_for_tree;
        struct TREE_TYPE_TRAIL trail;
    };
};

enum code_tree_type
{
    IMPORT,
    FUNCTION,
    TYPE, // type alias; type a = b<aa, bb>; or smth like this
    STRUCT,
    ENUM
};

struct CTREE_FUNCTION
{
    sds name;
    struct parse_tree* type;
    struct parse_tree* block;
};

// this is the part of the full tree of the program
struct __attribute__((packed)) code_tree
{
    enum code_tree_type type;
    union
    {
        struct CTREE_FUNCTION* tree;
    };
};

typedef struct code_tree* tree_ptr;
#define T tree_ptr
void
tree_ptr_free(T*);
T
tree_ptr_copy(T*);
#include "lib/ctl/vec.h"
// type of the full program tree will be vec_tree_ptr

struct parser
{
    struct TOK* tokens; // sds
    unsigned int pos;
};

void
print_entire_expression(struct parse_tree* tree);

bool
build_block(struct parser* p, struct parse_tree* tree);

void
free_parse_tree(struct parse_tree* tree);

#endif // KAPUC_PARSE_H
