// #include "llvm-c/Types.h"

#ifndef KAPUC_CODE_TREE_H
#define KAPUC_CODE_TREE_H

#include "lib/sds.h"

// maybe we should have class in c
typedef struct CodeTreeNode CodeTreeNode;
//typedef void (*CodeGenFunc)(CodeTreeNode*);

typedef union NodeValue {
    int intVal;
    float floatVal;
    char* stringVal;
} NodeValue;

typedef enum CodeTreeNodeType {
    q_err,
    // normal stuff
    t_file,
    t_func,
    t_types,
    t_let,
    t_return,
    t_block,
    // data stuff
    d_int,
    d_type,
    d_pointer_get_address,
    d_pointer_get_value,
    // operation stuff
    e_expr,
    e_add,
    e_minus,
    e_multiply,
    e_divide,
    // coffee, internal one starts with _
    c__c_call,
} CodeTreeNodeType;

struct CodeTreeNode {
    enum CodeTreeNodeType type;
    CodeTreeNode* children;
    char* value;
//    CodeGenFunc codegen;
};

#ifdef SHIT_IS_IN_TESTING
struct CodeTreeNode generate_return(struct AST *tree);
void print_tree_node(struct CodeTreeNode *tree, int levels);
struct CodeTreeNode generate_block(struct AST *tree);
struct CodeTreeNode generate_function(struct AST *tree);
#endif

#endif //KAPUC_CODE_TREE_H
