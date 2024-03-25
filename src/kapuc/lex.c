#include "lib/stb_ds.h"
#include "lib/sds.h"
#include "lex.h"
#include "lib/log.h"

// Some helper macro
#ifdef AGGRESSIVE_DEBUG
#define TOK_PUSH(c, b, t) struct TOK new_tok = {c, t}; \
log_debug("tokenized %s as type %d", c, t);          \
arrput(b, new_tok);
#else
#define TOK_PUSH(c, b, t, start, end) struct TOK new_tok = {c, t, start, end}; \
arrput(b, new_tok);
#endif

#define TOK_SYMBOL(b, c, d, t, p, l) if (strcmp(c, d) == 0) \
{                                                     \
sdsfree(c);                                                      \
TOK_PUSH(NULL, b, t, p - l, p - 1)                                     \
continue;                                             \
}

#define TOK_MULTICHAR_SYMBOL(b2, t1, t2, p) if ((c = fgetc(stream)) == b2) \
{                                                                       \
TOK_PUSH(NULL, pToks, t2, p-2, p-1)                                               \
} else {                                                                       \
fseek(stream, -1L, SEEK_CUR);                                                  \
TOK_PUSH(NULL, pToks, t1, p-1, p-1)                                                        \
}                                                                       \
break;

#define TOK_SYMBOLS_CHECK(b, c, p) \
TOK_SYMBOL(pToks, word, "func", FUNC, p, 4) \
TOK_SYMBOL(pToks, word, "return", RETURN, p, 6)                             \
TOK_SYMBOL(pToks, word, "if", IF, p, 2)                                     \
TOK_SYMBOL(pToks, word, "elif", ELIF, p, 4)                                 \
TOK_SYMBOL(pToks, word, "else", ELSE, p, 4)                                 \
TOK_SYMBOL(pToks, word, "let", LET, p, 3)                                   \
TOK_SYMBOL(pToks, word, "const", CONST, p, 5) \
TOK_SYMBOL(pToks, word, "if", IF, p, 2)     \
TOK_SYMBOL(pToks, word, "else", ELSE, p, 4) \
TOK_SYMBOL(pToks, word, "elif", ELIF, p, 4) \
TOK_SYMBOL(pToks, word, "true", TRUE, p, 4) \
TOK_SYMBOL(pToks, word, "false", FALSE, p, 5)

