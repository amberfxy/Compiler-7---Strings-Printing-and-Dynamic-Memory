#ifndef CODEGEN_H
#define CODEGEN_H

#include "parser.h"
#include "stack_machine_ir.h"
#include "symbol_table.h"

IRProgram* generate_code(ASTNode* ast);

#endif // CODEGEN_H

