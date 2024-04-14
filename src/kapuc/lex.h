#include "lib/sds.h"

#include <stdio.h>

#ifndef KAPUC_LEX_H
#define KAPUC_LEX_H

enum TOK_TYPE
{
    FUNC,
    IF,
    ELSE,
    ELIF,
    LET,
    CONST,
    IDENT,
    TRUE,
    FALSE,
    COFFEE,
    STRING,
    NUM,
    RETURN,
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    LBRACKET,
    RBRACKET,
    COLON,
    DOUBLE_COLON,
    COMMA,
    SEMICOLON,
    DOT,
    DOTDOT,
    AMPERSAND,
    EXCLAM,
    PLUS,
    MINUS,
    STAR,
    SLASH,
    EQ,
    COMP_EQ,
    COMP_NEQ,
    LA_EQ,
    RA_EQ,
    RIGHT_ANGLE,
    LEFT_ANGLE,
    T_ERR
};

struct __attribute__((packed)) TOK
{
    sds s;
    enum TOK_TYPE t;
    long start;
    long end;
};

struct TOK*
lex(FILE* stream);

#endif // KAPUC_LEX_H
