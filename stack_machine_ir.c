#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stack_machine_ir.h"

IRProgram* create_ir_program() {
    IRProgram* program = malloc(sizeof(IRProgram));
    program->head = NULL;
    program->tail = NULL;
    return program;
}

void emit_ir(IRProgram* program, IROp op, int operand, const char* label) {
    IRInstruction* instr = malloc(sizeof(IRInstruction));
    instr->op = op;
    instr->operand = operand;
    instr->label = label ? strdup(label) : NULL;
    instr->str_value = NULL;
    instr->next = NULL;
    
    if (program->tail) {
        program->tail->next = instr;
        program->tail = instr;
    } else {
        program->head = instr;
        program->tail = instr;
    }
}

void emit_ir_str(IRProgram* program, IROp op, const char* str_value) {
    IRInstruction* instr = malloc(sizeof(IRInstruction));
    instr->op = op;
    instr->operand = 0;
    instr->label = NULL;
    instr->str_value = str_value ? strdup(str_value) : NULL;
    instr->next = NULL;
    
    if (program->tail) {
        program->tail->next = instr;
        program->tail = instr;
    } else {
        program->head = instr;
        program->tail = instr;
    }
}

void free_ir_program(IRProgram* program) {
    IRInstruction* instr = program->head;
    while (instr) {
        IRInstruction* next = instr->next;
        if (instr->label) {
            free(instr->label);
        }
        if (instr->str_value) {
            free(instr->str_value);
        }
        free(instr);
        instr = next;
    }
    free(program);
}

