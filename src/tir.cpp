#include "core.h"
#include "memory.h"
#include "file.h"
#include "lexer.h"
#include "process.h"

#include "string.h"

#include <cstdlib>
#include <stdio.h>
#include <string.h>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/NoFolder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/MC/TargetRegistry.h>

enum Keyword : i32 {
    KW_INVALID = 0,
    KW_RETURN,
};

enum ASTType : i32 {
    AST_INVALID = 0,

    AST_VAR,
    AST_VAR_DECL,

    AST_PROC_DECL,

    AST_RETURN,
    AST_LITERAL,
    AST_BINARY_OP,
};

const char* sz_from_enum(ASTType type)
{
    switch (type) {
    case AST_INVALID:   return "invalid";
    case AST_VAR:       return "var";
    case AST_VAR_DECL:  return "var_decl";
    case AST_PROC_DECL: return "proc_decl";
    case AST_RETURN:    return "return";
    case AST_LITERAL:   return "literal";
    case AST_BINARY_OP: return "binary_op";
    }

    return "invalid";
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

enum PrimitiveType {
    T_UNKNOWN = 0,
    T_VOID,
    T_INTEGER,
};

const char* sz_from_enum(PrimitiveType type)
{
    switch (type) {
    case T_UNKNOWN: return "unknown";
    case T_VOID:    return "void";
    case T_INTEGER: return "INT";
    }

    return "invalid";
}

struct TypeExpr {
    PrimitiveType type;
    i32           size;

    bool operator==(const TypeExpr &rhs) const = default;
    bool operator==(const PrimitiveType &rhs) const { return type == rhs; }
};

llvm::Type * llvm_type_from_type_expr(llvm::LLVMContext *context, TypeExpr type)
{
    switch (type.type) {
    case T_UNKNOWN: break;
    case T_VOID:
        return llvm::Type::getVoidTy(*context);
    case T_INTEGER:
        switch (type.size) {
        case 1: return llvm::Type::getInt8Ty(*context);
        case 2: return llvm::Type::getInt16Ty(*context);
        case 4: return llvm::Type::getInt32Ty(*context);
        case 8: return llvm::Type::getInt64Ty(*context);
        default: PANIC("Invalid integer size %d", type.size);
        }
        break;
    }

    return nullptr;
}

struct AST {
    AST *next;

    ASTType type;
    union {
        struct {
            Token identifier;
            TypeExpr ret_type;
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
            TypeExpr type;
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
            LOG_INFO("%.*svar %.*s",
                     depth, indent,
                     STRFMT(ast->var.identifier.str));
            break;
        case AST_VAR_DECL:
            LOG_INFO("%.*sdecl %.*s [%s:%d]",
                     depth, indent,
                     STRFMT(ast->var_decl.identifier.str),
                     sz_from_enum(ast->var_decl.type.type),
                    ast->var_decl.type.size);

            if (ast->var_decl.init) debug_print_ast(ast->var_decl.init, depth+1);
            break;
        case AST_LITERAL:
            LOG_INFO("%.*sliteral %.*s [%s:%d]",
                     depth, indent,
                     STRFMT(ast->literal.token.str),
                     sz_from_enum(ast->literal.type.type),
                     ast->literal.type.size);
            break;
        case AST_BINARY_OP:
            LOG_INFO("%.*sbinary op %.*s", depth, indent, STRFMT(ast->binary_op.op.str));
            debug_print_ast(ast->binary_op.lhs, depth+1);
            debug_print_ast(ast->binary_op.rhs, depth+1);
            break;
        case AST_PROC_DECL:
            LOG_INFO("%.*sproc %.*s [%s:%d]",
                     depth, indent,
                     STRFMT(ast->proc.identifier.str),
                     sz_from_enum(ast->proc.ret_type.type),
                     ast->proc.ret_type.size);

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
            .literal.type = { T_INTEGER, 0 },
        };
    } else if (optional_token(lexer, TOKEN_IDENTIFIER)) {
        expr = ALLOC_T(mem, AST) {
            .type = AST_VAR,
            .var.identifier = lexer->t,
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
        if (lexer->t == "i32") return { T_INTEGER, 4 };
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
                .type = AST_PROC_DECL,
                .proc.identifier = identifier,
                .proc.body = body,
            };
        }
    }

    return nullptr;
}

