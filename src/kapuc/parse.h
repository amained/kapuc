#ifndef KAPUC_PARSE_H
#define KAPUC_PARSE_H

#include "lib/sds.h"

#include <stdbool.h>

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
};

struct TREE_INT
{
    long value;
};

struct TREE_VARIABLE
{
    sds value;
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

// NOTE: this is linked list for a lot of reasons
// - stmt can be add and remove anytime
// - stmt can be move to point at other place
// - block can cease to exist
struct __attribute__((packed)) TREE_LEVEL_STMT
{
    struct parse_tree* statement;
    struct parse_tree** next;
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
    };
};

struct parser
{
    struct TOK* tokens; // sds
    unsigned int pos;
};

bool
build_entire_expression(struct parser* p, struct parse_tree* tree);

void
print_entire_expression(struct parse_tree* tree);

void
free_parse_tree(struct parse_tree* tree);

#endif // KAPUC_PARSE_H
