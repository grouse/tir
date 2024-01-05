#include "core.h"
#include "memory.h"
#include "file.h"
#include "lexer.h"

#include <cstdlib>
#include <stdio.h>

struct {
} opts;

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

Keyword keyword_from_string(String str)
{
    if (str == "return") return KW_RETURN;
    return KW_INVALID;
}

enum ASTType : i32 {
    AST_INVALID = 0,
    AST_RETURN,
    AST_PROCEDURE,
    AST_STATEMENT_LIST,
};


struct AST {
    AST *next;

    ASTType type;
    union {
        struct {
            String name;
            AST *body;
        } proc;
        struct {
            AST *expr;
        } ret;
    };
};

struct ASTRoot {
    Allocator mem;
    AST ast;
};

static char indent[256];
void debug_print_ast(AST *ast, i32 depth = 0)
{
    for (; ast; ast = ast->next) {
        switch (ast->type) {
        case AST_PROCEDURE:
            LOG_INFO("%.*sprocedure %.*s", depth, indent, STRFMT(ast->proc.name));
            //if (ast->proc.params) debug_print_ast(ast->proc.params, depth+1);
            if (ast->proc.body) debug_print_ast(ast->proc.body, depth+1);
            break;
        case AST_RETURN:
            LOG_INFO("%.*sreturn", depth, indent);
            if (ast->ret.expr) debug_print_ast(ast->ret.expr, depth+1);
            break;
        case AST_STATEMENT_LIST:
            LOG_INFO("%.*sstatements {", depth, indent);
            if (ast->next) debug_print_ast(ast->next, depth+1);
            LOG_INFO("%.*s}", depth, indent);
            break;
        case AST_INVALID: break;
        }
    }
}

AST* parse_expression(Lexer *lexer, Allocator mem)
{
    for (; *lexer && peek_token(lexer) != ';'; next_token(lexer)) {
    }
    return nullptr;
}

AST* parse_statement(Lexer *lexer, Allocator mem)
{
    AST *ast = nullptr;
    if (next_token(lexer) == '{') {
        AST *ptr = ast = ALLOC_T(mem, AST) {
            .type = AST_STATEMENT_LIST,
        };

        while (*lexer && peek_token(lexer) != '}') {
            ptr = ptr->next = parse_statement(lexer, mem);
        }
    } else {
        if (lexer->t == TOKEN_IDENTIFIER) {
            Token identifier = lexer->t;
            i32 kw = keyword_from_string(identifier.str);
            if (kw != KW_INVALID) {
                switch (kw) {
                case KW_RETURN:
                    ast = ALLOC_T(mem, AST) {
                        .type = AST_RETURN,
                        .ret.expr = parse_expression(lexer, mem)
                    };
                }
            }
        }
    }

    return ast;
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

        ASTRoot root{ tl_linear_allocator(MAX_AST_MEM) };
        AST *ast = &root.ast;

        Lexer lexer{ f.data, f.size, file };

        for (next_token(&lexer); lexer; next_token(&lexer)) {
            LOG_INFO("token: [%s] '%.*s'", sz_from_enum(lexer.t.type), STRFMT(lexer.t.str));

            if (lexer.t.type == TOKEN_IDENTIFIER) {
                Token identifier = lexer.t;

                i32 kw = keyword_from_string(identifier.str);
                if (kw != KW_INVALID) {
                    switch (kw) {
                    case KW_RETURN:
                        ast = ast->next = ALLOC_T(root.mem, AST) {
                            .type = AST_RETURN,
                            .ret.expr = parse_expression(&lexer, root.mem),
                        };
                        break;
                    }
                } else if (peek_token(&lexer) == ':' && peek_nth_token(&lexer, 2) == ':') {
                    if (peek_nth_token(&lexer, 3) == '(') {
                        next_nth_token(&lexer, 3);
                        if (!require_next_token(&lexer, ')')) return -1;

                        ast = ast->next = ALLOC_T(root.mem, AST) {
                            .type = AST_PROCEDURE,
                            .proc.name = identifier.str,
                            .proc.body = parse_statement(&lexer, root.mem),
                        };
                    }
                }
            }
        }

        debug_print_ast(&root.ast);
    }
    return 0;
}
