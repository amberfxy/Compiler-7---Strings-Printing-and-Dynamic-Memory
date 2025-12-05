#ifndef STACK_MACHINE_IR_H
#define STACK_MACHINE_IR_H

typedef enum {
    IR_PUSH,
    IR_PUSH_STR,  // Push string literal address
    IR_POP,
    IR_ADD,
    IR_SUB,
    IR_MUL,
    IR_DIV,
    IR_LOAD,
    IR_STORE,
    IR_CALL,
    IR_RET,
    IR_JMP,      // Unconditional jump
    IR_JZ,       // Jump if zero
    IR_JNZ,      // Jump if not zero
    IR_LABEL,    // Label definition
    IR_CMP,      // Compare (sets flags for conditional jumps)
    IR_PRINT,    // Print value/string
    IR_MALLOC,   // Allocate memory
    IR_FREE      // Free memory
} IROp;

typedef struct IRInstruction {
    IROp op;
    int operand;  // For PUSH, LOAD, STORE, etc.
    char* label;  // For jumps and labels
    char* str_value;  // For string literals
    struct IRInstruction* next;
} IRInstruction;

typedef struct {
    IRInstruction* head;
    IRInstruction* tail;
} IRProgram;

IRProgram* create_ir_program();
void emit_ir(IRProgram* program, IROp op, int operand, const char* label);
void emit_ir_str(IRProgram* program, IROp op, const char* str_value);
void free_ir_program(IRProgram* program);

#endif // STACK_MACHINE_IR_H

