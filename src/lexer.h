#ifndef LEXER_H
#define LEXER_H

#include "platform.h"
#include "string.h"
#include "core.h"

#define PARSE_ERROR(lexer, fmt, ...)\
    LOG_ERROR("parse error: %.*s:%d:%d: " fmt, STRFMT((lexer)->debug_name), (lexer)->t.l0+1, (lexer)->t.c0+1, ##__VA_ARGS__)

enum TokenType : u16 {
    TOKEN_ADD = '+',
    TOKEN_SUB = '-',
    TOKEN_MUL = '*',
    TOKEN_DIV = '/',

    TOKEN_START = 255, // NOTE(jesper): 0-255 reserved for ascii token values

    TOKEN_IDENTIFIER,
    TOKEN_INTEGER,
    TOKEN_NUMBER,

    TOKEN_WHITESPACE, // automatically eaten unless LEXER_WHITESPACE
    TOKEN_NEWLINE,    // automatically eaten unless LEXER_NEWLINE

    TOKEN_EOF,
};

struct Token {
    TokenType type;
    String str;

    i32 l0, c0;

    bool operator==(String str) const { return this->str == str; }
    bool operator==(char c) const { return type == (TokenType)c; }
    bool operator==(TokenType type) const { return this->type == type; }

    operator bool() const { return type != TOKEN_EOF; }
};

enum LexerFlags : u8 {
    LEXER_NEWLINE    = 1 << 0,
    LEXER_WHITESPACE = 1 << 1,

    LEXER_ALL        = 0xFF,
};

struct Lexer {
    char *ptr;
    char *end;

    Token t;

    String debug_name;
    i32 line;
    i32 col;

    u32 flags;

    Lexer(u8 *data, i32 size, String debug_name, u32 flags = 0)
        : ptr((char*)data), end((char*)data + size), debug_name(debug_name), flags(flags)
    {}

    Lexer(String str, String debug_name, u32 flags = 0)
        : ptr(str.data), end(str.data + str.length), debug_name(debug_name), flags(flags)
    {}

    operator bool() const { return ptr < end; }
};

inline const char* sz_from_enum(TokenType type)
{
    static char c[2] = { 0, 0 };

    switch (type) {
    case TOKEN_IDENTIFIER: return "IDENTIFIER";
    case TOKEN_INTEGER:    return "INTEGER";
    case TOKEN_NUMBER:     return "NUMBER";
    case TOKEN_WHITESPACE: return "WHITESPACE";
    case TOKEN_NEWLINE:    return "NEWLINE";
    case TOKEN_EOF:        return "EOF";
    default:
        c[0] = (char)type;
        return &c[0];
    }
}

#include "gen/lexer.h"

inline Token next_token(Lexer *lexer) { return next_token(lexer, lexer->flags); }
inline Token peek_token(Lexer *lexer) { return peek_token(lexer, lexer->flags); }

inline Token next_nth_token(Lexer *lexer, i32 n) { return next_nth_token(lexer, n, lexer->flags); }
inline Token peek_nth_token(Lexer *lexer, i32 n) { return peek_nth_token(lexer, n, lexer->flags); }

inline Token eat_until(Lexer *lexer, char terminator, u32 flags) { return eat_until(lexer, (TokenType)terminator, flags); }
inline Token eat_until(Lexer *lexer, TokenType terminator) { return eat_until(lexer, terminator, lexer->flags); }
inline Token eat_until(Lexer *lexer, char terminator) { return eat_until(lexer, (TokenType)terminator, lexer->flags); }

#endif // LEXER_H
