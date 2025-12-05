#ifndef LEXER_H
#define LEXER_H

typedef enum {
    TOKEN_EOF,
    TOKEN_INT_LIT,
    TOKEN_STRING_LIT,
    TOKEN_IDENT,
    
    // Keywords
    TOKEN_FN,
    TOKEN_RETURN,
    TOKEN_LET,
    TOKEN_CALL,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_INT,
    TOKEN_STRING,
    TOKEN_PRINT,
    TOKEN_MALLOC,
    TOKEN_FREE,
    
    // Operators
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_ASSIGN,
    
    // Comparison operators
    TOKEN_EQ,
    TOKEN_NE,
    TOKEN_LT,
    TOKEN_GT,
    TOKEN_LE,
    TOKEN_GE,
    
    // Punctuation
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_COLON,
    TOKEN_SEMICOLON,
    TOKEN_COMMA,
    TOKEN_ARROW
} TokenType;

typedef struct {
    TokenType type;
    char* value;
    int line;
    int col;
} Token;

Token* next_token();
void init_lexer(const char* source);
void cleanup_lexer();

#endif // LEXER_H

