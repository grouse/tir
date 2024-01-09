#include "core.h"
#include "memory.h"
#include "file.h"
#include "lexer.h"

#include <cstdlib>
#include <stdio.h>
#include <string.h>

enum Keyword : i32 {
    KW_INVALID = 0,
    KW_RETURN,
};

enum ASTType : i32 {
    AST_INVALID = 0,

    AST_RETURN,
    AST_PROCEDURE,
    AST_LITERAL,
    AST_BINARY_OP,
};

enum UnaryOp : i8 {
    UOP_INVALID = 0,

    // UOP_NEG,
};
const char* sz_from_enum(UnaryOp op)
{
    switch (op) {
    case UOP_INVALID: return "invalid";
    }
}


struct AST {
    AST *next;

    ASTType type;
    union {
        struct {
            Token identifier;
            AST *body;
        } proc;
        struct {
            Token op;
            AST *lhs;
            AST *rhs;
        } binary_op;
        struct {
            Token token;
        } literal;
        struct {
            Token token;
            AST *expr;
        } ret;
    };
};

Keyword keyword_from_string(String str)
{
    if (str == "return") return KW_RETURN;
    return KW_INVALID;
}


static char indent[256];
void debug_print_ast(AST *ast, i32 depth = 0)
{
    for (; ast; ast = ast->next) {
        switch (ast->type) {
        case AST_LITERAL:
            LOG_INFO("%.*sliteral %.*s", depth, indent, STRFMT(ast->literal.token.str));
            break;
        case AST_BINARY_OP:
            LOG_INFO("%.*sbinary op %.*s", depth, indent, STRFMT(ast->binary_op.op.str));
            debug_print_ast(ast->binary_op.lhs, depth+1);
            debug_print_ast(ast->binary_op.rhs, depth+1);
            break;
        case AST_PROCEDURE:
            LOG_INFO("%.*sprocedure %.*s", depth, indent, STRFMT(ast->proc.identifier.str));
            if (ast->proc.body) debug_print_ast(ast->proc.body, depth+1);
            break;
        case AST_RETURN:
            LOG_INFO("%.*sreturn", depth, indent);
            if (ast->ret.expr) debug_print_ast(ast->ret.expr, depth+1);
            break;
        case AST_INVALID: break;
        }
    }
}

UnaryOp optional_parse_unary_op(Lexer *)
{
    return UOP_INVALID;
}

i32 operator_precedence(Token op)
{
    switch (op.type) {
    case '+':
    case '-':
        return 10;
    case '*':
    case '/':
        return 20;
    default:
        return 0;
    }
}

bool is_binary_op(Token t)
{
    switch (t.type) {
    case '+':
    case '-':
    case '*':
    case '/':
        return true;
    default:
        return false;
    }
}

AST* parse_expression(Lexer *lexer, Allocator mem, i32 min_prec = 0)
{
    AST *expr = nullptr;
    if (optional_token(lexer, TOKEN_INTEGER)) {
        expr = ALLOC_T(mem, AST) {
            .type = AST_LITERAL,
            .literal.token = lexer->t,
        };
    }

    while (*lexer) {
        Token op = peek_token(lexer);
        if (!is_binary_op(op)) break;

        i32 prec = operator_precedence(op);
        if (prec < min_prec) break;
        next_token(lexer);

        AST *lhs = expr;
        AST *rhs = parse_expression(lexer, mem, prec+1);

        expr = ALLOC_T(mem, AST) {
            .type = AST_BINARY_OP,
            .binary_op.op = op,
            .binary_op.lhs = lhs,
            .binary_op.rhs = rhs,
        };
    }

    return expr;
}

AST* parse_statement(Lexer *lexer, Allocator mem)
{
    if (optional_token(lexer, '{')) {
        AST *ast = parse_statement(lexer, mem);
        if (!ast) return nullptr;

        while (*lexer && peek_token(lexer) != '}') {
            ast->next = parse_statement(lexer, mem);
            ast = ast->next;
        }

        if (!require_next_token(lexer, '}')) {
            PARSE_ERROR(lexer, "unclosed statement list");
            return nullptr;
        }

        return ast;
    } else if (optional_token(lexer, TOKEN_IDENTIFIER)) {
        Token identifier = lexer->t;

        if (i32 kw = keyword_from_string(identifier.str); kw != KW_INVALID) {
            AST *ast = nullptr;
            switch (kw) {
            case KW_RETURN:
                ast = ALLOC_T(mem, AST) {
                    .type = AST_RETURN,
                    .ret.expr = parse_expression(lexer, mem)
                };

                if (!require_next_token(lexer, ';')) {
                    PARSE_ERROR(lexer, "expected ';' after return statement");
                    return nullptr;
                }
                break;
            }

            return ast;
        } else {
            PARSE_ERROR(lexer, "unexpected identifier '%.*s'", STRFMT(lexer->t.str));
            return nullptr;
        }
    }

    return nullptr;
}

void emit_ast_x64(StringBuilder *sb, AST *ast);