TypeExpr ast_typecheck(AST *ast, AST *proc)
{
    switch (ast->type) {
    case AST_VAR_DECL:
        if (ast->var_decl.type.type == T_UNKNOWN) {
            if (ast->var_decl.init) {
                ast->var_decl.type = ast_typecheck(ast->var_decl.init, proc);
            } else {
                LOG_ERROR("Cannot infer type of '%.*s'",
                          STRFMT(ast->var_decl.identifier.str));
            }
        } else {
            if (ast->var_decl.init) {
                TypeExpr init_type = ast_typecheck(ast->var_decl.init, proc);
                if (init_type.type != ast->var_decl.type.type) {
                    // TODO(jesper): check if the type is compatible or implicitly convertible
                    LOG_ERROR("Type mismatch in declaration of '%.*s'", STRFMT(ast->var_decl.identifier.str));
                }
            }
        }
        return ast->var_decl.type;
    case AST_PROC_DECL:
        for (AST *stmt = ast->proc.body; stmt; stmt = stmt->next)
            ast_typecheck(stmt, ast);

        // TODO(jesper): proc type expr
        return { T_UNKNOWN };
    case AST_RETURN: {
        PANIC_IF(!proc || proc->type != AST_PROC_DECL, "expected AST_PROC_DECL");
        TypeExpr ret_type = { T_VOID };
        if (ast->ret.expr) ret_type = ast_typecheck(ast->ret.expr, proc);

        if (proc->proc.ret_type == T_UNKNOWN)
            proc->proc.ret_type = ret_type;

        if (proc->proc.ret_type.type != ret_type.type)
            LOG_ERROR("Type mismatch in return statement");

        return ret_type;
        } break;
    case AST_LITERAL:
        return ast->literal.type;
    case AST_BINARY_OP: {
        TypeExpr lhs = ast_typecheck(ast->binary_op.lhs, proc);
        TypeExpr rhs = ast_typecheck(ast->binary_op.rhs, proc);

        if (lhs.type != rhs.type) {
            LOG_ERROR("Type mismatch in binary operation '%.*s'",
                      STRFMT(ast->binary_op.op.str));
        }

        return lhs;
    } break;
    case AST_INVALID:
        PANIC_UNREACHABLE();
        break;
    }

    return { T_UNKNOWN };
}

