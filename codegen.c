#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"

static IRProgram* current_program;
static Scope* current_scope;
static Scope* global_scope;
static int label_counter = 0;

static char* generate_label(const char* prefix) {
    char* label = malloc(32);
    snprintf(label, 32, "%s_%d", prefix, label_counter++);
    return label;
}

static void gen_expression(ASTNode* node);
static void gen_statement(ASTNode* node);
static void gen_block(ASTNode* block);

static void gen_expression(ASTNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_INT_LIT:
            emit_ir(current_program, IR_PUSH, node->int_value, NULL);
            break;
            
        case AST_STRING_LIT:
            emit_ir_str(current_program, IR_PUSH_STR, node->string_value);
            break;
            
        case AST_MALLOC:
            gen_expression(node->left);  // Size argument
            emit_ir(current_program, IR_MALLOC, 0, NULL);
            break;
            
        case AST_VAR: {
            Symbol* sym = lookup(current_scope, node->var_name);
            if (!sym) {
                fprintf(stderr, "Error: undefined variable '%s'\n", node->var_name);
                exit(1);
            }
            emit_ir(current_program, IR_LOAD, sym->offset, NULL);
            break;
        }
        
        case AST_BINOP:
            gen_expression(node->left);
            gen_expression(node->right);
            switch (node->binop) {
                case BINOP_PLUS:
                    emit_ir(current_program, IR_ADD, 0, NULL);
                    break;
                case BINOP_MINUS:
                    emit_ir(current_program, IR_SUB, 0, NULL);
                    break;
                case BINOP_MULT:
                    emit_ir(current_program, IR_MUL, 0, NULL);
                    break;
                case BINOP_DIV:
                    emit_ir(current_program, IR_DIV, 0, NULL);
                    break;
            }
            break;
            
        case AST_COMPARE:
            gen_expression(node->left);
            gen_expression(node->right);
            // CMP will compare the two values and push 1 (true) or 0 (false)
            emit_ir(current_program, IR_CMP, node->compare_op, NULL);
            break;
            
        case AST_CALL_EXPR: {
            // Push arguments right-to-left (or left-to-right, depending on convention)
            ASTNode* arg = node->args;
            int arg_count = 0;
            while (arg) {
                gen_expression(arg);
                arg = arg->right;
                arg_count++;
            }
            
            // Call function
            char* label = malloc(strlen(node->call_name) + 2);
            sprintf(label, "_%s", node->call_name);
            emit_ir(current_program, IR_CALL, arg_count, label);
            free(label);
            
            // Result is on stack
            break;
        }
        
        default:
            fprintf(stderr, "Error: unexpected node type in expression: %d\n", node->type);
            exit(1);
    }
}

