#include "core.h"
#include "memory.h"
#include "file.h"
#include "lexer.h"
#include "process.h"

#include "string.h"

#include <cstdlib>
#include <stdio.h>
#include <string.h>

enum Keyword : i32 {
    KW_INVALID = 0,
    KW_RETURN,
};

enum ASTType : i32 {
    AST_INVALID = 0,

    AST_VAR_DECL,
    AST_VAR,

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

enum PrimitiveType {
    T_UNKNOWN = 0,
    T_i32,
};

const char* sz_from_enum(PrimitiveType type)
{
    switch (type) {
    case T_UNKNOWN: return "unknown";
    case T_i32:     return "i32";
    }

    return "invalid";
}

struct TypeExpr {
    PrimitiveType type;
};

struct AST {
    AST *next;

    ASTType type;
    union {
        struct {
            Token identifier;
            AST *body;
        } proc;
        struct {
            Token identifier;
            TypeExpr type;
            AST *init;
        } var_decl;
        struct {
            Token identifier;
        } var;
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

struct Module {
    AST *ast;
    AST *entry;

    DynamicArray<AST*> procedures;
};

#include "gen/internal/tir.h"

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
        case AST_VAR:
            LOG_INFO("%.*svar %.*s", depth, indent, STRFMT(ast->var.identifier.str));
            break;
        case AST_VAR_DECL:
            LOG_INFO("%.*sdecl %.*s [%s]", depth, indent, STRFMT(ast->var_decl.identifier.str), sz_from_enum(ast->var_decl.type.type));

            if (ast->var_decl.init) debug_print_ast(ast->var_decl.init, depth+1);
            break;
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
        if (prec <= min_prec) break;
        next_token(lexer);

        AST *lhs = expr;
        AST *rhs = parse_expression(lexer, mem, prec);

        expr = ALLOC_T(mem, AST) {
            .type = AST_BINARY_OP,
            .binary_op.op = op,
            .binary_op.lhs = lhs,
            .binary_op.rhs = rhs,
        };
    }

    return expr;
}

TypeExpr parse_type_expression(Lexer *lexer)
{
    if (optional_token(lexer, TOKEN_IDENTIFIER)) {
        if (lexer->t == "i32") return { T_i32 };
    }
    return { T_UNKNOWN };
}

AST* parse_statement(Lexer *lexer, Allocator mem) INTERNAL
{
    if (optional_token(lexer, '{')) {
        AST *stmt = parse_statement(lexer, mem);
        if (!stmt) return nullptr;

        AST *ptr = stmt;
        while (*lexer && peek_token(lexer) != '}') {
            ptr->next = parse_statement(lexer, mem);
            ptr = ptr->next;
        }

        if (!require_next_token(lexer, '}')) {
            PARSE_ERROR(lexer, "unclosed statement list");
            return nullptr;
        }

        return stmt;
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
        } else if (optional_token(lexer, ':')) {
            AST *decl = ALLOC_T(mem, AST) {
                .type = AST_VAR_DECL,
                .var_decl.identifier = identifier,
                .var_decl.type = parse_type_expression(lexer),
            };

            if (optional_token(lexer, '=')) {
                Token op = lexer->t;

                AST *lhs = ALLOC_T(mem, AST) {
                    .type = AST_VAR,
                    .var.identifier = identifier,
                };

                AST *rhs = parse_expression(lexer, mem);

                decl->var_decl.init = ALLOC_T(mem, AST) {
                    .type = AST_BINARY_OP,
                    .binary_op.op = op,
                    .binary_op.lhs = lhs,
                    .binary_op.rhs = rhs,
                };
            }

            if (!require_next_token(lexer, ';')) {
                PARSE_ERROR(lexer, "expected ';' after declaration");
                return nullptr;
            }

            return decl;
        } else {
            PARSE_ERROR(lexer, "unexpected identifier '%.*s'", STRFMT(lexer->t.str));
            return nullptr;
        }
    }

    return nullptr;
}

AST* parse_proc_decl(Lexer *lexer, Allocator mem) INTERNAL
{
    if (lexer->t.type != TOKEN_IDENTIFIER) return nullptr;
    Token identifier = lexer->t;

    // TODO(jesper): this meains `main :\s*: ()` is valid syntax, should it be?
    if (peek_token(lexer) == ':' &&
        peek_nth_token(lexer, 2) == ':')
    {
        if (peek_nth_token(lexer, 3) == '(') {
            next_nth_token(lexer, 3);

            if (!require_next_token(lexer, ')')) {
                PARSE_ERROR(lexer, "expected ')'");
                return nullptr;
            }

            AST *body = parse_statement(lexer, mem);
            if (!body) {
                PARSE_ERROR(lexer, "expected statement after procedure declaration");
                return nullptr;
            }

            return ALLOC_T(mem, AST) {
                .type = AST_PROCEDURE,
                .proc.identifier = identifier,
                .proc.body = body,
            };
        }
    }

    return nullptr;
}

