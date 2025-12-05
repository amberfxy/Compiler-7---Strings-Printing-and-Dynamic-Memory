#ifndef PARSER_H
#define PARSER_H

typedef enum {
    AST_INT_LIT,
    AST_STRING_LIT,
    AST_VAR,
    AST_BINOP,
    AST_ASSIGN,
    AST_VAR_DECL,
    AST_RETURN,
    AST_CALL_EXPR,
    AST_CALL_STMT,
    AST_FN_DEF,
    AST_PROGRAM,
    AST_BLOCK,
    AST_IF,
    AST_WHILE,
    AST_COMPARE,
    AST_PRINT,
    AST_MALLOC,
    AST_FREE
} ASTNodeType;

typedef enum {
    BINOP_PLUS,
    BINOP_MINUS,
    BINOP_MULT,
    BINOP_DIV
} BinOpType;

typedef enum {
    COMPARE_EQ,
    COMPARE_NE,
    COMPARE_LT,
    COMPARE_GT,
    COMPARE_LE,
    COMPARE_GE
} CompareOpType;

typedef struct ASTNode ASTNode;

struct ASTNode {
    ASTNodeType type;
    ASTNode* left;
    ASTNode* right;
    ASTNode* condition;  // For if/while
    ASTNode* then_block; // For if
    ASTNode* else_block; // For if/else
    ASTNode* body;       // For while
    
    // For function definitions
    char* fn_name;
    ASTNode* params;
    ASTNode* body_nodes;
    
    // For function calls
    char* call_name;
    ASTNode* args;
    
    // For variables
    char* var_name;
    
    // For literals
    int int_value;
    char* string_value;  // For string literals
    
    // For binary operations
    BinOpType binop;
    
    // For comparison operations
    CompareOpType compare_op;
    
    // For blocks
    ASTNode* statements;
    int stmt_count;
};

ASTNode* parse_program();
void free_ast(ASTNode* node);

#endif // PARSER_H