i32 ast_sizecheck(AST *ast, AST *proc, i32 topdown_size = 0)
{
    switch (ast->type) {
    case AST_INVALID:
        PANIC_UNREACHABLE();
        break;
    case AST_VAR_DECL:
        if (ast->var_decl.init) {
            i32 init_size = ast_sizecheck(ast->var_decl.init, proc, ast->var_decl.type.size);
            if (init_size != 0 && init_size != ast->var_decl.type.size) {
                LOG_ERROR("Size mismatch in declaration of '%.*s'", STRFMT(ast->var_decl.identifier.str));
            }
            ast->var_decl.type.size = init_size;
        } else if (ast->var_decl.type.size == 0) {
            LOG_ERROR("Cannot infer size of '%.*s'", STRFMT(ast->var_decl.identifier.str));
        }

        return ast->var_decl.type.size;
    case AST_LITERAL:
        if (ast->literal.type.size == 0)
            ast->literal.type.size = topdown_size;

        if (ast->literal.type.size == 0 &&
            ast->literal.type == T_INTEGER)
        {
            // TODO(jesper): I want to do more checking here to determine the smallest size required by the expression (which in the case of integer literals, depend on the size of the literal size itself), and propagate that information both upwards and sideways to other expressions, but I'm not sure how to do that yet.
            ast->literal.type.size = 4;
        }

        if (ast->literal.type.size == 0) {
            LOG_ERROR("Cannot infer size of '%.*s'", STRFMT(ast->literal.token.str));
        }
        return ast->literal.type.size;
    case AST_BINARY_OP: {
        i32 lhs = ast_sizecheck(ast->binary_op.lhs, proc, topdown_size);
        i32 rhs = ast_sizecheck(ast->binary_op.rhs, proc, lhs);

        if (lhs != rhs) {
            // TODO(jesper): check for implicit conversion
            LOG_ERROR("Size mismatch in binary operation '%.*s'", STRFMT(ast->binary_op.op.str));
        }

        return lhs;
        } break;
    case AST_PROC_DECL:
        for (AST *stmt = ast->proc.body; stmt; stmt = stmt->next)
            ast_sizecheck(stmt, ast);

        // TODO(jesper): proc type size?
        return 0;
    case AST_RETURN: {
        PANIC_IF(!proc || proc->type != AST_PROC_DECL, "expected AST_PROC_DECL");
        i32 ret_size = proc->proc.ret_type.size;
        if (ast->ret.expr)
            ret_size = ast_sizecheck(ast->ret.expr, proc, ret_size);

        if (proc->proc.ret_type.size == 0 &&
            proc->proc.ret_type != T_VOID)
        {
            proc->proc.ret_type.size = ret_size;
        }

        if (ret_size != proc->proc.ret_type.size) {
            LOG_ERROR("Size mismatch in return statement");
        }

        return ret_size;
        } break;
    }

    return 0;
}


struct LLVMIR {
    llvm::LLVMContext context;
    llvm::IRBuilder<llvm::NoFolder> *ir;
    llvm::Module *module;
};

llvm::Value* llvm_codegen_expr(LLVMIR *llvm, AST *ast)
{
    SArena scratch = tl_scratch_arena();

    switch (ast->type) {
    case AST_LITERAL: {
        i32 val = atoi(sz_string(ast->literal.token.str, scratch));
        return llvm->ir->getInt32(val);
        } break;
    case AST_BINARY_OP: {
        llvm::Value *lhs = llvm_codegen_expr(llvm, ast->binary_op.lhs);
        llvm::Value *rhs = llvm_codegen_expr(llvm, ast->binary_op.rhs);

        switch (ast->binary_op.op.type) {
        case '+': return llvm->ir->CreateAdd(lhs, rhs);
        case '-': return llvm->ir->CreateSub(lhs, rhs);
        case '*': return llvm->ir->CreateMul(lhs, rhs);
        case '/': return llvm->ir->CreateSDiv(lhs, rhs);
        default:
            LOG_ERROR("Invalid binary op '%.*s'",
                      STRFMT(ast->binary_op.op.str));
            break;
        }
        break;
    }
    default:
        LOG_ERROR("Invalid AST node type '%s'", sz_from_enum(ast->type));
        break;
    }
    return nullptr;
}


llvm::Function* llvm_codegen_proc(LLVMIR *llvm, AST *ast)
{
    PANIC_IF(ast->type != AST_PROC_DECL, "expected AST_PROC_DECL");

    SArena scratch = tl_scratch_arena();

    llvm::Type *ret_type = llvm_type_from_type_expr(
        &llvm->context,
        ast->proc.ret_type);

    if (!ret_type) ret_type = llvm::Type::getVoidTy(llvm->context);

    llvm::Function *func = llvm::Function::Create(
        llvm::FunctionType::get(ret_type, false),
        llvm::Function::ExternalLinkage,
        sz_string(ast->proc.identifier.str, scratch),
        llvm->module);

    llvm::BasicBlock *block = llvm::BasicBlock::Create(llvm->context, "entry", func);
    llvm->ir->SetInsertPoint(block);

    for (auto *stmt = ast->proc.body; stmt; stmt = stmt->next) {
        switch (stmt->type) {
        case AST_VAR_DECL:
            break;
        case AST_RETURN:
            if (stmt->ret.expr) {
                auto *val = llvm_codegen_expr(llvm, stmt->ret.expr);
                llvm->ir->CreateRet(val);
            } else {
                llvm->ir->CreateRetVoid();
            }
            break;
        default:
            LOG_ERROR("Invalid statement type '%s'", sz_from_enum(stmt->type));
            break;
        }
    }

    return func;
}

