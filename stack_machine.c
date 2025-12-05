#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stack_machine.h"

static const char* get_op_name(IROp op) {
    switch (op) {
        case IR_PUSH: return "PUSH";
        case IR_PUSH_STR: return "PUSH_STR";
        case IR_POP: return "POP";
        case IR_ADD: return "ADD";
        case IR_SUB: return "SUB";
        case IR_MUL: return "MUL";
        case IR_DIV: return "DIV";
        case IR_LOAD: return "LOAD";
        case IR_STORE: return "STORE";
        case IR_CALL: return "CALL";
        case IR_RET: return "RET";
        case IR_JMP: return "JMP";
        case IR_JZ: return "JZ";
        case IR_JNZ: return "JNZ";
        case IR_LABEL: return "LABEL";
        case IR_CMP: return "CMP";
        case IR_PRINT: return "PRINT";
        case IR_MALLOC: return "MALLOC";
        case IR_FREE: return "FREE";
        default: return "UNKNOWN";
    }
}

static const char* get_cmp_name(int cmp_op) {
    switch (cmp_op) {
        case 0: return "EQ";  // COMPARE_EQ
        case 1: return "NE";  // COMPARE_NE
        case 2: return "LT";  // COMPARE_LT
        case 3: return "GT";  // COMPARE_GT
        case 4: return "LE";  // COMPARE_LE
        case 5: return "GE";  // COMPARE_GE
        default: return "UNKNOWN";
    }
}