static void gen_statement(ASTNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_VAR_DECL:
            gen_expression(node->left);
            {
                // Declare variable in current scope if not already declared
                Symbol* sym = lookup(current_scope, node->var_name);
                if (!sym) {
                    sym = declare_var(current_scope, node->var_name, SYM_VAR);
                }
                emit_ir(current_program, IR_STORE, sym->offset, NULL);
            }
            break;
            
        case AST_ASSIGN:
            gen_expression(node->left);
            {
                Symbol* sym = lookup(current_scope, node->var_name);
                if (!sym) {
                    fprintf(stderr, "Error: undefined variable '%s'\n", node->var_name);
                    exit(1);
                }
                emit_ir(current_program, IR_STORE, sym->offset, NULL);
            }
            break;
            
        case AST_RETURN:
            gen_expression(node->left);
            emit_ir(current_program, IR_RET, 0, NULL);
            break;
            
        case AST_CALL_STMT: {
            // Generate function call
            ASTNode* arg = node->args;
            int arg_count = 0;
            while (arg) {
                gen_expression(arg);
                arg = arg->right;
                arg_count++;
            }
            
            char* label = malloc(strlen(node->call_name) + 2);
            sprintf(label, "_%s", node->call_name);
            emit_ir(current_program, IR_CALL, arg_count, label);
            free(label);
            
            // Discard return value
            emit_ir(current_program, IR_POP, 0, NULL);
            break;
        }
            
        case AST_IF: {
            char* else_label = generate_label("else");
            char* end_label = generate_label("endif");
            
            // Generate condition
            gen_expression(node->condition);
            
            // Jump to else if condition is false (zero)
            emit_ir(current_program, IR_JZ, 0, else_label);
            
            // Generate then block
            gen_block(node->then_block);
            
            // Jump to end (skip else)
            if (node->else_block) {
                emit_ir(current_program, IR_JMP, 0, end_label);
            }
            
            // Else block label
            if (node->else_block) {
                emit_ir(current_program, IR_LABEL, 0, else_label);
                gen_block(node->else_block);
                emit_ir(current_program, IR_LABEL, 0, end_label);
                free(end_label);
            } else {
                emit_ir(current_program, IR_LABEL, 0, else_label);
            }
            
            free(else_label);
            break;
        }
        
        case AST_WHILE: {
            char* loop_label = generate_label("loop");
            char* end_label = generate_label("endloop");
            
            // Loop start label
            emit_ir(current_program, IR_LABEL, 0, loop_label);
            
            // Generate condition
            gen_expression(node->condition);
            
            // Jump to end if condition is false
            emit_ir(current_program, IR_JZ, 0, end_label);
            
            // Generate body
            gen_block(node->body);
            
            // Jump back to loop start
            emit_ir(current_program, IR_JMP, 0, loop_label);
            
            // End label
            emit_ir(current_program, IR_LABEL, 0, end_label);
            
            free(loop_label);
            free(end_label);
            break;
        }
        
        case AST_BLOCK:
            gen_block(node);
            break;
            
        case AST_PRINT:
            gen_expression(node->left);
            emit_ir(current_program, IR_PRINT, 0, NULL);
            break;
            
        case AST_FREE:
            gen_expression(node->left);
            emit_ir(current_program, IR_FREE, 0, NULL);
            break;
            
        default:
            fprintf(stderr, "Error: unexpected node type in statement: %d\n", node->type);
            exit(1);
    }
}

static void gen_block(ASTNode* block) {
    if (!block || block->type != AST_BLOCK) return;
    
    ASTNode* stmt = block->statements;
    while (stmt) {
        gen_statement(stmt);
        stmt = stmt->right;
    }
}

IRProgram* generate_code(ASTNode* ast) {
    current_program = create_ir_program();
    label_counter = 0;
    
    // Create global scope
    global_scope = create_scope(NULL);
    current_scope = global_scope;
    
    if (!ast || ast->type != AST_PROGRAM) {
        return current_program;
    }
    
    ASTNode* stmt = ast->statements;
    while (stmt) {
        if (stmt->type == AST_FN_DEF) {
            // Function definition - create function scope
            Scope* old_scope = current_scope;
            Scope* fn_scope = create_scope(old_scope);
            current_scope = fn_scope;
            
            // Declare parameters
            ASTNode* param = stmt->params;
            while (param) {
                declare_param(current_scope, param->var_name);
                param = param->right;
            }
            
            // Function label
            char* fn_label = malloc(strlen(stmt->fn_name) + 2);
            sprintf(fn_label, "_%s", stmt->fn_name);
            emit_ir(current_program, IR_LABEL, 0, fn_label);
            free(fn_label);
            
            // Generate function body
            if (stmt->body_nodes) {
                gen_block(stmt->body_nodes);
            }
            
            // If no return statement, add implicit return
            emit_ir(current_program, IR_RET, 0, NULL);
            
            // Restore scope
            current_scope = old_scope;
            // Note: fn_scope cleanup would be done after code generation
        } else {
            gen_statement(stmt);
        }
        
        stmt = stmt->right;
    }
    
    return current_program;
}

