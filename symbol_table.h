#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

typedef enum {
    SYM_VAR,
    SYM_FN,
    SYM_PARAM
} SymType;

typedef struct Symbol {
    char* name;
    SymType type;
    int offset;  // Stack offset
    struct Symbol* next;
} Symbol;

typedef struct Scope {
    Symbol* symbols;
    struct Scope* parent;
    int local_count;
    int param_count;
} Scope;

Scope* create_scope(Scope* parent);
void destroy_scope(Scope* scope);
Symbol* lookup(Scope* scope, const char* name);
Symbol* declare_var(Scope* scope, const char* name, SymType type);
Symbol* declare_param(Scope* scope, const char* name);

#endif // SYMBOL_TABLE_H

