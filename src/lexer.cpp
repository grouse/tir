#include "lexer.h"

Token next_token(Lexer *lexer, u32 flags) EXPORT
{
    Token &t = lexer->t;
    t.l0 = lexer->line;
    t.c0 = lexer->col;

    while (*lexer) {
        if (lexer->ptr[0] == ' ' || lexer->ptr[0] == '\t') {
            t.type = TOKEN_WHITESPACE;
            t.str.data = lexer->ptr++;
            lexer->col++; // TODO: utf8

            while (lexer->ptr < lexer->end &&
                   (lexer->ptr[0] == ' ' || lexer->ptr[0] == '\t'))
            {
                lexer->ptr++;
                lexer->col++; // TODO: utf8
            }

            t.str.length = (i32)(lexer->ptr - t.str.data);
            if (flags & LEXER_WHITESPACE) return t;
        } else if (lexer->ptr[0] == '\n' || lexer->ptr[0] == '\r') {
            t.type = TOKEN_NEWLINE;
            t.str.data = lexer->ptr;

            lexer->line++; lexer->col = 0;
            if (lexer->ptr[0] == '\r') lexer->ptr++;
            if (lexer->ptr[0] == '\n') lexer->ptr++;

            t.str.length = (i32)(lexer->ptr - t.str.data);
            if (flags & LEXER_NEWLINE) return t;
        } else if ((*lexer->ptr >= '0' && *lexer->ptr <= '9') ||
                   (lexer->ptr+1 < lexer->end && *lexer->ptr == '-' && *(lexer->ptr+1) >= '0' && *(lexer->ptr+1) <= '9'))
        {
            t.type = TOKEN_INTEGER;
            t.str.data = lexer->ptr++;
            lexer->col++; // TODO: utf8

            while (lexer->ptr < lexer->end) {
                if (t.type == TOKEN_INTEGER && *lexer->ptr == '.') {
                    t.type = TOKEN_NUMBER;
                } else if (*lexer->ptr > '9' || *lexer->ptr < '0') break;
                lexer->ptr++;
                lexer->col++; // TODO: utf8
            }

            t.str.length = (i32)(lexer->ptr - t.str.data);
            return t;
        } else if ((*lexer->ptr >= 'a' && *lexer->ptr <= 'z') ||
                   (*lexer->ptr >= 'A' && *lexer->ptr <= 'Z') ||
                   (u8)(*lexer->ptr) > 127)
        {
            t.type = TOKEN_IDENTIFIER;
            t.str.data = lexer->ptr++;
            lexer->col++; // TODO: utf8

            while (lexer->ptr < lexer->end) {
                if ((*lexer->ptr < 'a' || *lexer->ptr > 'z') &&
                    (*lexer->ptr < 'A' || *lexer->ptr > 'Z') &&
                    (*lexer->ptr < '0' || *lexer->ptr > '9') &&
                    *lexer->ptr != '_' && (u8)(*lexer->ptr) <= 127)
                {
                    break;
                }

                lexer->ptr++;
                lexer->col++; // TODO: utf8
            }

            t.str.length = (i32)(lexer->ptr - t.str.data);
            return t;
        } else {
            t.type = (TokenType)*lexer->ptr;
            t.str = { lexer->ptr, 1 };
            lexer->ptr++;
            lexer->col++; // TODO: utf8
            return t;
        }
    }

    t = { TOKEN_EOF, { lexer->ptr, 0 } };
    return t;
}

Token next_nth_token(Lexer *lexer, i32 n, u32 flags) EXPORT
{
    while (n-- > 0) next_token(lexer, flags);
    return lexer->t;
}

Token peek_token(Lexer *lexer, u32 flags) EXPORT
{
    Lexer copy = *lexer;
    return next_token(&copy, flags);
}

Token peek_nth_token(Lexer *lexer, i32 n, u32 flags) EXPORT
{
    Lexer copy = *lexer;
    while (n-- > 0) next_token(&copy, flags);
    return copy.t;
}

Token eat_until(Lexer *lexer, TokenType terminator, u32 flags) EXPORT
{
    Token t;
    do t = next_token(lexer, flags);
    while (t.type != TOKEN_EOF && t.type != terminator);
    return t;
}

