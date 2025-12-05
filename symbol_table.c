#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symbol_table.h"

#define HASH_SIZE 101

static unsigned int hash(const char* name) {
    unsigned int h = 0;
    for (const char* p = name; *p; p++) {
        h = h * 31 + *p;
    }
    return h % HASH_SIZE;
}

Scope* create_scope(Scope* parent) {
    Scope* scope = malloc(sizeof(Scope));
    scope->symbols = NULL;
    scope->parent = parent;
    scope->local_count = 0;
    scope->param_count = 0;
    return scope;
}

void destroy_scope(Scope* scope) {
    Symbol* sym = scope->symbols;
    while (sym) {
        Symbol* next = sym->next;
        free(sym->name);
        free(sym);
        sym = next;
    }
    free(scope);
}

Symbol* lookup(Scope* scope, const char* name) {
    Scope* current = scope;
    while (current) {
        Symbol* sym = current->symbols;
        while (sym) {
            if (strcmp(sym->name, name) == 0) {
                return sym;
            }
            sym = sym->next;
        }
        current = current->parent;
    }
    return NULL;
}

Symbol* declare_var(Scope* scope, const char* name, SymType type) {
    if (!scope) {
        fprintf(stderr, "Error: declare_var called with NULL scope\n");
        exit(1);
    }
    // Check if already declared in current scope
    Symbol* existing = scope->symbols;
    while (existing) {
        if (strcmp(existing->name, name) == 0) {
            return existing;  // Already exists
        }
        existing = existing->next;
    }
    
    Symbol* sym = malloc(sizeof(Symbol));
    sym->name = strdup(name);
    sym->type = type;
    scope->local_count++;
    // Local variables use negative offsets from rbp
    sym->offset = -(scope->local_count * 8);
    sym->next = scope->symbols;
    scope->symbols = sym;
    return sym;
}

Symbol* declare_param(Scope* scope, const char* name) {
    Symbol* sym = malloc(sizeof(Symbol));
    sym->name = strdup(name);
    sym->type = SYM_PARAM;
    scope->param_count++;
    // Parameters use positive offsets: first param at [rbp+16], second at [rbp+24], etc.
    sym->offset = 16 + (scope->param_count - 1) * 8;
    sym->next = scope->symbols;
    scope->symbols = sym;
    return sym;
}

