# CS5008 – Compiler 7: Strings, Printing, and Dynamic Memory

**Author:** Xinyuan Fan (Amber)  
**NUID:** 002059850

---

## Overview

This project extends the Jive compiler from Compiler 6 to implement strings, printing, and dynamic memory management. The implementation includes:

* **String literals**: Support for string literals with double quotes
* **String variables**: String type declarations and assignments
* **Print statements**: `print()` function to print both integers and strings
* **Dynamic memory**: `malloc()` and `free()` functions for memory management
* **Comment support**: Single-line comments with `//`

---

## Features Implemented

### 1. String Literals
- String literals are enclosed in double quotes: `"Hello, World!"`
- Strings are stored in the data section of the generated assembly
- String variables can be declared and assigned

### 2. String Type
- New `string` type keyword for variable declarations
- Syntax: `let msg: string = "Hello";`

### 3. Print Statement
- `print()` function can print both integers and strings
- Automatically detects type based on value (heuristic: values < 0x1000 are integers, else strings)
- Syntax: `print(value);`

### 4. Dynamic Memory
- `malloc(size)` - Allocates memory of specified size, returns pointer
- `free(ptr)` - Frees previously allocated memory
- Both can be used in expressions and statements

### 5. Comments
- Single-line comments with `//`
- Comments are ignored by the lexer

---

## Project Structure

| File                                        | Description                                                                                                      |
| ------------------------------------------- | ---------------------------------------------------------------------------------------------------------------- |
| lexer.c / lexer.h                           | Lexical analyzer with tokens for strings, print, malloc, free, and comment support                                |
| parser.c / parser.h                         | Parser with support for string literals, string variables, print statements, and memory operations              |
| symbol_table.c / symbol_table.h              | Symbol table (unchanged from Compiler 6)                                                                        |
| stack_machine_ir.c / stack_machine_ir.h     | IR definitions: added PUSH_STR, PRINT, MALLOC, and FREE operations                                             |
| codegen.c                                   | Code generation: generates IR for strings, print, and memory operations                                         |
| stack_machine.c                             | Assembly generation: converts string and memory IR to x86-64 assembly with printf, malloc, and free calls     |
| main.c                                      | Compiler driver                                                                                                  |
| main.jive                                   | Test program demonstrating strings, printing, and dynamic memory                                                 |

---

## Build & Run

### Compilation

```bash
# Compile the compiler
gcc -o compiler lexer.c parser.c symbol_table.c codegen.c \
    stack_machine.c stack_machine_ir.c main.c
```

### Usage

```bash
# Compile a Jive source file to assembly
./compiler main.jive out.asm

# Assemble and link (macOS/Mach-O64)
nasm -f macho64 out.asm -o out.o
gcc out.o -o a.out

# Assemble and link (Linux/ELF64)
nasm -f elf64 out.asm -o out.o
gcc out.o -o a.out

# Execute
./a.out
```

---

## Implementation Details

### String Literals

Strings are stored in the `.data` section of the generated assembly:

```assembly
section .data
str_0: db "Hello, World!", 0
```

String addresses are pushed onto the stack using `lea` instruction:

```assembly
lea rax, [rel str_0]
push rax
```

### Print Statement

The print function uses a heuristic to determine if a value is an integer or string:
- If value < 0x1000: print as integer using `%d` format
- Otherwise: print as string using `%s` format

The implementation calls `printf` from the C standard library:

```assembly
lea rdi, [rel fmt_str]  ; or fmt_int
mov rsi, rax
xor rax, rax
call _printf
```

### Dynamic Memory

`malloc` and `free` are implemented as calls to the C standard library functions:

```assembly
; malloc(size)
pop rdi
call _malloc
push rax  ; Return pointer

; free(ptr)
pop rdi
call _free
```

### Comment Support

Single-line comments are handled in the lexer's `skip_whitespace()` function:

```c
else if (source[pos] == '/' && source[pos + 1] == '/') {
    // Skip single-line comment
    while (source[pos] != '\0' && source[pos] != '\n') {
        pos++;
    }
}
```

---

## Example Program

See `main.jive` for a complete example:

```jive
fn main() -> int {
    // Test string literals and printing
    let msg: string = "Hello, World!";
    print(msg);
    
    // Test integer printing
    let x: int = 42;
    print(x);
    
    // Test dynamic memory allocation
    let size: int = 100;
    let ptr: int = malloc(size);
    
    // Test free
    free(ptr);
    
    return 0;
}
```

---

## Changes from Compiler 6

### Lexer (lexer.h, lexer.c)
- Added `TOKEN_STRING_LIT` for string literals
- Added `TOKEN_STRING` keyword for string type
- Added `TOKEN_PRINT`, `TOKEN_MALLOC`, `TOKEN_FREE` keywords
- Added comment handling in `skip_whitespace()`
- Added string literal parsing with escape sequence support

### Parser (parser.h, parser.c)
- Added `AST_STRING_LIT` node type
- Added `AST_PRINT`, `AST_MALLOC`, `AST_FREE` node types
- Added `string_value` field to ASTNode for string literals
- Updated variable declaration parsing to support string type
- Added parsing for print, malloc, and free statements/expressions

### IR (stack_machine_ir.h, stack_machine_ir.c)
- Added `IR_PUSH_STR` for pushing string literal addresses
- Added `IR_PRINT`, `IR_MALLOC`, `IR_FREE` operations
- Added `str_value` field to IRInstruction
- Added `emit_ir_str()` function for string IR instructions

### Code Generator (codegen.c)
- Added code generation for `AST_STRING_LIT` → `IR_PUSH_STR`
- Added code generation for `AST_PRINT` → `IR_PRINT`
- Added code generation for `AST_MALLOC` → `IR_MALLOC`
- Added code generation for `AST_FREE` → `IR_FREE`

### Stack Machine (stack_machine.c)
- Added data section generation for string literals
- Added format strings (`fmt_int`, `fmt_str`) for printf
- Added assembly generation for `IR_PUSH_STR`
- Added assembly generation for `IR_PRINT` with type detection
- Added assembly generation for `IR_MALLOC` and `IR_FREE`
- Added extern declarations for `printf`, `malloc`, `free`

---

## Testing

The compiler has been tested with:
- String literal declarations and assignments
- Print statements for strings and integers
- Dynamic memory allocation with malloc
- Memory deallocation with free
- Nested function calls with strings
- Comments in source code

---

## Notes

- String type checking is minimal - the compiler uses heuristics to determine if a value should be printed as an integer or string
- Memory management follows C conventions - malloc returns a pointer (stored as int), free takes a pointer
- String concatenation and other string operations are not yet implemented
- The compiler generates assembly that links with the C standard library for printf, malloc, and free

---

## About

CS5008 Homework 7 - Compiler Implementation: Strings, Printing, and Dynamic Memory

