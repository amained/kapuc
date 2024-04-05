#include "parse.h"
#include "lex.h"
#include "lib/log.h"
#include "lib/sds.h"
#include "lib/stb_ds.h"

#define expect_return(tok, t_expected, callback)                               \
    expect(tok, t_expected, callback, { return; })
#define expect(tok, t_expected, callback_success, callback_failure)            \
    if (tok.t != t_expected) {                                                 \
        callback_failure                                                       \
    } else                                                                     \
        callback_success

// declare some functions needed because c
struct AST
parse_expr(struct Parser* p);
struct AST
parse_list_decl(struct Parser* p, struct AST (*f)(struct Parser*));

struct AST
parse_atom(struct Parser* p)
{
    if (p->pos >= arrlen(p->tokens)) {
        return (struct AST){ .value = NULL, .node_type = -1 };
    }
    struct TOK tok = p->tokens[p->pos];
    switch (tok.t) {
        case IDENT:
            p->pos++;
            struct AST ast = { .value = tok.s, .node_type = D_IDENT };
            return ast;
        case NUM:
            p->pos++;
            struct AST ast2 = { .value = tok.s, .node_type = D_INT };
            return ast2;
        case STRING:
            p->pos++;
            struct AST ast8 = { .value = tok.s, .node_type = D_STRING };
            return ast8;
        case TRUE:
            p->pos++;
            struct AST ast9 = { .value = NULL, .node_type = D_TRUE };
            return ast9;
        case FALSE:
            p->pos++;
            struct AST ast10 = { .value = NULL, .node_type = D_FALSE };
            return ast10;
        case AMPERSAND:
            p->pos++;
            struct AST ast6 = parse_atom(p);
            if (ast6.node_type == -1) {
                log_error("failed to parse atom");
                p->error = true;
                return (struct AST){ .value = NULL, .node_type = -1 };
            }
            struct AST ast7 = { .sub_value = 0,
                                .nodes = NULL,
                                .node_type = E_UNARY }; // 0: REFERENCE
            arrput(ast7.nodes, ast6);
            return ast7;
        case LPAREN:
            p->pos++;
            struct AST ast3 = parse_expr(p);
            expect(
              p->tokens[p->pos],
              RPAREN,
              {
                  p->pos++;
                  return ast3;
              },
              { log_error("Expected )"); }) p->error = true;
            return (struct AST){ .value = NULL, .node_type = -1 };
        case LBRACKET:
            p->pos++;
            struct AST ast4 = parse_list_decl(p, &parse_expr);
            expect(
              p->tokens[p->pos],
              RBRACKET,
              {
                  p->pos++;
                  return ast4;
              },
              {
                  log_error("Expected ]");
                  log_debug("current token pos: %d", p->pos);
              }) p->error = true;
            return (struct AST){ .value = NULL, .node_type = -1 };
        case DOTDOT:
            p->pos++;
            struct AST ast5 = { .value = NULL,
                                .node_type = E_DOTS }; // variadic/other shit
            return ast5;
        default:
            return (struct AST){ .value = NULL, .node_type = -1 };
    }
}

int8_t
calc_precedence(enum TOK_TYPE t)
{
    switch (t) {
        case PLUS:
        case MINUS:
            return 1;
        case STAR:
        case SLASH:
            return 2;
        default:
            return -1;
    }
}

struct AST
parse_expr_1(struct AST left, struct Parser* p, int8_t precedence)
{
    if (left.node_type == -1) {
        return left;
    }
    if (p->pos >= arrlen(p->tokens)) {
        return left;
    }
    struct TOK op = p->tokens[p->pos];
    while (p->pos < arrlen(p->tokens) && calc_precedence(op.t) >= precedence) {
        op = p->tokens[p->pos];
        if (calc_precedence(op.t) == -1)
            return left;
        p->pos++;
        struct AST right = parse_atom(p);
        if (right.node_type == -1) {
            return right;
        }
        if (p->pos < arrlen(p->tokens)) {
            struct TOK next_op = p->tokens[p->pos];
            if (calc_precedence(next_op.t) > calc_precedence(op.t)) {
                right = parse_expr_1(right, p, calc_precedence(op.t) + 1);
            }
        }
        struct AST new_tree = {
            .sub_value = op.t - 21, .nodes = NULL, .node_type = E_UNARY
        }; // 0: PLUS, 1: MINUS, 2: MULTIPLY, 3: DIVIDE
        arrput(new_tree.nodes, left);
        arrput(new_tree.nodes, right);
        left = new_tree;
    }
    return left;
}

struct AST
parse_expr(struct Parser* p)
{
    return parse_expr_1(parse_atom(p), p, 0);
}