bool require_next_token(Lexer *lexer, TokenType type, Token *t /*= nullptr */) EXPORT
{
    next_token(lexer);
    if (t) *t = lexer->t;
    if (lexer->t.type == type) return true;
    if (lexer->t.type == TOKEN_INTEGER && type == TOKEN_NUMBER) return true;
    return false;

}

bool require_next_token(Lexer *lexer, char c, Token *t /*= nullptr */) EXPORT
{
    next_token(lexer);
    if (t) *t = lexer->t;
    if (lexer->t.type == (TokenType)c) return true;
    return false;
}

bool optional_token(Lexer *lexer, TokenType type, Token *t /*= nullptr */) EXPORT
{
    Token lh = peek_token(lexer);
    if (lh.type != type && (lh.type != TOKEN_INTEGER || type != TOKEN_NUMBER)) return false;

    next_token(lexer);
    if (t) *t = lexer->t;
    return true;
}

bool optional_token(Lexer *lexer, char c, Token *t /*= nullptr */) EXPORT
{
    Token lh = peek_token(lexer);
    if (lh.type != (TokenType)c) return false;

    next_token(lexer);
    if (t) *t = lexer->t;
    return true;
}

bool optional_identifier(Lexer *lexer, String str, Token *t /*= nullptr */)
{
    if (peek_token(lexer) == str) {
        next_token(lexer);
        if (t) *t = lexer->t;
        return true;
    }

    return false;
}

bool is_identifier(Token t, String str) EXPORT
{
    return t.type == TOKEN_IDENTIFIER && t.str == str;
}

bool parse_version_decl(Lexer *lexer, i32 *version_out, i32 max_version) EXPORT
{
    if (!require_next_token(lexer, '#', &lexer->t)) return false;
    if (!require_next_token(lexer, TOKEN_IDENTIFIER, &lexer->t)) return false;
    if (lexer->t != "version") return false;

    Token version_token;
    if (!require_next_token(lexer, TOKEN_INTEGER, &version_token)) return false;
    i32 version = i32_from_string(version_token.str);

    if (version < 0) {
        PARSE_ERROR(lexer, "version cannot be a negative number");
        return false;
    }

    if (version > max_version) {
        PARSE_ERROR(lexer, "version (%d) is higher than currently supported (%d)", version, max_version);
        return false;
    }

    *version_out = version;
    return true;
}

bool parse_float(Lexer *lexer, f32 *value) EXPORT
{
    if (!require_next_token(lexer, TOKEN_NUMBER)) return false;
    if (!f32_from_string(lexer->t.str, value)) return false;
    return true;
}

bool parse_floatn(Lexer *lexer, i32 n, f32 *value) EXPORT
{
    for (i32 i = 0; i < n; i++) {
        if (!require_next_token(lexer, TOKEN_NUMBER)) return false;
        if (!f32_from_string(lexer->t.str, &value[i])) return false;
    }
    return true;
}

bool parse_int(Lexer *lexer, i32 *value) EXPORT
{
    if (!require_next_token(lexer, TOKEN_INTEGER)) return false;
    if (!i32_from_string(lexer->t.str, value)) return false;
    return true;
}

bool parse_string(Lexer *lexer, String *str) EXPORT
{
    *str = String{};

    if (next_token(lexer) == '"') {
        str->data = lexer->t.str.data+1;
        if (!eat_until(lexer, '"')) {
            PARSE_ERROR(lexer, "parsing string; expected '\"', got '%.*s'", STRFMT(lexer->t.str));
            return false;
        }
        str->length = (i32)((i64)lexer->t.str.data - (i64)str->data);
    } else {
        *str = lexer->t.str;
        while (lexer->t != TOKEN_EOF && peek_token(lexer) != ';') {
            next_token(lexer);
            str->length = (i32)((i64)lexer->ptr - (i64)str->data);
        }
    }

    return true;
}

bool parse_string(Lexer *lexer, String *str, Allocator mem) EXPORT
{
    String s;
    if (!parse_string(lexer, &s)) return false;
    *str = duplicate_string(s, mem);
    return true;
}

bool parse_bool(Lexer *lexer, bool *value) EXPORT
{
    if (!require_next_token(lexer, TOKEN_IDENTIFIER)) return false;

    if (lexer->t == "true") *value = true;
    else if (lexer->t == "false") *value = false;
    else {
        PARSE_ERROR(lexer, "parsing boolean; expected 'true' or 'false', got '%.*s'", STRFMT(lexer->t.str));
        return false;
    }

    return true;
}
