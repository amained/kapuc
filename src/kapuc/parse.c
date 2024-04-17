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
build_atom(struct parser* p, struct parse_tree* tree);

// basically build atom without suffix (type props, factorial, etc)
static inline bool
build_small_atom(struct parser* p, struct parse_tree* tree)
{
    struct TOK t = {};
    if (get_cur_tok(p, &t)) {
        switch (t.t) {
        case NUM: {
            tree->type = INT;
            // TODO: The lexer get the float here too? not sure. maybe we
            // should make the lexer just get the number don't care about
            // float?

            log_debug("ok??");
            tree->int_tree.value =
              atol(t.s); // NOLINT(cert-err34-c) atol works here
                         // because we know it is an integer
            log_debug("%ld", tree->int_tree.value);
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

static bool
build_atom(struct parser* p, struct parse_tree* tree)
{
    if (build_small_atom(p, tree)) {
        struct TOK t;
        if (get_cur_tok(p, &t)) {
            switch (t.t) {
            case DOT: {
                advance(p);
                log_debug("so true!");
                struct parse_tree new_tree;
                new_tree.type = TYPE_TRAIL;
                new_tree.trail.trail_type = 0;
                new_tree.trail.current = malloc(sizeof(struct parse_tree));
                memmove(new_tree.trail.current,
                        tree,
                        sizeof(struct parse_tree)); // megmove count: 2
                new_tree.trail.next = malloc(sizeof(struct parse_tree));
                build_atom(p, new_tree.trail.next);
                *tree = new_tree;
                return true;
            }
            case DOUBLE_COLON: {
                advance(p);
                struct parse_tree new_tree;
                new_tree.type = TYPE_TRAIL;
                new_tree.trail.trail_type = 1;
                new_tree.trail.current = malloc(sizeof(struct parse_tree));
                memmove(new_tree.trail.current,
                        tree,
                        sizeof(struct parse_tree)); // megmove count: 3
                new_tree.trail.next = malloc(sizeof(struct parse_tree));
                build_atom(p, new_tree.trail.next);
                *tree = new_tree;
                return true;
            }
            default: {
                return true; // some foreign shit we don't have to handle
            }
            }
        }
        return true;
    }
    return true;
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
    case PLUS_EQ:
    case MINUS_EQ:
        return 3;
    case MULT_EQ:
    case DIV_EQ:
        return 4;
    case COMP_EQ:
    case COMP_NEQ:
    case LEFT_ANGLE:
    case RIGHT_ANGLE:
        return 5;
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
    case COMP_EQ:
        return 4;
    case COMP_NEQ:
        return 5;
    case LEFT_ANGLE:
        return 6;
    case RIGHT_ANGLE:
        return 7;
    case PLUS_EQ:
        return 8;
    case MINUS_EQ:
        return 9;
    case MULT_EQ:
        return 10;
    case DIV_EQ:
        return 11;
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
    log_debug("atom failed to build, pos: %d", p->pos);
    return false;
}

static inline bool
build_assignment_expression(struct parser* p, struct parse_tree* tree)
{
    struct TOK t;
    tree->assign_tree.type = malloc(sizeof(struct parse_tree));
    if (build_atom(p, tree->assign_tree.type)) {
        if (get_cur_tok(p, &t) && t.t == IDENT) {
            advance(p);
            tree->assign_tree.name = t.s;
            if (get_cur_tok(p, &t) && t.t == SEMICOLON)
                return true;
            // stmt, check for equal sign
            if (t.t == EQ) {
                advance(p);
                tree->assign_tree.value = malloc(sizeof(struct parse_tree));
                if (build_entire_expression(p, tree->assign_tree.value)) {
                    if (get_cur_tok(p, &t) && t.t == SEMICOLON) {
                        advance(p);
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

// basically the level_stmt
bool
build_block_statement(struct parser* p, struct parse_tree* tree)
{
    struct TOK t;
    if (get_cur_tok(p, &t)) {
        switch (t.t) {
        case CONST: {
            advance(p);
            tree->type = STMT_ASSIGNMENT;
            tree->assign_tree.isConst = true;
            return build_assignment_expression(p, tree);
        }
        case LET: {
            advance(p);
            tree->type = STMT_ASSIGNMENT;
            tree->assign_tree.isConst = false;
            return build_assignment_expression(p, tree);
        }
        case RETURN: {
            advance(p);
            tree->type = STMT_RETURN;
            tree->return_tree.value = malloc(sizeof(struct parse_tree));
            if (build_entire_expression(p, tree->return_tree.value)) {
                if (get_cur_tok(p, &t) && t.t == SEMICOLON) {
                    advance(p);
                    return true;
                }
            }
            return false;
        }
        case IF: {
            advance(p);
            tree->type = IFS;
            tree->ifs_tree.condition = malloc(sizeof(struct parse_tree));
            if (build_entire_expression(p, tree->ifs_tree.condition)) {
                tree->ifs_tree.value = malloc(sizeof(struct parse_tree));
                if (build_block(p, tree->ifs_tree.value))
                    return true;
            }
            return false;
        }
        case FOR: {
            advance(p);
            tree->type = LOOP_FOR;
            tree->loop_for_tree.start = malloc(sizeof(struct parse_tree));
            if (build_block(p, tree->loop_for_tree.start)) {
                tree->loop_for_tree.while_cond =
                  malloc(sizeof(struct parse_tree));

                if (build_block(p, tree->loop_for_tree.while_cond)) {
                    tree->loop_for_tree.end = malloc(sizeof(struct parse_tree));

                    if (build_block(p, tree->loop_for_tree.end)) {
                        // condition parsing finished; build block
                        tree->loop_for_tree.inside =
                          malloc(sizeof(struct parse_tree));
                        if (build_block(p, tree->loop_for_tree.inside))
                            return true;
                    }
                }
            }
            return false;
        }
        case SEMICOLON: {
            advance(p);
            log_debug("excess semicolon??"); // currently excess semis still a problem
            return false;
        }
        default: {
            // expression? assignment?
            if (t.t == IDENT && get_next_tok(p, &t) && t.t == EQ) {
                // assignment
                get_cur_tok(p, &t);
                tree->type = STMT_ASSIGNMENT;
                tree->assign_tree.name = t.s;
                tree->assign_tree.type = NULL;
                advance(p);
                advance(p);
                tree->assign_tree.value = malloc(sizeof(struct parse_tree));
                if (build_entire_expression(p, tree->assign_tree.value)) {
                    if (get_cur_tok(p, &t) && t.t == SEMICOLON) {
                        advance(p);
                        return true;
                    }
                }
                return false;
            }
            if (build_entire_expression(p, tree)) {
                if (get_cur_tok(p, &t) && t.t == SEMICOLON) {
                    advance(p);
                    return true;
                }
            }
        }
        }
        return false;
    }
    return false;
}

bool
build_block(struct parser* p, struct parse_tree* tree)
{
    // incase of one statement, we just return the statement
    // if there's more, we will return tree_level_stmts
    struct TOK t;
    if (get_cur_tok(p, &t)) {
        switch (t.t) {
        case LBRACE: {
            advance(p);
            if (get_cur_tok(p, &t) && t.t == RBRACE) {
                // empty statement, retreat;
                tree->type = EMPTY_LVL_STMTS;
                return true;
            }
            tree->type = LVL_STMTS;
            // FIXME: VERY UNOPTIMIZED, THIS SHOULD BE REMODELED
            // unfortunately my brain suck at thinking better method
            // maybe reverse linked list?? that would suck to get first
            // statement
            while (get_cur_tok(p, &t) && t.t != RBRACE) {
                tree->type = LVL_STMTS;
                tree->level_stmts_tree.statement =
                  malloc(sizeof(struct parse_tree));
                if (!build_block_statement(p, tree->level_stmts_tree.statement))
                    return false;
                tree->level_stmts_tree.next = NULL;
                if (get_cur_tok(p, &t) && t.t != RBRACE) {
                    tree->level_stmts_tree.next =
                      malloc(sizeof(struct parse_tree));
                    tree = tree->level_stmts_tree.next;
                }
            }
            return true;
        }
        default:
            return build_block_statement(p, tree); // foreign stuff
        }
    }
    return false;
}

void
print_entire_expression(struct parse_tree* tree)
{
    if (tree == NULL) {
        fputs("NUL", stdout);
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
        fputs(tree->var_tree.value, stdout);
        return;
    }
    case LOOP_FOR: {
        fputs("for ", stdout);
        print_entire_expression(tree->loop_for_tree.start);
        putchar(' ');
        print_entire_expression(tree->loop_for_tree.while_cond);
        putchar(' ');
        print_entire_expression(tree->loop_for_tree.end);
        fputs("->", stdout);
        print_entire_expression(tree->loop_for_tree.inside);
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
    case STMT_ASSIGNMENT: {
        printf("%s type: ", tree->assign_tree.isConst ? "const" : "let");
        print_entire_expression(tree->assign_tree.type);
        fputs(" value: ", stdout);
        print_entire_expression(tree->assign_tree.value);
        putchar(';');
        return;
    }
    case STMT_RETURN: {
        fputs("return ", stdout);
        print_entire_expression(tree->return_tree.value);
        putchar(';');
        return;
    }
    case LVL_STMTS: {
        fputs("stmts: {", stdout);
        print_entire_expression(tree->level_stmts_tree.statement);
        if (tree->level_stmts_tree.next != NULL) {
            fputs("->", stdout);
            print_entire_expression(tree->level_stmts_tree.next);
        }
        putchar('}');
        return;
    }
    case TYPE_TRAIL: {
        print_entire_expression(tree->trail.current);
        switch (tree->trail.trail_type) {
        case 0: {
            putchar('-');
        }
        case 1: {
            fputs("::", stdout);
        }
        }
        print_entire_expression(tree->trail.next);
        return;
    }
    default: {
        printf("unknown %d", tree->type);
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
    case TYPE_TRAIL: {
        free_parse_tree(tree->trail.current);
        free_parse_tree(tree->trail.next);
        break;
    }
    case STMT_ASSIGNMENT: {
        free_parse_tree(tree->assign_tree.type);
        free_parse_tree(tree->assign_tree.value);
        break;
    }
    case STMT_RETURN: {
        free_parse_tree(tree->return_tree.value);
        break;
    }
    default:
        break;
    }
    free(tree);
}