struct AST
parse_list_decl(struct Parser* p, struct AST (*f)(struct Parser*))
{
    struct AST tree = { .node_type = E_LIST };
    int length = arrlen(p->tokens);
    while (p->pos < length) {
        struct AST node = f(p);
        if (node.node_type == -1) {
            log_debug("failed to parse f(p)");
            p->error = true;
            return tree;
        }
        arrput(tree.nodes, node);
        if (p->tokens[p->pos].t != COMMA) {
            break; // end of the list
        }
        p->pos++;
    }
    return tree;
}

// this returns (*f)(*p) with types appended at the last of the nodes
struct AST
parse_type_decl(struct Parser* p, struct AST (*f)(struct Parser*))
{
    struct AST tree = f(p);
    if (tree.node_type == -1)
        return (struct AST){ .node_type = -1 };
    if (tree.sub_value == 0 && tree.node_type == E_UNARY)
        return tree; // REFERENCE
    if (tree.node_type == E_DOTS)
        return tree; // variadic
    expect(
      p->tokens[p->pos],
      COLON,
      { p->pos++; },
      { return (struct AST){ .node_type = -1 }; });
    struct AST type = parse_atom(p);
    arrput(tree.nodes, type);
    return tree;
}

struct AST
parse_let_decl(struct Parser* p)
{
    struct AST tree = { .node_type = S_LET };
    p->pos++;
    expect(
      p->tokens[p->pos],
      IDENT,
      {
          tree.value = p->tokens[p->pos].s;
          p->pos++;
      },
      { return (struct AST){ .node_type = -1 }; });
    return tree;
}

struct AST
parse_coffee(struct Parser* p)
{
    struct AST tree = { .node_type = S_COFFEE };
    p->pos++;
    tree.value = p->tokens[p->pos - 1].s;
    expect(
      p->tokens[p->pos],
      LPAREN,
      { p->pos++; },
      { return (struct AST){ .node_type = -1 }; });
    struct AST args = parse_list_decl(p, &parse_expr);
    arrput(tree.nodes, args);
    expect(
      p->tokens[p->pos],
      RPAREN,
      { p->pos++; },
      { return (struct AST){ .node_type = -1 }; });
    return tree;
}

struct AST
parse_block(struct Parser* pParser);

struct AST
parse_statement(struct Parser* p)
{
    struct TOK token = p->tokens[p->pos];
    switch (token.t) {
        case LET: {
            struct AST tree = parse_type_decl(p, &parse_let_decl);
            expect(
              p->tokens[p->pos],
              SEMICOLON,
              { p->pos++; },
              {
                  // with expression
                  expect(
                    p->tokens[p->pos],
                    EQ,
                    { p->pos++; },
                    { return (struct AST){ .node_type = -1 }; });
                  struct AST expression = parse_expr(p);
                  arrput(tree.nodes, expression);
                  expect(
                    p->tokens[p->pos],
                    SEMICOLON,
                    { p->pos++; },
                    { return (struct AST){ .node_type = -1 }; });
              });
            return tree;
        }
        case COFFEE: {
            struct AST tree = parse_coffee(p);
            expect(
              p->tokens[p->pos],
              SEMICOLON,
              { p->pos++; },
              { return (struct AST){ .node_type = -1 }; });
            return tree;
        }
        case IF: {
            struct AST tree = { .node_type = S_IF };
            p->pos++;
            struct AST expression = parse_expr(p);
            arrput(tree.nodes, expression);
            struct AST block = parse_block(p);
            arrput(tree.nodes, block);
            if (p->tokens[p->pos].t == ELIF) {
                while (p->tokens[p->pos].t == ELIF) {
                    p->pos++;
                    struct AST elif_expression = { .node_type = S_ELIF };
                    struct AST expression2 = parse_expr(p);
                    arrput(elif_expression.nodes, expression2);
                    struct AST block2 = parse_block(p);
                    arrput(elif_expression.nodes, block2);
                    arrput(tree.nodes, elif_expression);
                }
            }
            if (p->tokens[p->pos].t == ELSE) {
                p->pos++;
                struct AST else_expression = { .node_type = S_ELSE };
                struct AST block2 = parse_block(p);
                arrput(else_expression.nodes, block2);
                arrput(tree.nodes, else_expression);
            }
            return tree;
        }
        case RETURN: {
            struct AST tree = { .node_type = S_RETURN };
            p->pos++;
            struct AST expression = parse_expr(p);
            arrput(tree.nodes, expression);
            expect(
              p->tokens[p->pos],
              SEMICOLON,
              { p->pos++; },
              { return (struct AST){ .node_type = -1 }; });
            return tree;
        }
        default: {
            return (struct AST){ .node_type = -1 }; // end of expression?
        }
    }
}

