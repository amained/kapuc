#ifndef KAPUC_PARSE_H
#define KAPUC_PARSE_H

#include "lib/sds.h"
#include <stdbool.h>

enum ASTType {
  // data
  D_INT,
  D_IDENT,
  D_STRING,
  D_TRUE,
  D_FALSE,
  // expr
  E_UNARY,
  E_LIST,
  E_DOTS,
  // statements
  S_LET,
  S_RETURN,
  S_COFFEE,
  S_IF,
  S_ELIF,
  S_ELSE,
  // main tree stuff
  B_FILE_TREE,
  B_FUNC,
  B_STMTS,
  B_ERR // for error on the main tree
};

struct __attribute__((packed)) AST {
  sds value;
  uint8_t sub_value;
  struct AST *nodes;
  enum ASTType node_type;
};

struct __attribute__((aligned)) Parser {
  struct TOK *tokens;
  int pos;
  char *filename;
  bool error;
};

struct AST parse_tree(struct Parser *p);

void free_parser(struct Parser *p);

void free_tree(struct AST *tree);

void print_tree(struct AST *tree, int levels);

#ifdef SHIT_IS_IN_TESTING

struct AST parse_expr(struct Parser *p);
struct AST parse_atom(struct Parser *p);
struct AST parse_type_decl(struct Parser *p, struct AST(f)(struct Parser *));
struct AST parse_statement(struct Parser *p);
struct AST parse_func_decl(struct Parser *p);
struct AST parse_func(struct Parser *p);
struct AST parse_block(struct Parser *p);

#endif

#endif // KAPUC_PARSE_H
