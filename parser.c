#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "symbol_table.h"

static Token* current_token;
static Scope* current_scope;

static ASTNode* parse_expression();
static ASTNode* parse_statement();
static ASTNode* parse_block();

static void expect_token(TokenType expected) {
    if (!current_token) {
        fprintf(stderr, "Error: unexpected EOF, expected token %d\n", expected);
        exit(1);
    }
    if (current_token->type != expected) {
        fprintf(stderr, "Error: expected token %d, got %d at line %d, col %d\n",
                expected, current_token->type, current_token->line, current_token->col);
        fprintf(stderr, "Debug: current token value: %s\n", current_token->value ? current_token->value : "(null)");
        exit(1);
    }
    if (current_token->value) {
        free(current_token->value);
    }
    free(current_token);
    current_token = next_token();
}

static ASTNode* create_ast_node(ASTNodeType type) {
    ASTNode* node = calloc(1, sizeof(ASTNode));
    node->type = type;
    return node;
}

static ASTNode* parse_primary() {
    ASTNode* node = NULL;
    
    if (!current_token) {
        fprintf(stderr, "Error: unexpected EOF in primary expression\n");
        exit(1);
    }
    
    if (!current_token) {
        fprintf(stderr, "Error: unexpected EOF in primary expression\n");
        exit(1);
    }
    
    if (current_token->type == TOKEN_INT_LIT) {
        node = create_ast_node(AST_INT_LIT);
        node->int_value = atoi(current_token->value);
        free(current_token->value);
        free(current_token);
        current_token = next_token();
    } else if (current_token->type == TOKEN_STRING_LIT) {
        node = create_ast_node(AST_STRING_LIT);
        node->string_value = strdup(current_token->value);
        free(current_token->value);
        free(current_token);
        current_token = next_token();
    } else if (current_token->type == TOKEN_IDENT) {
        // Could be variable or function call - peek ahead
        char* ident_name = strdup(current_token->value);
        Token* peek_token = next_token();
        
        if (peek_token && peek_token->type == TOKEN_LPAREN) {
            // Function call expression
            free(current_token->value);
            free(current_token);
            current_token = peek_token;
            
            node = create_ast_node(AST_CALL_EXPR);
            node->call_name = ident_name;
            expect_token(TOKEN_LPAREN);
            
            // Parse arguments
            ASTNode* args = NULL;
            ASTNode* last_arg = NULL;
            
            if (current_token && current_token->type != TOKEN_RPAREN) {
                ASTNode* arg = parse_expression();
                args = arg;
                last_arg = arg;
                
                while (current_token && current_token->type == TOKEN_COMMA) {
                    expect_token(TOKEN_COMMA);
                    ASTNode* next_arg = parse_expression();
                    last_arg->right = next_arg;
                    last_arg = next_arg;
                }
            }
            
            node->args = args;
            expect_token(TOKEN_RPAREN);
        } else {
            // Variable reference
            node = create_ast_node(AST_VAR);
            node->var_name = ident_name;
            free(current_token->value);
            free(current_token);
            // Use the peeked token as the next current_token (or get next if peek was NULL)
            if (peek_token) {
                current_token = peek_token;
            } else {
                current_token = next_token();
            }
        }
    } else if (current_token && current_token->type == TOKEN_MALLOC) {
        // Malloc call (as expression)
        expect_token(TOKEN_MALLOC);
        expect_token(TOKEN_LPAREN);
        node = create_ast_node(AST_MALLOC);
        node->left = parse_expression();  // Size argument
        expect_token(TOKEN_RPAREN);
    } else if (current_token && current_token->type == TOKEN_LPAREN) {
        expect_token(TOKEN_LPAREN);
        node = parse_expression();
        expect_token(TOKEN_RPAREN);
    }
    
    if (!node) {
        fprintf(stderr, "Error: failed to parse primary expression\n");
        exit(1);
    }
    
    return node;
}

static ASTNode* parse_unary() {
    if (current_token && current_token->type == TOKEN_MINUS) {
        expect_token(TOKEN_MINUS);
        ASTNode* node = create_ast_node(AST_BINOP);
        node->binop = BINOP_MINUS;
        node->left = create_ast_node(AST_INT_LIT);
        node->left->int_value = 0;
        node->right = parse_unary();
        return node;
    }
    return parse_primary();
}

