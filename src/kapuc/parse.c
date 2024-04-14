#include "parse.h"

#include "lex.h"
#include "lib/log.h"
#include "lib/sds.h"
#include "lib/stb_ds.h"

#include <stdio.h>
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
get_next_tok_by_n(struct parser* p, struct TOK* tok, int n)
{
    if (arrlen(p->tokens) > p->pos + n) {
        *tok = p->tokens[p->pos + n];
        return true;
    }
    return false;
}

static inline bool
get_next_tok(struct parser* p, struct TOK* tok)
{
    return get_next_tok_by_n(p, tok, 1);
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
build_entire_expression(struct parser* p, struct parse_tree* tree);

static bool
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
              atol(t.s); // NOLINT(cert-err34-c) atol works here
                         // because we know it is an integer
            advance(p);
            return true;
        }
        case LPAREN: {
            advance(p);
            if (build_entire_expression(p, tree)) {
                if (get_cur_tok(p, &t) && t.t == RPAREN)
                    advance(p);
                else
                    return false;
                return true;
            }
            log_debug("failed to build PARENTHESIS expression");
            return false;
        }
        case IDENT: {
            // TODO: check for '.', '::', '()', etc. basically check for
            // func call
            tree->type = VARIABLE;
            tree->var_tree.value = t.s;
            advance(p);
            return true;
        }
        case AMPERSAND: {
            struct TOK t2 = {};
            if (get_next_tok(p, &t2) &&
                (t2.t == NUM || t2.t == IDENT || t2.t == AMPERSAND)) {
                advance(p);
                tree->type = PTR_REF;
                tree->ref_tree.value = malloc(sizeof(struct parse_tree));
                build_atom(p, tree->ref_tree.value);
                return true;
            }
            return false;
        }
        case STAR: {
            struct TOK t2 = {};
            if (get_next_tok(p, &t2) &&
                (t2.t == NUM || t2.t == IDENT || t2.t == AMPERSAND)) {
                advance(p);
                tree->type = PTR_DEREF;
                tree->deref_tree.value = malloc(sizeof(struct parse_tree));
                build_atom(p, tree->ref_tree.value);
                return true;
            }
            return false;
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
                    log_debug("get next_op at tok pos %d, real pos %d",
                              p->pos,
                              next_op.start);
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
                memmove(new_tree.binop_tree.left,
                        tree,
                        sizeof(struct parse_tree)); // this is so horrible i
                                                    // should meg myself now
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
        printf("%ld", tree->int_tree.value);
        return;
    }
    case PTR_REF: {
        putchar('&');
        print_entire_expression(tree->ref_tree.value);
        return;
    }
    case PTR_DEREF: {
        putchar('*');
        print_entire_expression(tree->ref_tree.value);
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

void
free_parse_tree(struct parse_tree* tree)
{
    if (tree == NULL)
        return;
    switch (tree->type) {
    case VARIABLE: {
        // sdsfree(tree->var_tree.value);
        // not freed here because we do that later at cleanup in main.
        break;
    }
    case PTR_REF: {
        free_parse_tree(tree->ref_tree.value);
        break;
    }
    case PTR_DEREF: {
        free_parse_tree(tree->deref_tree.value);
        break;
    }
    case BINARY_OP: {
        free_parse_tree(tree->binop_tree.left);
        free_parse_tree(tree->binop_tree.right);
        break;
    }
    default:
        break;
    }
    free(tree);
}
