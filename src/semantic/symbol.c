#include "jag/symbol.h"
#include <stdlib.h>
#include <string.h>

JagSymbolTable *jag_symbol_table_create(JagScopeKind scope_kind, JagSymbolTable *parent) {
    JagSymbolTable *table = calloc(1, sizeof(JagSymbolTable));
    table->scope_kind = scope_kind;
    table->parent = parent;
    return table;
}

void jag_symbol_table_free(JagSymbolTable *table) {
    if (!table) return;
    JagSymbol *curr = table->head;
    while (curr) {
        JagSymbol *next = curr->next;
        free(curr->name);
        free(curr);
        curr = next;
    }
    free(table);
}

bool jag_symbol_table_insert(JagSymbolTable *table, const char *name, JagType *type, JagSymbolKind kind, bool is_fixed, bool is_public, JagSourceLoc loc) {
    if (jag_symbol_table_lookup_current(table, name)) {
        return false;
    }
    JagSymbol *sym = calloc(1, sizeof(JagSymbol));
    sym->name = jag_strdup(name);
    sym->type = type;
    sym->kind = kind;
    sym->is_fixed = is_fixed;
    sym->is_public = is_public;
    sym->loc = loc;
    sym->next = table->head;
    table->head = sym;
    return true;
}

JagSymbol *jag_symbol_table_lookup(JagSymbolTable *table, const char *name) {
    JagSymbolTable *curr_table = table;
    while (curr_table) {
        JagSymbol *sym = jag_symbol_table_lookup_current(curr_table, name);
        if (sym) return sym;
        curr_table = curr_table->parent;
    }
    return NULL;
}

JagSymbol *jag_symbol_table_lookup_current(JagSymbolTable *table, const char *name) {
    if (!table) return NULL;
    JagSymbol *curr = table->head;
    while (curr) {
        if (strcmp(curr->name, name) == 0) return curr;
        curr = curr->next;
    }
    return NULL;
}