// we can't use parse_list_decl here because we need to parse the type_decl
struct AST
parse_types(struct Parser* p)
{
    struct AST tree = { .node_type = E_LIST };
    while (p->pos < arrlen(p->tokens)) {
        struct AST node = parse_type_decl(p, &parse_atom);
        if (node.node_type == -1) {
            return tree; // empty tree
        }
        arrput(tree.nodes, node);
        if (p->tokens[p->pos].t != COMMA || node.node_type == E_DOTS) {
            break; // end of the list/varargs
        }
        p->pos++;
    }
    return tree;
}

struct AST
parse_func_decl(struct Parser* p)
{
    struct AST tree = { .node_type = B_FUNC };
    p->pos++;
    expect(
      p->tokens[p->pos],
      IDENT,
      {
          tree.value = p->tokens[p->pos].s;
          p->pos++;
      },
      { return (struct AST){ .node_type = -1 }; });
    expect(
      p->tokens[p->pos],
      LPAREN,
      { p->pos++; },
      { return (struct AST){ .node_type = -1 }; });
    struct AST args = parse_types(p);
    arrput(tree.nodes, args);
    expect(
      p->tokens[p->pos],
      RPAREN,
      { p->pos++; },
      { return (struct AST){ .node_type = -1 }; });
    expect(
      p->tokens[p->pos],
      COLON,
      { p->pos++; },
      { return (struct AST){ .node_type = -1 }; });
    expect(
      p->tokens[p->pos],
      IDENT,
      { arrput(tree.nodes, parse_atom(p)); },
      { return (struct AST){ .node_type = -1 }; });
    return tree;
}

struct AST
parse_block(struct Parser* p)
{
    if (p->tokens[p->pos].t == LBRACE) {
        struct AST tree = {};
        tree.node_type = B_STMTS;
        p->pos++;
        if (p->tokens[p->pos].t == RBRACE)
            return tree;
        while (p->pos < arrlen(p->tokens)) {
            struct AST node = parse_statement(p);
            if (node.node_type == -1) {
                log_debug("failed to parse statement");
                p->error = true;
                return tree;
            }
            arrput(tree.nodes, node);
            if (p->tokens[p->pos].t == RBRACE) {
                p->pos++;
                break;
            }
        }
        return tree;
    } else {
        struct AST tree = parse_statement(p);
        if (tree.node_type == -1) {
            log_debug("failed to parse statement");
            p->error = true;
            return tree;
        }
        return tree;
    }
}

struct AST
parse_func(struct Parser* p)
{
    struct AST tree = parse_func_decl(p);
    if (tree.node_type == -1) {
        log_debug("failed to parse func_decl");
        p->error = true;
        return tree;
    }
    expect(
      p->tokens[p->pos],
      SEMICOLON,
      { p->pos++; },
      {
          // with block
          struct AST block = parse_block(p);
          arrput(tree.nodes, block);
      });
    return tree;
}

void
print_tree(struct AST* tree, int levels)
{
    printf("%*s", levels * 4, "");
    if (tree->value != NULL) {
        printf("(%d): %s", tree->node_type, tree->value);
    } else {
        printf("(%d): NULL", tree->node_type);
    }
    printf("[%d:%02X]\n", tree->sub_value, tree->sub_value);
    if (tree->nodes != NULL)
        for (int i = 0; i < arrlen(tree->nodes); i++) {
            print_tree(&tree->nodes[i], levels + 1);
        }
}

struct AST
parse_tree(struct Parser* p)
{
    struct AST tree = { .node_type = B_FILE_TREE, .value = p->filename };
    while (p->pos < arrlen(p->tokens)) {
        if (p->tokens[p->pos].t == FUNC) {
            struct AST node = parse_func(p);
            if (node.node_type == -1) {
                log_debug("failed to parse func_decl");
                return tree;
            }
            arrput(tree.nodes, node);
        } else {
            log_debug("Unexpected token %d", p->tokens[p->pos].t);
            p->error = true;
            p->pos++;
        }
    }
    return tree;
}

void
free_parser(struct Parser* p)
{
    for (int i = 0; i < arrlen(p->tokens); i++) {
        sdsfree(p->tokens[i].s);
    }
    arrfree(p->tokens);
}

void
free_tree(struct AST* t)
{
    if (t->nodes != NULL) {
        for (int i = 0; i < arrlen(t->nodes); i++) {
            free_tree(&t->nodes[i]);
        }
        arrfree(t->nodes);
    }
}