void emit_ast_x64_mov(StringBuilder *sb, String dst, AST *src)
{
    if (src->type == AST_LITERAL) {
        append_stringf(sb, "mov %.*s, %.*s\n", STRFMT(dst), STRFMT(src->literal.token.str));
    } else if (src->type == AST_BINARY_OP) {
        emit_ast_x64(sb, src->binary_op.lhs);
        append_stringf(sb, "push eax\n");
        emit_ast_x64(sb, src->binary_op.rhs);
        append_stringf(sb, "pop ebx\n");

        switch (src->binary_op.op.type) {
        case '+': append_stringf(sb, "add eax, ebx\n"); break;
        case '-': append_stringf(sb, "sub eax, ebx\n"); break;
        case '*': append_stringf(sb, "imul eax, ebx\n"); break;
        case '/': append_stringf(sb, "idiv ebx\n"); break;
        default: LOG_ERROR("Invalid binary op '%.*s'", STRFMT(src->binary_op.op.str)); break;
        }

        if (dst != "eax") append_stringf(sb, "mov %.*s, eax\n", STRFMT(dst));
    } else {
        LOG_ERROR("Invalid AST node type '%d'", src->type);
    }
}

void emit_ast_x64(StringBuilder *sb, AST *ast)
{
    while (ast) {
        switch (ast->type) {
        case AST_LITERAL:
            append_stringf(sb, "mov eax, %.*s\n", STRFMT(ast->literal.token.str));
            break;
        case AST_PROCEDURE:
            append_stringf(sb, "%.*s:\n", STRFMT(ast->proc.identifier.str));
            emit_ast_x64(sb, ast->proc.body);
            break;
        case AST_BINARY_OP:
            emit_ast_x64_mov(sb, "eax", ast);
            break;
        case AST_RETURN:
            if (ast->ret.expr) emit_ast_x64_mov(sb, "eax", ast->ret.expr);
            append_stringf(sb, "ret\n");
            break;
        default: LOG_ERROR("Invalid AST node type '%d'", ast->type); break;
        }
        ast = ast->next;
    }
}

void print_usage()
{
    printf("Usage: tir <file> [options]\n");
    printf("Options:\n");
    printf("  -h, --help  Print this message\n");
    printf("  -o <file>   Output file\n");
    printf("\n");
}

int main(int argc, char *argv[])
{
    for (auto &c : indent) c = ' ';
    constexpr i32 MAX_AST_MEM = 10*MiB;

    init_default_allocators();

    char *src = nullptr;
    char *output = nullptr;

    for (i32 i = 0; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (argv[i][1] == 'h' || strcmp(&argv[i][1], "-help") == 0) {
                print_usage();
                return 0;
            } else if (argv[i][1] == 'o' || strcmp(&argv[i][1], "-out") == 0) {
                if (i+1 >= argc) {
                    LOG_ERROR("Expected output file after '-o'");
                    return -1;
                }

                output = argv[++i];
            } else {
                LOG_ERROR("Unknown option '%s'", argv[i]);
                return -1;
            }
        } else {
            src = argv[i];
        }
    }

    if (!src) {
        print_usage();
        return -1;
    }

    if (!output) {
        i32 src_len = strlen(src);
        output = (char*)malloc(src_len+3);
        memcpy(output, src, src_len);

        char *p = strrchr(output, '.');
        if (!p) p = output + src_len;
        memcpy(p, ".s", 3);
    }

    {
        String file = string(src);
        SArena scratch = tl_scratch_arena();
        FileInfo f = read_file(file, scratch);
        if (!f.data) {
            LOG_ERROR("Failed to read file '%.*s'", STRFMT(file));
            return -1;
        }

        Allocator mem = tl_linear_allocator(MAX_AST_MEM);

        AST root{};
        AST *ast = &root;

        Lexer lexer{ f.data, f.size, file };

        while (next_token(&lexer)) {
            if (lexer.t.type == TOKEN_IDENTIFIER) {
                Token identifier = lexer.t;

                if (i32 kw = keyword_from_string(identifier.str); kw != KW_INVALID) {
                    switch (kw) {
                    case KW_RETURN:
                        PARSE_ERROR(&lexer, "unexpected return statement");
                        break;
                    }
                } else if (peek_token(&lexer) == ':' &&
                           peek_nth_token(&lexer, 2) == ':')
                {
                    if (peek_nth_token(&lexer, 3) == '(') {
                        next_nth_token(&lexer, 3);

                        if (!require_next_token(&lexer, ')')) {
                            PARSE_ERROR(&lexer, "expected ')'");
                            break;
                        }

                        ast = ast->next = ALLOC_T(mem, AST) {
                            .type = AST_PROCEDURE,
                            .proc.identifier = identifier,
                            .proc.body = parse_statement(&lexer, mem),
                        };
                    }
                } else {
                    PARSE_ERROR(&lexer, "unexpected identifier '%.*s'", STRFMT(identifier.str));
                    return -1;
                }
            } else {
                PARSE_ERROR(&lexer, "unknown token '%.*s'", STRFMT(lexer.t.str));
                return -1;
            }
        }

        debug_print_ast(root.next);

        StringBuilder sb = { .alloc = scratch };
        append_string(&sb, ".globl main\n");
        emit_ast_x64(&sb, root.next);
        write_file(string(output), &sb);
    }

    return 0;
}