void generate_assembly(IRProgram* program, const char* output_file) {
    FILE* f = fopen(output_file, "w");
    if (!f) {
        fprintf(stderr, "Error: cannot open output file '%s'\n", output_file);
        exit(1);
    }
    
    // Detect platform (macOS vs Linux)
    #ifdef __APPLE__
    #define PLATFORM_MACOS 1
    #else
    #define PLATFORM_MACOS 0
    #endif
    
    // Collect string literals first
    IRInstruction* instr = program->head;
    int string_counter = 0;
    while (instr) {
        if (instr->op == IR_PUSH_STR && instr->str_value) {
            string_counter++;
        }
        instr = instr->next;
    }
    
    // Write data section for string literals and format strings
    fprintf(f, "section .data\n");
    if (string_counter > 0) {
        instr = program->head;
        int str_idx = 0;
        while (instr) {
            if (instr->op == IR_PUSH_STR && instr->str_value) {
                fprintf(f, "str_%d: db \"%s\", 0\n", str_idx, instr->str_value);
                str_idx++;
            }
            instr = instr->next;
        }
    }
    // Add format strings for printf
    fprintf(f, "fmt_int: db \"%%d\", 10, 0\n");
    fprintf(f, "fmt_str: db \"%%s\", 0\n");
    fprintf(f, "\n");
    
    if (PLATFORM_MACOS) {
        fprintf(f, "section .text\n");
        fprintf(f, "global _main\n");
        fprintf(f, "extern _printf\n");
        fprintf(f, "extern _malloc\n");
        fprintf(f, "extern _free\n\n");
    } else {
        fprintf(f, "section .text\n");
        fprintf(f, "global _start\n");
        fprintf(f, "extern printf\n");
        fprintf(f, "extern malloc\n");
        fprintf(f, "extern free\n\n");
        // Linux entry point - check if main exists and call it
        int has_main = 0;
        IRInstruction* check_instr = program->head;
        while (check_instr) {
            if (check_instr->op == IR_LABEL && check_instr->label && 
                strcmp(check_instr->label, "_main") == 0) {
                has_main = 1;
                break;
            }
            check_instr = check_instr->next;
        }
        if (has_main) {
            fprintf(f, "_start:\n");
            fprintf(f, "    call _main\n");
            fprintf(f, "    mov rax, 60\n");
            fprintf(f, "    mov rdi, 0\n");
            fprintf(f, "    syscall\n\n");
        }
    }
    
    // Generate assembly
    instr = program->head;
    int instruction_num = 0;
    int str_index = 0;
    
    while (instr) {
        switch (instr->op) {
            case IR_LABEL:
                if (instr->label) {
                    fprintf(f, "%s:\n", instr->label);
                }
                break;
                
            case IR_PUSH:
                fprintf(f, "    push %d\n", instr->operand);
                break;
                
            case IR_PUSH_STR: {
                // Find the string index for this string
                int current_str_idx = 0;
                IRInstruction* search_instr = program->head;
                while (search_instr != instr) {
                    if (search_instr->op == IR_PUSH_STR && search_instr->str_value) {
                        current_str_idx++;
                    }
                    search_instr = search_instr->next;
                }
                // Push address of string literal
                fprintf(f, "    lea rax, [rel str_%d]\n", current_str_idx);
                fprintf(f, "    push rax\n");
                break;
            }
                
            case IR_POP:
                fprintf(f, "    pop rax\n");
                break;
                
            case IR_ADD:
                fprintf(f, "    pop rbx\n");
                fprintf(f, "    pop rax\n");
                fprintf(f, "    add rax, rbx\n");
                fprintf(f, "    push rax\n");
                break;
                
            case IR_SUB:
                fprintf(f, "    pop rbx\n");
                fprintf(f, "    pop rax\n");
                fprintf(f, "    sub rax, rbx\n");
                fprintf(f, "    push rax\n");
                break;
                
            case IR_MUL:
                fprintf(f, "    pop rbx\n");
                fprintf(f, "    pop rax\n");
                fprintf(f, "    imul rax, rbx\n");
                fprintf(f, "    push rax\n");
                break;
                
            case IR_DIV:
                fprintf(f, "    pop rbx\n");
                fprintf(f, "    pop rax\n");
                fprintf(f, "    cqo\n");  // Sign extend rax into rdx:rax
                fprintf(f, "    idiv rbx\n");
                fprintf(f, "    push rax\n");
                break;
                
            case IR_LOAD:
                if (instr->operand >= 0) {
                    // Parameter
                    fprintf(f, "    mov rax, [rbp + %d]\n", instr->operand);
                } else {
                    // Local variable
                    fprintf(f, "    mov rax, [rbp %d]\n", instr->operand);
                }
                fprintf(f, "    push rax\n");
                break;
                
            case IR_STORE:
                fprintf(f, "    pop rax\n");
                if (instr->operand >= 0) {
                    // Parameter
                    fprintf(f, "    mov [rbp + %d], rax\n", instr->operand);
                } else {
                    // Local variable
                    fprintf(f, "    mov [rbp %d], rax\n", instr->operand);
                }
                break;
                
            case IR_CALL:
                // Arguments are already on stack
                // Call function
                if (instr->label) {
                    fprintf(f, "    call %s\n", instr->label);
                }
                // Return value is in rax, push it
                fprintf(f, "    push rax\n");
                break;
                
            case IR_RET:
                fprintf(f, "    pop rax\n");
                fprintf(f, "    mov rsp, rbp\n");
                fprintf(f, "    pop rbp\n");
                fprintf(f, "    ret\n");
                break;
                
            case IR_CMP: {
                // Compare two values on stack and push result (1 or 0)
                char true_label[64];
                char end_label[64];
                snprintf(true_label, sizeof(true_label), ".cmp_true_%d", instruction_num);
                snprintf(end_label, sizeof(end_label), ".cmp_end_%d", instruction_num);
                
                fprintf(f, "    pop rbx\n");  // right operand
                fprintf(f, "    pop rax\n");  // left operand
                fprintf(f, "    cmp rax, rbx\n");
                
                // Jump based on comparison operator (in operand field)
                // 0=EQ, 1=NE, 2=LT, 3=GT, 4=LE, 5=GE
                switch (instr->operand) {
                    case 0: // EQ
                        fprintf(f, "    je %s\n", true_label);
                        break;
                    case 1: // NE
                        fprintf(f, "    jne %s\n", true_label);
                        break;
                    case 2: // LT
                        fprintf(f, "    jl %s\n", true_label);
                        break;
                    case 3: // GT
                        fprintf(f, "    jg %s\n", true_label);
                        break;
                    case 4: // LE
                        fprintf(f, "    jle %s\n", true_label);
                        break;
                    case 5: // GE
                        fprintf(f, "    jge %s\n", true_label);
                        break;
                }
                
                // False case: push 0
                fprintf(f, "    push 0\n");
                fprintf(f, "    jmp %s\n", end_label);
                
                // True case: push 1
                fprintf(f, "%s:\n", true_label);
                fprintf(f, "    push 1\n");
                
                fprintf(f, "%s:\n", end_label);
                break;
            }
                
            case IR_JMP:
                if (instr->label) {
                    fprintf(f, "    jmp %s\n", instr->label);
                }
                break;
                
            case IR_JZ: {
                // Jump if zero (equal) - check flags from previous CMP
                fprintf(f, "    pop rax\n");
                fprintf(f, "    test rax, rax\n");
                if (instr->label) {
                    fprintf(f, "    jz %s\n", instr->label);
                }
                break;
            }
                
            case IR_JNZ: {
                // Jump if not zero
                fprintf(f, "    pop rax\n");
                fprintf(f, "    test rax, rax\n");
                if (instr->label) {
                    fprintf(f, "    jnz %s\n", instr->label);
                }
                break;
            }
                
            case IR_PRINT: {
                // Print value/string from stack
                // For simplicity, we'll print as integer if it's a number, or as string if it's a pointer
                // In a real implementation, we'd need type information
                // Here we'll assume: if value < 0x1000, it's an int, else it's a string pointer
                char print_str_label[64];
                char print_end_label[64];
                snprintf(print_str_label, sizeof(print_str_label), ".print_str_%d", instruction_num);
                snprintf(print_end_label, sizeof(print_end_label), ".print_end_%d", instruction_num);
                
                fprintf(f, "    pop rax\n");
                fprintf(f, "    cmp rax, 0x1000\n");
                fprintf(f, "    jge %s\n", print_str_label);
                
                // Print as integer
                if (PLATFORM_MACOS) {
                    fprintf(f, "    lea rdi, [rel fmt_int]\n");
                    fprintf(f, "    mov rsi, rax\n");
                    fprintf(f, "    xor rax, rax\n");  // No vector args
                    fprintf(f, "    call _printf\n");
                } else {
                    fprintf(f, "    mov rsi, rax\n");
                    fprintf(f, "    lea rdi, [rel fmt_int]\n");
                    fprintf(f, "    xor rax, rax\n");  // No vector args
                    fprintf(f, "    call printf\n");
                }
                fprintf(f, "    jmp %s\n", print_end_label);
                
                // Print as string
                fprintf(f, "%s:\n", print_str_label);
                if (PLATFORM_MACOS) {
                    fprintf(f, "    lea rdi, [rel fmt_str]\n");
                    fprintf(f, "    mov rsi, rax\n");
                    fprintf(f, "    xor rax, rax\n");  // No vector args
                    fprintf(f, "    call _printf\n");
                } else {
                    fprintf(f, "    mov rsi, rax\n");
                    fprintf(f, "    lea rdi, [rel fmt_str]\n");
                    fprintf(f, "    xor rax, rax\n");  // No vector args
                    fprintf(f, "    call printf\n");
                }
                
                fprintf(f, "%s:\n", print_end_label);
                break;
            }
                
            case IR_MALLOC: {
                // Allocate memory - size is on stack
                fprintf(f, "    pop rdi\n");  // Size argument
                if (PLATFORM_MACOS) {
                    fprintf(f, "    call _malloc\n");
                } else {
                    fprintf(f, "    call malloc\n");
                }
                fprintf(f, "    push rax\n");  // Push returned pointer
                break;
            }
                
            case IR_FREE: {
                // Free memory - pointer is on stack
                fprintf(f, "    pop rdi\n");  // Pointer argument
                if (PLATFORM_MACOS) {
                    fprintf(f, "    call _free\n");
                } else {
                    fprintf(f, "    call free\n");
                }
                break;
            }
        }
        
        instr = instr->next;
        instruction_num++;
    }
    
    // Add exit code (only if no main function)
    int has_main_check = 0;
    IRInstruction* check_instr2 = program->head;
    while (check_instr2) {
        if (check_instr2->op == IR_LABEL && check_instr2->label && 
            strcmp(check_instr2->label, "_main") == 0) {
            has_main_check = 1;
            break;
        }
        check_instr2 = check_instr2->next;
    }
    if (!has_main_check) {
        if (PLATFORM_MACOS) {
            fprintf(f, "\n    mov rax, 0x2000001  ; exit syscall\n");
        } else {
            fprintf(f, "\n    mov rax, 60  ; exit syscall\n");
        }
        fprintf(f, "    mov rdi, 0\n");
        fprintf(f, "    syscall\n");
    }
    
    fclose(f);
}