void emit_ast_x64(StringBuilder *sb, AST *ast)
{
    for (; ast; ast = ast->next) {
        switch (ast->type) {
        case AST_LITERAL:
            append_stringf(sb, "  mov eax, %.*s\n", STRFMT(ast->literal.token.str));
            break;
        case AST_PROC_DECL:
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
            default:
                LOG_ERROR("Invalid binary op '%.*s'",
                          STRFMT(ast->binary_op.op.str));
                break;
            }

            break;
        case AST_RETURN:
            if (ast->ret.expr) emit_ast_x64(sb, ast->ret.expr);
            append_stringf(sb, "  ret\n");
            break;
        default:
            LOG_ERROR("Invalid AST node type '%s'", sz_from_enum(ast->type));
            break;
        }
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

    Module module{};
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

        AST **ptr = &module.ast;
        while (next_token(&lexer)) {
            if (AST *proc = parse_proc_decl(&lexer, mem); proc) {
                array_add(&module.procedures, proc);
                (*ptr) = proc;
                ptr = &proc->next;

                if (proc->proc.identifier == "main") module.entry = proc;
            } else {
                PARSE_ERROR(&lexer, "unknown declaration in global scope");
                return -1;
            }
        }
    }

    for (AST *ast = module.ast; ast; ast = ast->next) {
        ast_typecheck(ast, nullptr);
    }

    for (AST *ast = module.ast; ast; ast = ast->next) {
        ast_sizecheck(ast, nullptr);
    }

    debug_print_ast(module.ast);

    LLVMIR llvm{};
    llvm.ir = new llvm::IRBuilder<llvm::NoFolder>(
        llvm::BasicBlock::Create(llvm.context, "entry"));
    llvm.module = new llvm::Module("tir", llvm.context);

    {
        SArena scratch = tl_scratch_arena();

        for (AST *it = module.ast; it; it = it->next) {
            if (it->type == AST_PROC_DECL) {
                llvm_codegen_proc(&llvm, it);
            }
        }

        llvm.module->print(llvm::outs(), nullptr);
    }


    {
        SArena scratch = tl_scratch_arena();

        llvm::InitializeAllTargetInfos();
        llvm::InitializeAllTargets();
        llvm::InitializeAllTargetMCs();
        llvm::InitializeAllAsmParsers();
        llvm::InitializeAllAsmPrinters();

        std::string target_triple = llvm::sys::getDefaultTargetTriple();

        std::string error;
        auto *target = llvm::TargetRegistry::lookupTarget(target_triple, error);
        if (!target) LOG_ERROR("Failed to lookup target: %s", error.c_str());

        llvm::TargetOptions opts{};
        auto *target_machine = target->createTargetMachine(
            target_triple, "generic", "",
            opts,
            llvm::Reloc::PIC_);

        llvm.module->setTargetTriple(target_triple);
        llvm.module->setDataLayout(target_machine->createDataLayout());

        std::error_code ec;
        llvm::raw_fd_ostream out{
            sztringf(scratch, "%s/%s.o", out_dir, out_name),
            ec,
            llvm::sys::fs::OF_None
        };

        llvm::legacy::PassManager pass_manager{};
        target_machine->addPassesToEmitFile(
            pass_manager,
            out, nullptr,
            llvm::CGFT_ObjectFile);

        pass_manager.run(*llvm.module);
        out.flush();
    }

    if (opts.out_type == OUTPUT_EXECUTABLE) {
        SArena scratch = tl_scratch_arena();
        run_process("clang", {
            stringf(scratch, "%s/%s.o", out_dir, out_name),
            "-o", stringf(scratch, "%s/%s", out_dir, out_name),
        });
    }

    return 0;
}