static ASTNode* parse_multiplicative() {
    ASTNode* left = parse_unary();
    
    while (current_token && (current_token->type == TOKEN_STAR || current_token->type == TOKEN_SLASH)) {
        ASTNode* node = create_ast_node(AST_BINOP);
        if (current_token->type == TOKEN_STAR) {
            node->binop = BINOP_MULT;
            expect_token(TOKEN_STAR);
        } else {
            node->binop = BINOP_DIV;
            expect_token(TOKEN_SLASH);
        }
        node->left = left;
        node->right = parse_unary();
        left = node;
    }
    
    return left;
}

static ASTNode* parse_additive() {
    ASTNode* left = parse_multiplicative();
    
    while (current_token && (current_token->type == TOKEN_PLUS || current_token->type == TOKEN_MINUS)) {
        ASTNode* node = create_ast_node(AST_BINOP);
        if (current_token->type == TOKEN_PLUS) {
            node->binop = BINOP_PLUS;
            expect_token(TOKEN_PLUS);
        } else {
            node->binop = BINOP_MINUS;
            expect_token(TOKEN_MINUS);
        }
        node->left = left;
        node->right = parse_multiplicative();
        left = node;
    }
    
    return left;
}

static ASTNode* parse_comparison() {
    ASTNode* left = parse_additive();
    
    if (current_token && (current_token->type == TOKEN_EQ || current_token->type == TOKEN_NE ||
        current_token->type == TOKEN_LT || current_token->type == TOKEN_GT ||
        current_token->type == TOKEN_LE || current_token->type == TOKEN_GE)) {
        ASTNode* node = create_ast_node(AST_COMPARE);
        
        if (current_token->type == TOKEN_EQ) {
            node->compare_op = COMPARE_EQ;
            expect_token(TOKEN_EQ);
        } else if (current_token->type == TOKEN_NE) {
            node->compare_op = COMPARE_NE;
            expect_token(TOKEN_NE);
        } else if (current_token->type == TOKEN_LT) {
            node->compare_op = COMPARE_LT;
            expect_token(TOKEN_LT);
        } else if (current_token->type == TOKEN_GT) {
            node->compare_op = COMPARE_GT;
            expect_token(TOKEN_GT);
        } else if (current_token->type == TOKEN_LE) {
            node->compare_op = COMPARE_LE;
            expect_token(TOKEN_LE);
        } else if (current_token->type == TOKEN_GE) {
            node->compare_op = COMPARE_GE;
            expect_token(TOKEN_GE);
        }
        
        node->left = left;
        node->right = parse_additive();
        return node;
    }
    
    return left;
}

static ASTNode* parse_expression() {
    return parse_comparison();
}

static ASTNode* parse_block() {
    expect_token(TOKEN_LBRACE);
    ASTNode* block = create_ast_node(AST_BLOCK);
    block->statements = NULL;
    block->stmt_count = 0;
    
    ASTNode* last_stmt = NULL;
    
    while (current_token && current_token->type != TOKEN_RBRACE) {
        ASTNode* stmt = parse_statement();
        
        if (block->statements == NULL) {
            block->statements = stmt;
            last_stmt = stmt;
        } else {
            last_stmt->right = stmt;
            last_stmt = stmt;
        }
        block->stmt_count++;
    }
    
    expect_token(TOKEN_RBRACE);
    return block;
}

