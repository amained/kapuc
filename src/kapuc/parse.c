#include "parse.h"
#include "lex.h"
#include "lib/log.h"
#include "lib/sds.h"
#include "lib/stb_ds.h"
#include <stdint.h>
#include <stdlib.h>

static bool
get_cur_tok(struct parser* p, struct TOK* tok)
{
    if (arrlen(p->tokens) > p->pos) {
        *tok = p->tokens[p->pos];
        return true;
    }
    return false;
}

static bool
get_next_tok(struct parser* p, struct TOK* tok)
{
    if (arrlen(p->tokens) > p->pos + 1) {
        tok = &p->tokens[p->pos + 1];
        return true;
    }
    return false;
}

static bool
advance(struct parser* p)
{
    if (arrlen(p->tokens) > p->pos) {
        p->pos++;
        return true;
    }
    return false;
}

bool
build_atom(struct parser* p, struct parse_tree* tree)
{
    struct TOK t = {};
    if (get_cur_tok(p, &t)) {
        switch (t.t) {
            case NUM: {
                tree->type = INT;
                // TODO: The lexer get the float here too? not sure. maybe we
                // should make the lexer just get the number don't care about
                // float?
                tree->int_tree.value =
                  atol(t.s); // NOLINT(cert-err34-c) atol works here because we
                             // know it is an integer
                advance(p);
                return true;
            }
            case IDENT: {
                tree->type = VARIABLE;
                tree->var_tree.value = t.s;
                advance(p);
                return true;
            }
            default: {
                return false;
            }
        }
    }
    return false;
}

static int8_t
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

static int8_t
generate_type_from_op(enum TOK_TYPE t)
{
    switch (t) {
        case PLUS:
            return 0;
        case MINUS:
            return 1;
        case STAR:
            return 2;
        case SLASH:
            return 3;
        default:
            return -1;
    }
}
static bool
build_nth_entire_expression(struct parser* p,
                            struct parse_tree* tree,
                            int8_t precedence)
{
    struct TOK op;
    if (!get_cur_tok(p, &op))
        return true; // no token left to process
    while (calc_precedence(op.t) >= precedence) {
        if (get_cur_tok(p, &op)) {
            int8_t prec = calc_precedence(op.t);
            if (prec == -1)
                break;
            advance(p);
            struct parse_tree* right = malloc(sizeof(struct parse_tree));
            if (build_atom(p, right)) {
                struct TOK next_op;
                if (get_cur_tok(p, &next_op)) {
                    log_debug("get next_op at pos %d", p->pos);
                    if (calc_precedence(next_op.t) > prec) {
                        if (!build_nth_entire_expression(p, right, prec + 1))
                            return false;
                    }
                }
                log_debug("constructing new_tree at pos %d", p->pos);
                struct parse_tree new_tree;
                new_tree.type = BINARY_OP;
                new_tree.binop_tree.type = generate_type_from_op(op.t);
                new_tree.binop_tree.left = malloc(sizeof(struct parse_tree));
                memcpy(
                  new_tree.binop_tree.left, tree, sizeof(struct parse_tree));
                new_tree.binop_tree.right = right;
                *tree = new_tree;
            } else
                break;
        } else
            break;
    }
    return true;
}

// basically the equ_expression rule
bool
build_entire_expression(struct parser* p, struct parse_tree* tree)
{
    if (build_atom(p, tree))
        return build_nth_entire_expression(p, tree, 0);
    log_debug("atom failed to build");
    return false;
}

void
print_entire_expression(struct parse_tree* tree)
{
    if (tree == NULL) {
        printf("wtf??");
        return;
    }
    switch (tree->type) {
        case INT: {
            printf("%d", tree->int_tree.value);
            return;
        }
        case VARIABLE: {
            printf("%s", tree->var_tree.value);
            return;
        }
        case BINARY_OP: {
            printf("(%d ", tree->binop_tree.type);
            print_entire_expression(tree->binop_tree.left);
            putchar(' ');
            print_entire_expression(tree->binop_tree.right);
            putchar(')');
            return;
        }
        default: {
            printf("unknown");
            return;
        }
    }
}
