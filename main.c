#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "stack_machine.h"
#include "symbol_table.h"

static char* read_file(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Error: cannot open file '%s'\n", filename);
        exit(1);
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char* content = malloc(size + 1);
    fread(content, 1, size, f);
    content[size] = '\0';
    
    fclose(f);
    return content;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input.jive> <output.asm>\n", argv[0]);
        return 1;
    }
    
    char* source = read_file(argv[1]);
    
    init_lexer(source);
    ASTNode* ast = parse_program();
    cleanup_lexer();
    
    IRProgram* ir = generate_code(ast);
    if (ir) {
        generate_assembly(ir, argv[2]);
        printf("Compilation successful. Output: %s\n", argv[2]);
        
        // Cleanup after successful generation
        if (ir) free_ir_program(ir);
        if (ast) free_ast(ast);
        if (source) free(source);
        return 0;
    } else {
        fprintf(stderr, "Error: code generation failed\n");
        if (ast) free_ast(ast);
        if (source) free(source);
        return 1;
    }
}