static ASTNode* parse_statement() {
    ASTNode* node = NULL;
    
    if (current_token && current_token->type == TOKEN_LET) {
        // Variable declaration
        expect_token(TOKEN_LET);
        char* var_name = strdup(current_token->value);
        expect_token(TOKEN_IDENT);
        expect_token(TOKEN_COLON);
        
        // Support both int and string types
        int is_string = 0;
        if (current_token && current_token->type == TOKEN_STRING) {
            expect_token(TOKEN_STRING);
            is_string = 1;
        } else {
            expect_token(TOKEN_INT);
        }
        
        expect_token(TOKEN_ASSIGN);
        
        node = create_ast_node(AST_VAR_DECL);
        node->var_name = var_name;
        node->int_value = is_string ? 1 : 0;  // Use int_value as type flag: 1 = string, 0 = int
        node->left = parse_expression();
        expect_token(TOKEN_SEMICOLON);
        
        if (current_scope) {
            declare_var(current_scope, var_name, SYM_VAR);
        }
    } else if (current_token && current_token->type == TOKEN_IDENT) {
        // Could be assignment or function call
        char* name = strdup(current_token->value);
        free(current_token->value);
        free(current_token);
        current_token = next_token();
        
        if (current_token && current_token->type == TOKEN_ASSIGN) {
            // Assignment
            expect_token(TOKEN_ASSIGN);
            node = create_ast_node(AST_ASSIGN);
            node->var_name = name;
            node->left = parse_expression();
            expect_token(TOKEN_SEMICOLON);
        } else if (current_token && current_token->type == TOKEN_LPAREN) {
            // Function call statement
            node = create_ast_node(AST_CALL_STMT);
            node->call_name = name;
            expect_token(TOKEN_LPAREN);
            
            ASTNode* args = NULL;
            ASTNode* last_arg = NULL;
            
            if (current_token && current_token->type != TOKEN_RPAREN) {
                ASTNode* arg = parse_expression();
                args = arg;
                last_arg = arg;
                
                while (current_token && current_token->type == TOKEN_COMMA) {
                    expect_token(TOKEN_COMMA);
                    ASTNode* next_arg = parse_expression();
                    last_arg->right = next_arg;
                    last_arg = next_arg;
                }
            }
            
            node->args = args;
            expect_token(TOKEN_RPAREN);
            expect_token(TOKEN_SEMICOLON);
        }
    } else if (current_token && current_token->type == TOKEN_CALL) {
        // Call statement
        expect_token(TOKEN_CALL);
        expect_token(TOKEN_IDENT);
        node = create_ast_node(AST_CALL_STMT);
        node->call_name = strdup(current_token->value);
        free(current_token->value);
        free(current_token);
        current_token = next_token();
        expect_token(TOKEN_LPAREN);
        
        ASTNode* args = NULL;
        ASTNode* last_arg = NULL;
        
        if (current_token && current_token->type != TOKEN_RPAREN) {
            ASTNode* arg = parse_expression();
            args = arg;
            last_arg = arg;
            
            while (current_token && current_token->type == TOKEN_COMMA) {
                expect_token(TOKEN_COMMA);
                ASTNode* next_arg = parse_expression();
                last_arg->right = next_arg;
                last_arg = next_arg;
            }
        }
        
        node->args = args;
        expect_token(TOKEN_RPAREN);
        expect_token(TOKEN_SEMICOLON);
    } else if (current_token && current_token->type == TOKEN_RETURN) {
        // Return statement
        expect_token(TOKEN_RETURN);
        node = create_ast_node(AST_RETURN);
        node->left = parse_expression();
        expect_token(TOKEN_SEMICOLON);
    } else if (current_token && current_token->type == TOKEN_IF) {
        // If statement
        expect_token(TOKEN_IF);
        expect_token(TOKEN_LPAREN);
        node = create_ast_node(AST_IF);
        node->condition = parse_expression();
        expect_token(TOKEN_RPAREN);
        node->then_block = parse_block();
        
        if (current_token && current_token->type == TOKEN_ELSE) {
            expect_token(TOKEN_ELSE);
            // Handle else if - if next token is IF, parse it as an if statement
            // Otherwise parse a block
            if (current_token && current_token->type == TOKEN_IF) {
                // else if - create a block containing the if statement
                ASTNode* else_if_block = create_ast_node(AST_BLOCK);
                else_if_block->statements = parse_statement(); // Parse the if statement
                else_if_block->stmt_count = 1;
                node->else_block = else_if_block;
            } else {
                // Regular else block
                node->else_block = parse_block();
            }
        }
    } else if (current_token && current_token->type == TOKEN_WHILE) {
        // While loop
        expect_token(TOKEN_WHILE);
        expect_token(TOKEN_LPAREN);
        node = create_ast_node(AST_WHILE);
        node->condition = parse_expression();
        expect_token(TOKEN_RPAREN);
        node->body = parse_block();
    } else if (current_token && current_token->type == TOKEN_PRINT) {
        // Print statement
        expect_token(TOKEN_PRINT);
        expect_token(TOKEN_LPAREN);
        node = create_ast_node(AST_PRINT);
        node->left = parse_expression();
        expect_token(TOKEN_RPAREN);
        expect_token(TOKEN_SEMICOLON);
    } else if (current_token && current_token->type == TOKEN_FREE) {
        // Free statement
        expect_token(TOKEN_FREE);
        expect_token(TOKEN_LPAREN);
        node = create_ast_node(AST_FREE);
        node->left = parse_expression();  // Pointer argument
        expect_token(TOKEN_RPAREN);
        expect_token(TOKEN_SEMICOLON);
    } else if (current_token && current_token->type == TOKEN_LBRACE) {
        // Block statement
        node = parse_block();
    } else {
        if (!current_token) {
            fprintf(stderr, "Unexpected EOF in statement\n");
        } else {
            fprintf(stderr, "Unexpected token in statement: %d\n", current_token->type);
        }
        exit(1);
    }
    
    return node;
}

