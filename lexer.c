#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

static const char* source;
static int pos;
static int line;
static int col;
static Token* current_token;

static char* copy_string(const char* start, int len) {
    char* str = malloc(len + 1);
    strncpy(str, start, len);
    str[len] = '\0';
    return str;
}

static void skip_whitespace() {
    while (source[pos] != '\0') {
        if (source[pos] == ' ' || source[pos] == '\t') {
            pos++;
            col++;
        } else if (source[pos] == '\n') {
            pos++;
            line++;
            col = 1;
        } else if (source[pos] == '/' && source[pos + 1] == '/') {
            // Skip single-line comment
            while (source[pos] != '\0' && source[pos] != '\n') {
                pos++;
            }
            if (source[pos] == '\n') {
                pos++;
                line++;
                col = 1;
            }
        } else {
            break;
        }
    }
}

static int is_keyword(const char* str, const char* keyword) {
    return strcmp(str, keyword) == 0;
}

static Token* make_token(TokenType type, const char* value) {
    Token* token = malloc(sizeof(Token));
    token->type = type;
    token->value = value ? strdup(value) : NULL;
    token->line = line;
    token->col = col;
    return token;
}

Token* next_token() {
    skip_whitespace();
    
    if (source[pos] == '\0') {
        return make_token(TOKEN_EOF, NULL);
    }
    
    int start_pos = pos;
    int start_col = col;
    
    // String literals
    if (source[pos] == '"') {
        pos++;
        col++;
        start_pos = pos;
        start_col = col;
        
        // Find closing quote, handle escape sequences
        while (source[pos] != '\0' && source[pos] != '"') {
            if (source[pos] == '\\' && source[pos + 1] != '\0') {
                pos += 2;  // Skip escape sequence
                col += 2;
            } else {
                pos++;
                col++;
            }
        }
        
        if (source[pos] == '\0') {
            fprintf(stderr, "Error: unterminated string literal at line %d, col %d\n", line, col);
            return make_token(TOKEN_EOF, NULL);
        }
        
        int len = pos - start_pos;
        char* value = copy_string(source + start_pos, len);
        pos++;  // Skip closing quote
        col++;
        return make_token(TOKEN_STRING_LIT, value);
    }
    
    // Integer literals
    if (isdigit(source[pos])) {
        while (isdigit(source[pos])) {
            pos++;
            col++;
        }
        char* value = copy_string(source + start_pos, pos - start_pos);
        return make_token(TOKEN_INT_LIT, value);
    }
    
    // Identifiers and keywords
    if (isalpha(source[pos]) || source[pos] == '_') {
        while (isalnum(source[pos]) || source[pos] == '_') {
            pos++;
            col++;
        }
        char* value = copy_string(source + start_pos, pos - start_pos);
        
        // Check keywords
        if (is_keyword(value, "fn")) {
            free(value);
            return make_token(TOKEN_FN, NULL);
        }
        if (is_keyword(value, "return")) {
            free(value);
            return make_token(TOKEN_RETURN, NULL);
        }
        if (is_keyword(value, "let")) {
            free(value);
            return make_token(TOKEN_LET, NULL);
        }
        if (is_keyword(value, "call")) {
            free(value);
            return make_token(TOKEN_CALL, NULL);
        }
        if (is_keyword(value, "if")) {
            free(value);
            return make_token(TOKEN_IF, NULL);
        }
        if (is_keyword(value, "else")) {
            free(value);
            return make_token(TOKEN_ELSE, NULL);
        }
        if (is_keyword(value, "while")) {
            free(value);
            return make_token(TOKEN_WHILE, NULL);
        }
        if (is_keyword(value, "int")) {
            free(value);
            return make_token(TOKEN_INT, NULL);
        }
        if (is_keyword(value, "string")) {
            free(value);
            return make_token(TOKEN_STRING, NULL);
        }
        if (is_keyword(value, "print")) {
            free(value);
            return make_token(TOKEN_PRINT, NULL);
        }
        if (is_keyword(value, "malloc")) {
            free(value);
            return make_token(TOKEN_MALLOC, NULL);
        }
        if (is_keyword(value, "free")) {
            free(value);
            return make_token(TOKEN_FREE, NULL);
        }
        
        return make_token(TOKEN_IDENT, value);
    }
    
    // Operators and punctuation
    char ch = source[pos];
    pos++;
    col++;
    
    switch (ch) {
        case '+':
            return make_token(TOKEN_PLUS, NULL);
        case '-':
            if (source[pos] == '>') {
                pos++;
                col++;
                return make_token(TOKEN_ARROW, NULL);
            }
            return make_token(TOKEN_MINUS, NULL);
        case '*':
            return make_token(TOKEN_STAR, NULL);
        case '/':
            return make_token(TOKEN_SLASH, NULL);
        case '=':
            if (source[pos] == '=') {
                pos++;
                col++;
                return make_token(TOKEN_EQ, NULL);
            }
            return make_token(TOKEN_ASSIGN, NULL);
        case '!':
            if (source[pos] == '=') {
                pos++;
                col++;
                return make_token(TOKEN_NE, NULL);
            }
            // Error: unexpected '!'
            return make_token(TOKEN_EOF, NULL);
        case '<':
            if (source[pos] == '=') {
                pos++;
                col++;
                return make_token(TOKEN_LE, NULL);
            }
            return make_token(TOKEN_LT, NULL);
        case '>':
            if (source[pos] == '=') {
                pos++;
                col++;
                return make_token(TOKEN_GE, NULL);
            }
            return make_token(TOKEN_GT, NULL);
        case '(':
            return make_token(TOKEN_LPAREN, NULL);
        case ')':
            return make_token(TOKEN_RPAREN, NULL);
        case '{':
            return make_token(TOKEN_LBRACE, NULL);
        case '}':
            return make_token(TOKEN_RBRACE, NULL);
        case ':':
            return make_token(TOKEN_COLON, NULL);
        case ';':
            return make_token(TOKEN_SEMICOLON, NULL);
        case ',':
            return make_token(TOKEN_COMMA, NULL);
        default:
            return make_token(TOKEN_EOF, NULL);
    }
}

void init_lexer(const char* src) {
    source = src;
    pos = 0;
    line = 1;
    col = 1;
}

void cleanup_lexer() {
    // Cleanup if needed
}