void emit_ast_x64(StringBuilder *sb, AST *ast)
{
    while (ast) {
        switch (ast->type) {
        case AST_LITERAL:
            append_stringf(sb, "  mov eax, %.*s\n", STRFMT(ast->literal.token.str));
            break;
        case AST_PROCEDURE:
            append_stringf(sb, "%.*s:\n", STRFMT(ast->proc.identifier.str));
            emit_ast_x64(sb, ast->proc.body);
            break;
        case AST_BINARY_OP:
            emit_ast_x64(sb, ast->binary_op.lhs);
            append_stringf(sb, "  push eax\n");
            emit_ast_x64(sb, ast->binary_op.rhs);
            append_stringf(sb, "  pop ebx\n");

            switch (ast->binary_op.op.type) {
            case '+': append_stringf(sb, "  add eax, ebx\n"); break;
            case '-': append_stringf(sb, "  sub eax, ebx\n"); break;
            case '*': append_stringf(sb, "  imul eax, ebx\n"); break;
            case '/': append_stringf(sb, "  idiv ebx\n"); break;
            default: LOG_ERROR("Invalid binary op '%.*s'", STRFMT(ast->binary_op.op.str)); break;
            }

            break;
        case AST_RETURN:
            if (ast->ret.expr) emit_ast_x64(sb, ast->ret.expr);
            append_stringf(sb, "  ret\n");
            break;
        default: LOG_ERROR("Invalid AST node type '%d'", ast->type); break;
        }
        ast = ast->next;
    }
}

enum OutputType : i32 {
    OUTPUT_EXECUTABLE,
    OUTPUT_OBJECT,
};

struct {
    OutputType out_type;
} opts;

void print_usage()
{
    printf("Usage: tir <file> [options]\n");
    printf("Options:\n");
    printf("  -h, --help  Print this message\n");
    printf("  -o <file>   Output file\n");
    printf("  -c          Output object file\n");
    printf("\n");
}

int main(int argc, char *argv[])
{
    for (auto &c : indent) c = ' ';
    constexpr i32 MAX_AST_MEM = 10*MiB;

    init_default_allocators();

    char *src = nullptr;
    char *out = nullptr;

    char *src_name    = nullptr;
    char *out_name    = nullptr;
    char *out_dir     = nullptr;

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

                out = argv[++i];
            } else if (argv[i][1] == 'c') {
                opts.out_type = OUTPUT_OBJECT;
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

    src_name = strdup(src);
    if (char *p = strrchr(src_name, '/'); p) src_name = p+1;
    if (char *p = strrchr(src_name, '.'); p) *p = '\0';

    if (!out) {
        if (opts.out_type == OUTPUT_EXECUTABLE)
            out = sztringf(mem_dynamic, "./%s", src_name);
        else
            out = sztringf(mem_dynamic, "./%s.o", src_name);
    }

    out_name = strdup(out);
    if (char *p = strrchr(out_name, '/'); p) out_name = p+1;
    if (char *p = strrchr(out_name, '.'); p) *p = '\0';

    out_dir = strdup(out);
    if (char *p = strrchr(out_dir, '/'); p) *p = '\0';

    Module global{};
    {
        SArena scratch = tl_scratch_arena();

        String file = string(src);
        FileInfo f = read_file(file, scratch);
        if (!f.data) {
            LOG_ERROR("Failed to read file '%.*s'", STRFMT(file));
            return -1;
        }

        Allocator mem = tl_linear_allocator(MAX_AST_MEM);
        Lexer lexer{ f.data, f.size, file };

        AST **ptr = &global.ast;
        while (next_token(&lexer)) {
            if (AST *proc = parse_proc_decl(&lexer, mem); proc) {
                array_add(&global.procedures, proc);
                (*ptr) = proc;
                ptr = &proc->next;

                if (proc->proc.identifier == "main") global.entry = proc;
            } else {
                PARSE_ERROR(&lexer, "unknown declaration in global scope");
                return -1;
            }
        }

        debug_print_ast(global.ast);
    }

    {
        SArena scratch = tl_scratch_arena();
        StringBuilder sb = { .alloc = scratch };

        append_stringf(&sb, ".intel_syntax noprefix\n");
        append_stringf(&sb, ".globl _start\n");

        for (auto *decl : global.procedures) {
            append_stringf(&sb, ".globl %.*s\n", STRFMT(decl->proc.identifier.str));
        }

        if (global.entry) {
            append_string(&sb, "_start:\n");
            append_stringf(&sb, "  call %.*s\n", STRFMT(global.entry->proc.identifier.str));
            append_string(&sb, "  mov ebx, eax\n");
            append_string(&sb, "  mov eax, 1\n");
            append_string(&sb, "  int 0x80\n");
        }


        emit_ast_x64(&sb, global.ast);
        write_file(stringf(scratch, "%s/%s.s", out_dir, out_name), &sb);
    }

    {
        SArena scratch = tl_scratch_arena();

        run_process("clang", {
            "-masm=intel",
            "-c",
            stringf(scratch, "%s/%s.s", out_dir, out_name),
            "-o", stringf(scratch, "%s/%s.o", out_dir, out_name),
        });

        if (opts.out_type == OUTPUT_EXECUTABLE) {
            run_process("ld.lld", {
                stringf(scratch, "%s/%s.o", out_dir, out_name),
                "-o", stringf(scratch, "%s/%s", out_dir, out_name),
            });
        }
    }

    return 0;
}