ASTNode* parse_program() {
    current_token = next_token();
    current_scope = create_scope(NULL);
    
    ASTNode* program = create_ast_node(AST_PROGRAM);
    program->statements = NULL;
    program->stmt_count = 0;
    
    ASTNode* last_stmt = NULL;
    
    while (current_token && current_token->type != TOKEN_EOF) {
        ASTNode* stmt = NULL;
        
        if (current_token && current_token->type == TOKEN_FN) {
            // Function definition
            expect_token(TOKEN_FN);
            char* fn_name = strdup(current_token->value);
            expect_token(TOKEN_IDENT);
            expect_token(TOKEN_LPAREN);
            
            stmt = create_ast_node(AST_FN_DEF);
            stmt->fn_name = fn_name;
            stmt->params = NULL;
            
            Scope* fn_scope = create_scope(current_scope);
            Scope* old_scope = current_scope;
            current_scope = fn_scope;
            
            ASTNode* last_param = NULL;
            
            if (current_token && current_token->type != TOKEN_RPAREN) {
                char* param_name = strdup(current_token->value);
                expect_token(TOKEN_IDENT);
                ASTNode* param = create_ast_node(AST_VAR_DECL);
                param->var_name = param_name;
                expect_token(TOKEN_COLON);
                expect_token(TOKEN_INT);
                
                declare_param(current_scope, param_name);
                stmt->params = param;
                last_param = param;
                
                while (current_token && current_token->type == TOKEN_COMMA) {
                    expect_token(TOKEN_COMMA);
                    char* next_param_name = strdup(current_token->value);
                    expect_token(TOKEN_IDENT);
                    ASTNode* next_param = create_ast_node(AST_VAR_DECL);
                    next_param->var_name = next_param_name;
                    expect_token(TOKEN_COLON);
                    expect_token(TOKEN_INT);
                    
                    declare_param(current_scope, next_param_name);
                    last_param->right = next_param;
                    last_param = next_param;
                }
            }
            
            expect_token(TOKEN_RPAREN);
            expect_token(TOKEN_ARROW);
            expect_token(TOKEN_INT);
            stmt->body_nodes = parse_block();
            
            current_scope = old_scope;
            // Note: fn_scope cleanup would be done after code generation
        } else {
            stmt = parse_statement();
        }
        
        if (program->statements == NULL) {
            program->statements = stmt;
            last_stmt = stmt;
        } else {
            last_stmt->right = stmt;
            last_stmt = stmt;
        }
        program->stmt_count++;
    }
    
    return program;
}

void free_ast(ASTNode* node) {
    if (!node) return;
    
    // Free child nodes first
    // Note: For BLOCK and PROGRAM nodes, statements are linked via 'right' pointer
    // and should be freed separately to avoid double-free through other paths
    
    // Free direct child nodes (these are not in statements list)
    free_ast(node->left);
    free_ast(node->right);
    free_ast(node->condition);
    free_ast(node->then_block);
    free_ast(node->else_block);
    free_ast(node->body);
    free_ast(node->params);
    free_ast(node->args);
    free_ast(node->body_nodes);
    
    // Free statements list if it exists (for BLOCK and PROGRAM nodes)
    // These statements are linked via 'right' pointer and must be freed carefully
    if (node->statements) {
        ASTNode* stmt = node->statements;
        while (stmt) {
            ASTNode* next = stmt->right;
            // Clear the right pointer before freeing to avoid issues
            stmt->right = NULL;
            // Recursively free the statement
            free_ast(stmt);
            stmt = next;
        }
        node->statements = NULL;
    }
    
    // Free string pointers
    if (node->var_name) {
        free(node->var_name);
        node->var_name = NULL;
    }
    if (node->fn_name) {
        free(node->fn_name);
        node->fn_name = NULL;
    }
    if (node->call_name) {
        free(node->call_name);
        node->call_name = NULL;
    }
    if (node->string_value) {
        free(node->string_value);
        node->string_value = NULL;
    }
    
    // Free the node itself
    free(node);
}