/// lex tokens from file stream
/// NOTE: Free the tokens yourself idiot
/// NOTE: Close the stream/file yourself too
struct TOK *lex(FILE *stream) {
    struct TOK *pToks = NULL;
    int c;
    while ((c = fgetc(stream)) != EOF) {
        if (c == EOF) break;
        if ((c >= 97 && c <= 122) || c == '_') {
            long start = ftell(stream) - 1;
            sds word = sdsnew((const char *) &c);
            while ((c = fgetc(stream)) != EOF && ((c >= 97 && c <= 122) || c == '_')) {
                word = sdscatlen(word, &c, 1);
            }
            if (c == EOF) {
                long p = ftell(stream);
                TOK_SYMBOLS_CHECK(pToks, word, p);
                TOK_PUSH(word, pToks, IDENT, start, p - 1);
                break;
            }
            fseek(stream, -1L, SEEK_CUR);
            long p = ftell(stream);
            TOK_SYMBOLS_CHECK(pToks, word, p);
            TOK_PUSH(word, pToks, IDENT, start, p - 1);
        } else if (c == ' ' || c == '\n' || c == '\t') continue;
        else if (c == '#') {
            while ((c = fgetc(stream)) != EOF && c != '\n');
        } else if (c >= 48 && c <= 57) {
            long start = ftell(stream);
            sds word = sdsnew((const char *) &c);
            while ((c = fgetc(stream)) != EOF && c >= 48 && c <= 57) {
                word = sdscatlen(word, &c, 1);
            }
            if (c == EOF) {
                TOK_PUSH(word, pToks, NUM, start, ftell(stream) -1);
                break;
            }
            fseek(stream, -1L, SEEK_CUR);
            TOK_PUSH(word, pToks, NUM, start, ftell(stream) -1);
        } else {
            switch (c) {
                case '(': {
                    TOK_PUSH(NULL, pToks, LPAREN, ftell(stream) - 1, ftell(stream) - 1)
                    break;
                }
                case ')': {
                    TOK_PUSH(NULL, pToks, RPAREN, ftell(stream) - 1, ftell(stream) - 1)
                    break;
                }
                case '{': {
                    TOK_PUSH(NULL, pToks, LBRACE, ftell(stream) - 1, ftell(stream) - 1)
                    break;
                }
                case '}': {
                    TOK_PUSH(NULL, pToks, RBRACE, ftell(stream) - 1, ftell(stream) - 1)
                    break;
                }
                case ';': {
                    TOK_PUSH(NULL, pToks, SEMICOLON, ftell(stream) - 1, ftell(stream) - 1)
                    break;
                }
                case ':': {
                    TOK_PUSH(NULL, pToks, COLON, ftell(stream) - 1, ftell(stream) - 1)
                    break;
                }
                case ',': {
                    TOK_PUSH(NULL, pToks, COMMA, ftell(stream) - 1, ftell(stream) - 1)
                    break;
                }
                case '+': {
                    TOK_PUSH(NULL, pToks, PLUS, ftell(stream) - 1, ftell(stream) - 1)
                    break;
                }
                case '-': {
                    TOK_PUSH(NULL, pToks, MINUS, ftell(stream) - 1, ftell(stream) - 1)
                    break;
                }
                case '*': {
                    TOK_PUSH(NULL, pToks, STAR, ftell(stream) - 1, ftell(stream) - 1)
                    break;
                }
                case '/': {
                    TOK_PUSH(NULL, pToks, SLASH, ftell(stream) - 1, ftell(stream) - 1)
                    break;
                }
                case '[': {
                    TOK_PUSH(NULL, pToks, LBRACKET, ftell(stream) - 1, ftell(stream) - 1)
                    break;
                }
                case ']': {
                    TOK_PUSH(NULL, pToks, RBRACKET, ftell(stream) - 1, ftell(stream) - 1)
                    break;
                }
                case '&': {
                    TOK_PUSH(NULL, pToks, AMPERSAND, ftell(stream) - 1, ftell(stream) - 1)
                    break;
                }
                case '"': {
                    sds word = sdsempty();
                    long start = ftell(stream) - 1;
                    while ((c = fgetc(stream)) != EOF && c != '"') {
                        word = sdscatlen(word, &c, 1);
                    }
                    if (c == EOF) {
                        TOK_PUSH(word, pToks, STRING, start, ftell(stream) -1);
                        break;
                    }
                    TOK_PUSH(word, pToks, STRING, start, ftell(stream) - 1)
                    break;
                }
                case '@': {
                    sds word = sdsempty();
                    long start = ftell(stream) - 1;
                    while ((c = fgetc(stream)) != EOF && ((c >= 97 && c <= 122) || c == '_')) {
                        word = sdscatlen(word, &c, 1);
                    }
                    if (c == EOF) {
                        break;
                    }
                    fseek(stream, -1L, SEEK_CUR);
                    TOK_PUSH(word, pToks, COFFEE, start, ftell(stream) -1);
                    break;
                }
                case '=': {
                    TOK_MULTICHAR_SYMBOL('=', EQ, COMP_EQ, ftell(stream));
                }
                case '<': {
                    TOK_MULTICHAR_SYMBOL('=', COMP_LT, COMP_LEQ, ftell(stream));
                }
                case '>': {
                    TOK_MULTICHAR_SYMBOL('=', COMP_MT, COMP_MEQ, ftell(stream));
                }
                case '!': {
                    TOK_MULTICHAR_SYMBOL('!', EXCLAM, COMP_NEQ, ftell(stream));
                }
                case '.': {
                    TOK_MULTICHAR_SYMBOL('.', DOT, DOTDOT, ftell(stream));
                }
                default: {
                    log_error("Expected symbol, got \"%c\" at %d", c, ftell(stream));
                    arrput(pToks, (struct TOK) {.t = T_ERR});
                }
            }
        }
    }
    return pToks;
}