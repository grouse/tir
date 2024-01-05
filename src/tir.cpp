#include "core.h"
#include "memory.h"
#include "file.h"
#include "lexer.h"

#include <cstdlib>
#include <stdio.h>

void print_usage()
{
    printf("Usage: tir [options] <source>\n");
    printf("\n");
    printf("Options:\n");
    printf("  -h, --help     Print this help message\n");
    printf("\n");
    printf("Arguments:\n");
    printf("  <source>       The source file to compile\n");
}

enum Keyword : i32 {
    KW_INVALID = 0,
    KW_RETURN,
};

enum ASTType : i32 {
    AST_INVALID = 0,

    AST_RETURN,
    AST_PROCEDURE,
    AST_LITERAL,
    AST_STATEMENT_LIST,
    AST_BINARY_OP,
};

enum BinaryOp : i8 {
    OP_INVALID = 0,

    OP_ADD,
    // OP_SUB
    // OP_MUL,
    // OP_DIV,
    //
    // OP_AND,
    // OP_NAND,
    // OP_OR,
    // OP_NOR,
    // OP_XOR,
    //
    // OP_EQ,
    // OP_NEQ,
    // OP_LT,
    // OP_LTE,
    // OP_GT,
    // OP_GTE,
    //
    // OP_SHL,
    // OP_SHR,
};
const char* sz_from_enum(BinaryOp op)
{
    switch (op) {
    case OP_ADD:     return "+";
    case OP_INVALID: return "invalid";
    }
}


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
            BinaryOp op;
            AST *lhs;
            AST *rhs;
        } binary_op;
        struct {
            Token token;
        } literal;
        struct {
            AST *stmts;
        } stmt_list;
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
            LOG_INFO("%.*sbinary op %s", depth, indent, sz_from_enum(ast->binary_op.op));
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
        case AST_STATEMENT_LIST:
            LOG_INFO("%.*sstatements", depth, indent);
            for (AST *stmt = ast->stmt_list.stmts; stmt; stmt = stmt->next) {
                debug_print_ast(stmt, depth+1);
            }
            break;
        case AST_INVALID: break;
        }
    }
}

BinaryOp optional_parse_binary_op(Lexer *lexer)
{
    if (optional_token(lexer, '+')) return OP_ADD;
    return OP_INVALID;
}

UnaryOp optional_parse_unary_op(Lexer *)
{
    return UOP_INVALID;
}

AST* parse_subexpression(Lexer *lexer, Allocator mem)
{
    AST *expr = nullptr;
    if (optional_token(lexer, TOKEN_INTEGER)) {
        expr = ALLOC_T(mem, AST) {
            .type = AST_LITERAL,
            .literal.token = lexer->t,
        };
    } else {
        return nullptr;
    }

    if (BinaryOp op = optional_parse_binary_op(lexer); op) {
        AST *lhs = expr;
        AST *rhs = parse_subexpression(lexer, mem);

        expr = ALLOC_T(mem, AST) {
            .type = AST_BINARY_OP,
            .binary_op.op = op,
            .binary_op.lhs = lhs,
            .binary_op.rhs = rhs,
        };
    }

    return expr;
}

AST* parse_expression(Lexer *lexer, Allocator mem)
{
    AST *ast = parse_subexpression(lexer, mem);
    if (!require_next_token(lexer, ';')) {
        PARSE_ERROR(lexer, "unclosed expression");
        return nullptr;
    }

    return ast;
}

AST* parse_statement(Lexer *lexer, Allocator mem)
{
    if (optional_token(lexer, '{')) {
        AST *ast = ALLOC_T(mem, AST) {
            .type = AST_STATEMENT_LIST,
        };

        AST **ptr = &ast->stmt_list.stmts;
        while (*lexer && lexer->t != '}') {
            *ptr = parse_statement(lexer, mem);
            if (*ptr) ptr = &(*ptr)->next;
        }
        return ast;
    } else if (next_token(lexer) == TOKEN_IDENTIFIER) {
        Token identifier = lexer->t;

        if (i32 kw = keyword_from_string(identifier.str); kw != KW_INVALID) {
            AST *ast = nullptr;
            switch (kw) {
            case KW_RETURN:
                ast = ALLOC_T(mem, AST) {
                    .type = AST_RETURN,
                    .ret.expr = parse_expression(lexer, mem)
                };
                break;
            }

            return ast;
        }
    }

    return nullptr;

}

int main(int argc, char *argv[])
{
    for (auto &c : indent) c = ' ';
    constexpr i32 MAX_AST_MEM = 10*MiB;

    init_default_allocators();

    char *src = nullptr;
    for (i32 i = 0; i < argc; i++) {
        if (argv[i][0] == '-') {
            print_usage();
            return 0;
        } else {
            src = argv[i];
        }
    }

    if (!src) {
        print_usage();
        return -1;
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
                } else if (peek_token(&lexer) == ':' && peek_nth_token(&lexer, 2) == ':') {
                    if (peek_nth_token(&lexer, 3) == '(') {
                        next_nth_token(&lexer, 3);
                        if (!require_next_token(&lexer, ')')) return -1;

                        ast = ast->next = ALLOC_T(mem, AST) {
                            .type = AST_PROCEDURE,
                            .proc.identifier = identifier,
                            .proc.body = parse_statement(&lexer, mem),
                        };
                    }
                }
            }
        }

        debug_print_ast(root.next);
    }

    return 0;
}
