#include "core.h"
#include "memory.h"
#include "file.h"
#include "lexer.h"
#include "process.h"
#include "hash_table.h"

#include "string.h"

#include <cstdlib>
#include <stdio.h>
#include <string.h>

#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>

#ifdef _WIN32
#define strdup _strdup
#endif

enum Keyword : i32 {
    KW_INVALID = 0,
    KW_RETURN,
};

enum ASTType : i32 {
    AST_INVALID = 0,

    AST_VAR_DECL,
    AST_VAR_LOAD,
    AST_VAR_STORE,

    AST_PROC_DECL,
    AST_PROC_CALL,

    AST_RETURN,
    AST_LITERAL,
    AST_BINARY_OP,
};

const char* sz_from_enum(ASTType type)
{
    switch (type) {
    case AST_INVALID:   return "invalid";
    case AST_VAR_LOAD:  return "var_load";
    case AST_VAR_DECL:  return "var_decl";
    case AST_VAR_STORE: return "var_store";
    case AST_PROC_DECL: return "proc_decl";
    case AST_PROC_CALL: return "proc_call";
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

enum PrimitiveType : i32 {
    T_INVALID = -1,
    T_UNKNOWN = 0,
    T_VOID,
    T_INTEGER,
    T_UNSIGNED,
    T_SIGNED,
    T_FLOAT,
    T_BOOL,
};

const char* sz_from_enum(PrimitiveType type)
{
    switch (type) {
    case T_UNKNOWN:  return "unknown";
    case T_VOID:     return "void";
    case T_INTEGER:  return "INT";
    case T_SIGNED:   return "SINT";
    case T_UNSIGNED: return "UINT";
    case T_FLOAT:    return "FLOAT";
    case T_BOOL:     return "BOOL";
    case T_INVALID:  break;
    }

    return "invalid";
}

struct TypeExpr {
    PrimitiveType prim;
    i32           size;

    explicit operator bool() { return prim != T_UNKNOWN && size != 0; }
    bool operator==(const TypeExpr &rhs) const = default;
    bool operator==(const PrimitiveType &rhs) const { return prim == rhs; }
};

LLVMTypeRef llvm_type_from_type_expr(LLVMContextRef context, TypeExpr type)
{
    switch (type.prim) {
    case T_INVALID: break;
    case T_UNKNOWN: break;
    case T_VOID:
        return LLVMVoidType();
    case T_INTEGER:
        PANIC("undetermined signage of integer");
        return nullptr;
    case T_SIGNED:
        switch (type.size) {
        case 1: return LLVMInt8Type();
        case 2: return LLVMInt16Type();
        case 4: return LLVMInt32Type();
        case 8: return LLVMInt64Type();
        default: PANIC("Invalid integer size %d", type.size);
        }
        break;
    case T_UNSIGNED:
        // TODO(jesper): how does LLVM distinguish between signed and unsigned types?
        switch (type.size) {
        case 1: return LLVMInt8Type();
        case 2: return LLVMInt16Type();
        case 4: return LLVMInt32Type();
        case 8: return LLVMInt64Type();
        default: PANIC("Invalid unsignd integer size %d", type.size);
        }
        break;
    case T_FLOAT:
        switch (type.size) {
        case 4: return LLVMFloatType();
        case 8: return LLVMDoubleType();
        default: PANIC("invalid float size: %d", type.size);
        }
        break;
    case T_BOOL:
        switch (type.size) {
        case 1: return LLVMInt1Type();
        default: PANIC("invalid bool size: %d", type.size);
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
        } proc_decl;
        struct {
            Token identifier;
        } proc_call;
        struct {
            Token identifier;
            TypeExpr type;
            AST *init;
        } var_decl;
        struct {
            Token identifier;
            AST *rhs;
        } var_store;
        struct {
            Token identifier;
        } var_load;
        struct {
            Token op;
            AST *lhs;
            AST *rhs;
        } binary_op;
        struct {
            Token token;
            TypeExpr type;
            union {
                i64 ival;
                f32 fval;
                bool bval;
            };
        } literal;
        struct {
            Token token;
            AST *expr;
        } ret;
    };
};

enum SymbolType {
    SYM_VARIABLE,
    SYM_PROC,
};

struct Symbol {
    SymbolType type;
    union {
        struct {
            TypeExpr type;
        } variable;
        struct {
            AST *ast;
        } proc;
    };
};

struct Module {
    AST *ast;
    AST *entry;

    DynamicArray<AST*> procedures;
    HashTable<String, Symbol> symbols;
};


struct Scope {
    LLVMBasicBlockRef entry;
    HashTable<String, LLVMValueRef> variables;
};

struct LLVMProc {
    LLVMValueRef func;
    LLVMTypeRef  func_t;
};

struct LLVMIR {
    LLVMContextRef context;
    LLVMBuilderRef ir;
    LLVMModuleRef module;

    HashTable<String, LLVMProc> procedures;

    Scope scope;
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
        case AST_VAR_LOAD:
            LOG_INFO("%.*svar %.*s",
                     depth, indent,
                     STRFMT(ast->var_load.identifier.str));
            break;
        case AST_VAR_DECL:
            LOG_INFO("%.*sdecl %.*s [%s:%d]",
                     depth, indent,
                     STRFMT(ast->var_decl.identifier.str),
                     sz_from_enum(ast->var_decl.type.prim),
                    ast->var_decl.type.size);

            if (ast->var_decl.init) {
                LOG_INFO("%.*sinit", depth, indent);
                debug_print_ast(ast->var_decl.init, depth+1);
            }
            break;
        case AST_VAR_STORE:
            LOG_INFO("%.*sstore [%.*s]", depth, indent, STRFMT(ast->var_store.identifier.str));
            debug_print_ast(ast->var_store.rhs, depth+1);
            break;
        case AST_LITERAL:
            LOG_INFO("%.*sliteral %.*s [%s:%d]",
                     depth, indent,
                     STRFMT(ast->literal.token.str),
                     sz_from_enum(ast->literal.type.prim),
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
                     STRFMT(ast->proc_decl.identifier.str),
                     sz_from_enum(ast->proc_decl.ret_type.prim),
                     ast->proc_decl.ret_type.size);

            if (ast->proc_decl.body) debug_print_ast(ast->proc_decl.body, depth+1);
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

        if (!i64_from_string(lexer->t.str, &expr->literal.ival)) {
            TERROR(lexer->t, "invalid integer literal");
            return nullptr;
        }

        if (expr->literal.ival < 0) expr->literal.type.prim = T_SIGNED;
    } else if (optional_token(lexer, TOKEN_NUMBER)) {
        // TODO(jesper): how do I distinguish between f32 and f64 in literals?
        expr = ALLOC_T(mem, AST) {
            .type = AST_LITERAL,
            .literal.token = lexer->t,
            .literal.type = { T_FLOAT, 4 },
        };

        if (!f32_from_string(lexer->t.str, &expr->literal.fval)) {
            TERROR(lexer->t, "invalid float literal");
            return nullptr;
        }
    } else if (optional_identifier(lexer, "false") ||
               optional_identifier(lexer, "true"))
    {
        expr = ALLOC_T(mem, AST) {
            .type = AST_LITERAL,
            .literal.token = lexer->t,
            .literal.type = { T_BOOL, 1 },
        };

        if (!bool_from_string(lexer->t.str, &expr->literal.bval)) {
            TERROR(lexer->t, "invalid boolean literal");
            return nullptr;
        }
    } else if (optional_token(lexer, TOKEN_IDENTIFIER)) {
        Token identifier = lexer->t;
        if (optional_token(lexer, '(')) {
            if (!require_next_token(lexer, ')')) {
                PARSE_ERROR(lexer, "expected ')'");
                return nullptr;
            }

            expr = ALLOC_T(mem, AST) {
                .type = AST_PROC_CALL,
                .proc_call.identifier = identifier,
            };

        } else {
            expr = ALLOC_T(mem, AST) {
                .type = AST_VAR_LOAD,
                .var_load.identifier = lexer->t,
            };
        }
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
        if (lexer->t == "i8")  return { T_SIGNED,  1 };
        if (lexer->t == "i16") return { T_SIGNED,  2 };
        if (lexer->t == "i32") return { T_SIGNED,  4 };
        if (lexer->t == "i64") return { T_SIGNED,  8 };

        if (lexer->t == "u8")  return { T_UNSIGNED, 1 };
        if (lexer->t == "u16") return { T_UNSIGNED, 2 };
        if (lexer->t == "u32") return { T_UNSIGNED, 4 };
        if (lexer->t == "u64") return { T_UNSIGNED, 8 };

        if (lexer->t == "f32") return { T_FLOAT, 4 };
        if (lexer->t == "f64") return { T_FLOAT, 8 };

        if (lexer->t == "bool") return { T_BOOL, 1 };
        return { T_INVALID };
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
                decl->var_decl.init = parse_expression(lexer, mem);
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

AST* parse_proc_decl(Lexer *lexer, Module *module, Allocator mem) INTERNAL
{
    Lexer stored = *lexer;

    if (lexer->t.type != TOKEN_IDENTIFIER) return nullptr;
    Token identifier = lexer->t;


    // TODO(jesper): this meains `main :\s*: ()` is valid syntax, should it be?
    if (optional_token(lexer, ':') && optional_token(lexer, ':')) {
        if (optional_token(lexer, '(')) {
            if (!require_next_token(lexer, ')')) {
                PARSE_ERROR(lexer, "expected ')'");
                return nullptr;
            }

            TypeExpr ret_type { T_UNKNOWN };
            if (optional_token(lexer, '-')) {
                if (!require_next_token(lexer, '>')) {
                    PARSE_ERROR(lexer, "expected '->' after parameter list");
                    return nullptr;
                }

                ret_type = parse_type_expression(lexer);
                if (ret_type == T_UNKNOWN) {
                    PARSE_ERROR(lexer, "missing explicit type expression for return type; add appropriate return type or remove the '->' for implicit retun type deduction");
                    return nullptr;
                }

                if (ret_type == T_INVALID) {
                    PARSE_ERROR(
                        lexer,
                        "invalid type expression for return type: '%.*s'",
                        STRFMT(lexer->t.str));
                    return nullptr;
                }
            }


            AST *body = parse_statement(lexer, mem);
            if (!body) {
                PARSE_ERROR(lexer, "expected statement after procedure declaration");
                return nullptr;
            }

            AST *proc = ALLOC_T(mem, AST) {
                .type = AST_PROC_DECL,
                .proc_decl.identifier = identifier,
                .proc_decl.body = body,
                .proc_decl.ret_type = ret_type,
            };

            map_find_emplace(&module->symbols, identifier.str, {
                .type = SYM_PROC,
                .proc = { proc },
            });

            return proc;
        }
    }

    *lexer = stored;
    return nullptr;
}

TypeExpr ast_typecheck(AST *ast, TypeExpr parent, Module *module, AST *proc)
{
    switch (ast->type) {
    case AST_VAR_LOAD: {
        Symbol *sym = map_find(&module->symbols, ast->var_load.identifier.str);
        if (!sym) {
            TERROR(ast->var_load.identifier,
                   "unknown variable '%.*s'",
                   STRFMT(ast->var_load.identifier.str));
            return { T_INVALID };
        }

        if (sym->type != SYM_VARIABLE) {
            TERROR(ast->var_load.identifier,
                   "'%.*s' is not a variable",
                   STRFMT(ast->var_load.identifier.str));
            return { T_INVALID };
        }

        return sym->variable.type;
        } break;
    case AST_VAR_STORE: {
        Symbol *sym = map_find(&module->symbols, ast->var_store.identifier.str);
        if (!sym) {
            TERROR(ast->var_load.identifier,
                   "Unknown variable '%.*s'",
                   STRFMT(ast->var_load.identifier.str));
            return { T_INVALID };
        }

        if (sym->type != SYM_VARIABLE) {
            TERROR(ast->var_load.identifier,
                   "symbol '%.*s' is not a variable",
                   STRFMT(ast->var_load.identifier.str));
            return { T_INVALID };
        }

        TypeExpr lhs = sym->variable.type;
        TypeExpr rhs = ast_typecheck(ast->var_store.rhs, lhs, module, proc);
        if (lhs != rhs) {

            // TODO(jesper): implicit/explicit conversion rules
            TERROR(ast->var_store.identifier,
                   "type mismatch, variable declared as [%s:%d], assignment deduced as [%s:%d]",
                   sz_from_enum(lhs.prim), lhs.size,
                   sz_from_enum(rhs.prim), rhs.size);
            return { T_INVALID };
        }

        return lhs;
        } break;
    case AST_VAR_DECL:
        if (ast->var_decl.init) {
            TypeExpr init_type = ast_typecheck(ast->var_decl.init, ast->var_decl.type, module, proc);
            if (ast->var_decl.type == T_UNKNOWN) {
                ast->var_decl.type = init_type;
            } else if (init_type.prim != ast->var_decl.type.prim) {
                // TODO(jesper): check if the type is compatible or implicitly convertible
                TERROR(ast->var_decl.identifier,
                       "type mismatch in declaration and assignment of variable, declared as [%s:%d], assignment deduced as [%s:%d]",
                       sz_from_enum(ast->var_decl.type.prim), ast->var_decl.type.size,
                       sz_from_enum(init_type.prim), init_type.size);
                return { T_INVALID };
            }
        }

        if (ast->var_decl.type == T_INTEGER) {
            TERROR(ast->var_decl.identifier,
                   "cannot infer signage of integer variable, '%.*s'",
                   STRFMT(ast->var_decl.identifier.str));
            return { T_INVALID };
        }

        if (ast->var_decl.type == T_UNKNOWN) {
            TERROR(ast->var_decl.identifier,
                   "cannot infer type of '%.*s'",
                   STRFMT(ast->var_decl.identifier.str));
            return { T_INVALID };
        }

        map_set(&module->symbols, ast->var_decl.identifier.str, {
            SYM_VARIABLE,
            .variable = { ast->var_decl.type }
        });

        return ast->var_decl.type;
    case AST_PROC_DECL:
        for (AST *stmt = ast->proc_decl.body; stmt; stmt = stmt->next) {
            if (ast_typecheck(stmt, ast->proc_decl.ret_type, module, ast) == T_INVALID)
                return { T_INVALID };
        }

        // TODO(jesper): proc type expr?
        return ast->proc_decl.ret_type;

    case AST_PROC_CALL: {
        Symbol *sym = map_find(&module->symbols, ast->proc_call.identifier.str);
        if (!sym || sym->type != SYM_PROC) {
            TERROR(ast->proc_call.identifier, "procedure not found: '%.*s'", STRFMT(ast->proc_call.identifier.str));
            return { T_INVALID };
        }

        return sym->proc.ast->proc_decl.ret_type;
        } break;

    case AST_RETURN: {
        PANIC_IF(!proc || proc->type != AST_PROC_DECL, "expected AST_PROC_DECL");

        TypeExpr ret_type = { T_VOID };
        // TODO(jesper): I don't think this is correct, but I need to add more conversion rules and explicit return type syntax before really testing this further
        if (ast->ret.expr) ret_type = ast_typecheck(ast->ret.expr, { T_SIGNED }, module, proc);

        if (proc->proc_decl.ret_type == T_UNKNOWN)
            proc->proc_decl.ret_type = ret_type;

        if (proc->proc_decl.ret_type.prim != ret_type.prim) {
            TERROR(ast->ret.token,
                   "type mismatch in return statement; proc ret type dedeuced to [%s:%d], return expression deduced as [%s:%d]",
                   sz_from_enum(proc->proc_decl.ret_type.prim), proc->proc_decl.ret_type.size,
                   sz_from_enum(ret_type.prim), ret_type.size);

            return { T_INVALID };
        }

        return ret_type;
        } break;
    case AST_LITERAL:
        if (ast->literal.type == T_INTEGER) {
            if (parent == T_SIGNED || parent == T_UNSIGNED)
                ast->literal.type.prim = parent.prim;
            else if (parent == T_UNKNOWN)
                ast->literal.type.prim = T_SIGNED;
        }

        return ast->literal.type;
    case AST_BINARY_OP: {
        TypeExpr lhs = ast_typecheck(ast->binary_op.lhs, parent, module, proc);
        TypeExpr rhs = ast_typecheck(ast->binary_op.rhs, parent, module, proc);

        if (lhs.prim != rhs.prim) {
            TERROR(ast->binary_op.op,
                   "type mismatch in binary operation [%s:%d] %.*s [%s:%d]",
                   sz_from_enum(lhs.prim), lhs.size,
                   STRFMT(ast->binary_op.op.str),
                   sz_from_enum(rhs.prim), rhs.size);
        }

        return lhs;
    } break;
    case AST_INVALID:
        PANIC_UNREACHABLE();
        break;
    }

    return { T_UNKNOWN };
}

TypeExpr ast_sizecheck(AST *ast, Module *module, AST *proc, i32 topdown_size = 0)
{
    switch (ast->type) {
    case AST_VAR_LOAD: {
        Symbol *sym = map_find(&module->symbols, ast->var_load.identifier.str);
        if (!sym) {
            TERROR(ast->var_load.identifier, "Unknown variable '%.*s'", STRFMT(ast->var_load.identifier.str));
            return { T_INVALID };
        }

        if (sym->type != SYM_VARIABLE) {
            TERROR(ast->var_load.identifier,
                   "symbol '%.*s' is not a variable",
                   STRFMT(ast->var_load.identifier.str));
            return { T_INVALID };
        }

        if (sym->variable.type.size == 0) sym->variable.type.size = topdown_size;
        return sym->variable.type;
        } break;
    case AST_VAR_STORE: {
        Symbol *sym = map_find(&module->symbols, ast->var_store.identifier.str);
        if (!sym) {
            TERROR(ast->var_load.identifier,
                   "unknown variable '%.*s'",
                   STRFMT(ast->var_load.identifier.str));
            return { T_INVALID };
        }

        if (sym->type != SYM_VARIABLE) {
            TERROR(ast->var_load.identifier,
                   "symbol '%.*s' is not a variable",
                   STRFMT(ast->var_load.identifier.str));
            return { T_INVALID };
        }

        TypeExpr rhs = ast_sizecheck(ast->var_store.rhs, module, proc, sym->variable.type.size);

        if (sym->variable.type.size == 0) sym->variable.type.size = rhs.size;
        if (rhs.size != 0 && rhs.size != sym->variable.type.size) {
            TERROR(ast->var_store.identifier,
                   "sizing mismtach in variable store, declared as [%s:%d], rhs requires deduced as size %d",
                   sz_from_enum(sym->variable.type.prim), sym->variable.type.size,
                   rhs);
            return { T_INVALID };
        }

        return sym->variable.type;
        } break;
    case AST_INVALID:
        PANIC_UNREACHABLE();
        break;
    case AST_VAR_DECL:
        if (ast->var_decl.init) {
            TypeExpr init_type = ast_sizecheck(ast->var_decl.init, module, proc, ast->var_decl.type.size);

            if (ast->var_decl.type.size == 0) {
                ast->var_decl.type.size = init_type.size;
            }

            if (init_type.size != 0 && init_type.size > ast->var_decl.type.size) {
                TERROR(ast->var_decl.identifier,
                       "size mismatch in variable declaration, declared as [%s:%d], initialization expression deduced as [%s:%d]",
                       sz_from_enum(ast->var_decl.type.prim), ast->var_decl.type.size,
                       sz_from_enum(init_type.prim), init_type.size);
                return { T_INVALID };
            }
        } else if (ast->var_decl.type.size == 0) {
            TERROR(ast->var_decl.identifier, "unable to infer size, variable declaration require either an explicit type, or an assignment expression to automatically deduce type from");
            return { T_INVALID };
        }

        return ast->var_decl.type;
    case AST_LITERAL:
        if (ast->literal.type.size == 0)
            ast->literal.type.size = topdown_size;

        if (ast->literal.type.size == 0 && ast->literal.type == T_SIGNED) {
            i64 ival = ast->literal.ival;
            if (ival <= i8_MAX && ival >= i8_MIN) ast->literal.type.size = 1;
            else if (ival <= i16_MAX && ival >= i16_MIN) ast->literal.type.size = 2;
            else if (ival <= i32_MAX && ival >= i32_MIN) ast->literal.type.size = 4;
            else ast->literal.type.size = 8;
        }

        if (ast->literal.type.size == 0 && ast->literal.type == T_UNSIGNED) {
            u64 uval = ast->literal.ival;
            if (uval <= u8_MAX) ast->literal.type.size = 1;
            else if (uval <= u16_MAX) ast->literal.type.size = 2;
            else if (uval <= u32_MAX) ast->literal.type.size = 4;
            else ast->literal.type.size = 8;
        }

        if (ast->literal.type.size == 0) {
            TERROR(ast->literal.token, "cannot infer size of '%.*s'", STRFMT(ast->literal.token.str));
        }
        return ast->literal.type;
    case AST_BINARY_OP: {
        TypeExpr lhs = ast_sizecheck(ast->binary_op.lhs, module, proc, topdown_size);
        TypeExpr rhs = ast_sizecheck(ast->binary_op.rhs, module, proc, lhs.size);

        if (lhs.size == 0 && rhs.size != 0) {
            lhs = ast_sizecheck(ast->binary_op.lhs, module, proc, rhs.size);
        }

        if (lhs.size != rhs.size) {
            // TODO(jesper): check for implicit conversion
            TERROR(ast->binary_op.op,
                   "size mismatch in binary operation, lhs %d, rhs %d",
                   lhs, rhs);

        }

        return lhs;
        } break;

    case AST_PROC_DECL:
        for (AST *stmt = ast->proc_decl.body; stmt; stmt = stmt->next) {
            if (ast_sizecheck(stmt, module, ast) == T_INVALID)
                return { T_INVALID };
        }

        return ast->proc_decl.ret_type;

    case AST_PROC_CALL: {
        Symbol *sym = map_find(&module->symbols, ast->proc_call.identifier.str);
        if (!sym || sym->type != SYM_PROC) {
            TERROR(ast->proc_call.identifier, "procedure not found: '%.*s'", STRFMT(ast->proc_call.identifier.str));
            return { T_INVALID };
        }

        return sym->proc.ast->proc_decl.ret_type;
        } break;

    case AST_RETURN: {
        PANIC_IF(!proc || proc->type != AST_PROC_DECL, "expected AST_PROC_DECL");

        TypeExpr ret_type = proc->proc_decl.ret_type;
        if (ast->ret.expr)
            ret_type = ast_sizecheck(ast->ret.expr, module, proc, ret_type.size);

        if (proc->proc_decl.ret_type.size == 0 &&
            proc->proc_decl.ret_type != T_VOID)
        {
            proc->proc_decl.ret_type.size = ret_type.size;
        }

        if (ret_type.size != proc->proc_decl.ret_type.size) {
            TERROR(ast->ret.token, "size mismatch in return statement");
        }

        return ret_type;
        } break;
    }

    return { T_UNKNOWN };
}

LLVMValueRef llvm_create_scoped_var(
    LLVMIR *llvm,
    Scope *scope,
    LLVMTypeRef type,
    String name)
{
    SArena scratch = tl_scratch_arena();

    LLVMValueRef var = LLVMBuildAlloca(llvm->ir, type, sz_string(name, scratch));
    map_set(&scope->variables, name, var);

    return var;
}

LLVMValueRef llvm_codegen_expr(LLVMIR *llvm, AST *ast)
{
    SArena scratch = tl_scratch_arena();

    switch (ast->type) {
    case AST_VAR_DECL: {
        LLVMValueRef var = llvm_create_scoped_var(
            llvm, &llvm->scope,
            llvm_type_from_type_expr(llvm->context, ast->var_decl.type),
            ast->var_decl.identifier.str);

        if (ast->var_decl.init) {
            LLVMValueRef init = llvm_codegen_expr(llvm, ast->var_decl.init);
            return LLVMBuildStore(llvm->ir, init, var);
        } else {
            // TODO(jesper): default init value
        }
        } break;

    case AST_VAR_STORE: {
        LLVMValueRef *it = map_find(
            &llvm->scope.variables,
            ast->var_store.identifier.str);

        if (!it) {
            TERROR(ast->var_load.identifier, "unknown variable");
            return nullptr;
        }

        LLVMValueRef rhs = llvm_codegen_expr(llvm, ast->var_store.rhs);
        return LLVMBuildStore(llvm->ir, rhs, *it);
        } break;

    case AST_VAR_LOAD: {
        LLVMValueRef *it = map_find(
            &llvm->scope.variables,
            ast->var_load.identifier.str);

        if (!it) {
            TERROR(ast->var_load.identifier, "unknown variable");
            return nullptr;
        }

        LLVMTypeRef type = LLVMGetAllocatedType(*it);
        size_t length; const char *name = LLVMGetValueName2(*it, &length);

        return LLVMBuildLoad2(llvm->ir, type, *it, name);
        } break;

    case AST_PROC_CALL: {
        LLVMProc *proc = map_find(&llvm->procedures, ast->proc_call.identifier.str);
        if (!proc) {
            TERROR(ast->proc_call.identifier, "unknown procedure '%.*s'", ast->proc_call.identifier.str);
            return nullptr;
        }

        return LLVMBuildCall2(llvm->ir, proc->func_t, proc->func, nullptr, 0, "");
        } break;

    case AST_LITERAL: {
        switch (ast->literal.type.prim) {
        case T_INTEGER:
            PANIC("undeterminate signage of integer");
            break;
        case T_SIGNED:
            switch (ast->literal.type.size) {
            case 1: return LLVMConstInt(LLVMInt8Type(), ast->literal.ival, true);
            case 2: return LLVMConstInt(LLVMInt16Type(), ast->literal.ival, true);
            case 4: return LLVMConstInt(LLVMInt32Type(), ast->literal.ival, true);
            case 8: return LLVMConstInt(LLVMInt64Type(), ast->literal.ival, true);
            default: PANIC("Invalid integer size %d", ast->literal.type.size);
            }
            break;
        case T_UNSIGNED:
            switch (ast->literal.type.size) {
            case 1: return LLVMConstInt(LLVMInt8Type(), ast->literal.ival, false);
            case 2: return LLVMConstInt(LLVMInt16Type(), ast->literal.ival, false);
            case 4: return LLVMConstInt(LLVMInt32Type(), ast->literal.ival, false);
            case 8: return LLVMConstInt(LLVMInt64Type(), ast->literal.ival, false);
            default: PANIC("Invalid integer size %d", ast->literal.type.size);
            }
            break;
        case T_FLOAT:
            switch (ast->literal.type.size) {
            case 4: return LLVMConstReal(LLVMFloatType(), ast->literal.fval);
            case 8: return LLVMConstReal(LLVMDoubleType(), ast->literal.fval);
            default:
                PANIC("invalid float size: %d", ast->literal.type.size);
                return nullptr;
            }
            break;
        case T_BOOL:
            switch (ast->literal.type.size) {
            case 1: return LLVMConstInt(LLVMInt1Type(), ast->literal.bval, false);
            default:
                PANIC("invalid boolean size: %d", ast->literal.type.size);
                return nullptr;
            }
            break;
        case T_INVALID:
        case T_UNKNOWN:
        case T_VOID:
            PANIC("invalid literal type");
            break;
        }
        } break;

    case AST_BINARY_OP: {
        LLVMValueRef lhs = llvm_codegen_expr(llvm, ast->binary_op.lhs);
        LLVMValueRef rhs = llvm_codegen_expr(llvm, ast->binary_op.rhs);

        // TODO(jesper): need to grab the type to know which instruction to emit
        // TypeExpr lhs_type = ast_typecheck(ast->binary_op.lhs, T_UNKNOWN, nullptr, nullptr);
        // TypeExpr rhs_type = ast_typecheck(ast->binary_op.lhs, T_UNKNOWN, nullptr, nullptr);

        switch (ast->binary_op.op.type) {
        case '+': return LLVMBuildAdd(llvm->ir, lhs, rhs, "");
        case '-': return LLVMBuildSub(llvm->ir, lhs, rhs, "");
        case '*': return LLVMBuildMul(llvm->ir, lhs, rhs, "");
        case '/': return LLVMBuildSDiv(llvm->ir, lhs, rhs, "");
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


void* llvm_codegen_proc(LLVMIR *llvm, AST *ast)
{
    PANIC_IF(ast->type != AST_PROC_DECL, "expected AST_PROC_DECL");

    SArena scratch = tl_scratch_arena();

    LLVMTypeRef ret_type = llvm_type_from_type_expr(
        llvm->context,
        ast->proc_decl.ret_type);

    if (!ret_type) ret_type = LLVMVoidType();

    LLVMTypeRef func_type = LLVMFunctionType(ret_type, nullptr, 0, false);
    LLVMValueRef func = LLVMAddFunction(llvm->module, sz_string(ast->proc_decl.identifier.str, scratch), func_type);

    LLVMBasicBlockRef entry = LLVMCreateBasicBlockInContext(llvm->context, "entry");
    LLVMAppendExistingBasicBlock(func, entry);
    LLVMPositionBuilderAtEnd(llvm->ir, entry);

    map_set(&llvm->procedures, ast->proc_decl.identifier.str, {
        .func = func,
        .func_t = func_type,
    });

    llvm->scope.entry = entry;
    map_destroy(&llvm->scope.variables);

    for (auto *stmt = ast->proc_decl.body; stmt; stmt = stmt->next) {
        switch (stmt->type) {
        case AST_VAR_DECL:
            llvm_codegen_expr(llvm, stmt);
            break;
        case AST_RETURN:
            if (stmt->ret.expr) {
                LLVMValueRef val = llvm_codegen_expr(llvm, stmt->ret.expr);
                LLVMBuildRet(llvm->ir, val);
            } else {
                LLVMBuildRetVoid(llvm->ir);
            }
            break;
        default:
            LOG_ERROR("Invalid statement type '%s'", sz_from_enum(stmt->type));
            break;
        }
    }

    return func;
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

    for (i32 i = 1; i < argc; i++) {
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
        LOG_ERROR("No input file");
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
            if (AST *proc = parse_proc_decl(&lexer, &module, mem); proc) {
                array_add(&module.procedures, proc);
                (*ptr) = proc;
                ptr = &proc->next;

                if (proc->proc_decl.identifier == "main") module.entry = proc;
            } else {
                PARSE_ERROR(&lexer, "unknown declaration in global scope");
                return -1;
            }
        }
    }

    for (AST *ast = module.ast; ast; ast = ast->next) {
        if (ast_typecheck(ast, { T_INVALID }, &module, nullptr) == T_INVALID)
            return -1;
    }

    for (AST *ast = module.ast; ast; ast = ast->next) {
        if (ast_sizecheck(ast, &module, nullptr) == T_INVALID)
            return -1;
    }

    debug_print_ast(module.ast);

    LLVMIR llvm{};
    llvm.context = LLVMGetGlobalContext();
    llvm.module = LLVMModuleCreateWithNameInContext("tir", llvm.context);
    llvm.ir = LLVMCreateBuilderInContext(llvm.context);

    {
        SArena scratch = tl_scratch_arena();

        for (AST *it = module.ast; it; it = it->next) {
            if (it->type == AST_PROC_DECL) {
                llvm_codegen_proc(&llvm, it);
            }
        }

        if (char *mod = LLVMPrintModuleToString(llvm.module); mod) {
            LOG_INFO("Generated LLVM IR:\n%s", mod);
            LLVMDisposeMessage(mod);
        }
    }


    DynamicArray<String> object_files{};
    {
        SArena scratch = tl_scratch_arena();

        LLVMInitializeX86TargetInfo();
        LLVMInitializeX86Target();
        LLVMInitializeX86TargetMC();
        LLVMInitializeX86AsmParser();
        LLVMInitializeX86AsmPrinter();

        char *sz_error = nullptr;
        char *target_triple = LLVMGetDefaultTargetTriple();

        LLVMTargetRef target;
        if (LLVMGetTargetFromTriple(target_triple, &target, &sz_error) != 0) {
            LOG_ERROR("Failed to get target from triple '%s': %s", target_triple, sz_error);
            return -1;
        }

        LLVMTargetMachineRef target_machine = LLVMCreateTargetMachine(
            target,
            target_triple, "generic", "",
            LLVMCodeGenLevelDefault,
            LLVMRelocPIC,
            LLVMCodeModelDefault);

        LLVMSetTarget(llvm.module, target_triple);

        FileHandle fd;
        String path;

        if (opts.out_type == OUTPUT_OBJECT) {
            path = stringf(mem_dynamic, "%s/%s.o", out_dir, out_name);
            fd = open_file(path, FILE_OPEN_TRUNCATE);
        } else {
            fd = create_temporary_file(out_name, ".o");
            path = file_path(fd, mem_dynamic);
        }

        defer { if(fd) close_file(fd); };

        LLVMPassManagerRef pass_manager = LLVMCreatePassManager();
        LLVMAddAnalysisPasses(target_machine, pass_manager);
        LLVMRunPassManager(pass_manager, llvm.module);

        LLVMMemoryBufferRef buffer;
        if (LLVMTargetMachineEmitToMemoryBuffer(
                target_machine, llvm.module,
                LLVMObjectFile,
                &sz_error, &buffer) != 0)
        {
            LOG_ERROR("Failed to emit object file: %s", sz_error);
            return -1;
        }

        const char *data = LLVMGetBufferStart(buffer);
        size_t size = LLVMGetBufferSize(buffer);
        write_file(fd, data, size);

        array_add(&object_files, path);
    }

    if (opts.out_type == OUTPUT_EXECUTABLE) {
        SArena scratch = tl_scratch_arena();
        DynamicArray<String> args{ .alloc = scratch };

#if defined(_WIN32)
        String exe_name = stringf(scratch, "%s/%s.exe", out_dir, out_name);
#elif defined(__linux__)
        String exe_name = stringf(scratch, "%s/%s", out_dir, out_name);
#endif

        array_add(&args, object_files);
        array_add(&args, { String{ "-o" }, exe_name });

        run_process("clang", args);
    }

    return 0;
}
