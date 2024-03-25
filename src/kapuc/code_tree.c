#include "code_tree.h"
#include "lib/stb_ds.h"
#include "stddef.h"
#include "src/kapuc/parse.h"
#include "lib/log.h"

// this need to be done before doing codegen because our ast suck, and we need to make it more descriptive
// BIG NOTE: *tree in this context is actually a pointer to the tree and not array/stb_ds

struct CodeTreeNode generate_int(struct AST *tree) {
    struct CodeTreeNode node = {.type = d_int, .children = NULL, .value = tree->value};
    return node;
}

struct CodeTreeNode generate_expr(struct AST *tree) {
    if (arrlen(tree->nodes) == 1) {
        return generate_int(&tree->nodes[0]);
    }
    struct CodeTreeNode node = {.children = NULL, .value = NULL};
    switch (tree->sub_value) {
        case 3:
            node.type = e_add;
            break;
        case 4:
            node.type = e_minus;
            break;
        case 5:
            node.type = e_multiply;
            break;
        case 6:
            node.type = e_divide;
            break;
        default:
            node.type = q_err;
            log_debug("error generating expr");
            break;
    }
    arrput(node.children, generate_int(&tree->nodes[0]));
    arrput(node.children, generate_int(&tree->nodes[1]));
    return node;
}

struct CodeTreeNode generate_return(struct AST *tree) {
    struct CodeTreeNode node = {.type = t_return, .children = NULL, .value = NULL};
    arrput(node.children, generate_expr(&tree->nodes[0]));
    return node;
}

struct CodeTreeNode generate_type(struct AST *tree) {
    struct CodeTreeNode node = {.type = d_type, .children = NULL, .value = tree->value};
    return node;
}

struct CodeTreeNode generate_function_types(struct AST *tree) {
    struct CodeTreeNode node = {.type = t_types, .children = NULL, .value = tree->value};
    return node;
}

struct CodeTreeNode generate_block(struct AST *tree) {
    // check if stmt or stmts
    if (tree->node_type == B_STMTS) {
        struct CodeTreeNode node = {.type = t_block, .children = NULL, .value = NULL};
        for (int i = 0; i < arrlen(tree->nodes); i++) {
            arrput(node.children, generate_return(&tree->nodes[i]));
        }
        return node;
    } else {
        struct CodeTreeNode node =  {.type = t_block, .children = NULL};
        arrput(node.children, generate_return(tree));
        return node;
    }
}

// func (func_name) -> (func_args, func_type, func_body)
// func_body -> stmt | (stmts -> (stmt, stmt, stmt, ...))
struct CodeTreeNode generate_function(struct AST *tree) {
    struct CodeTreeNode node = {.type = t_func, .children = NULL, .value = tree->value};
    arrput(node.children, generate_type(&tree->nodes[1]));
    arrput(node.children, generate_function_types(&tree->nodes[0]));
    // TODO: check if empty block
    // if it is empty, we bail
    arrput(node.children, generate_block(&tree->nodes[2]));
    return node;
}

struct CodeTreeNode generate_tree(struct AST *tree) {
    struct CodeTreeNode root = {.type = t_file, .children = NULL, .value = NULL};
    return root;
}

void print_tree_node(struct CodeTreeNode *tree, int levels) {
    for (int i = 0; i < levels; i++) printf("  ");
    if (tree->value != NULL) printf("(%d): %s\n", tree->type, tree->value);
    else printf("(%d)\n", tree->type);
    for (int i = 0; i < arrlen(tree->children); i++) {
        print_tree_node(&tree->children[i], levels + 1);
    }
}
