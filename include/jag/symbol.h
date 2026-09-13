#ifndef JAG_SYMBOL_H
#define JAG_SYMBOL_H

#include "jag/common.h"
#include "jag/types.h"

typedef enum {
    SYMBOL_VAR,
    SYMBOL_FIXED,
    SYMBOL_FUN,
    SYMBOL_CLASS,
    SYMBOL_STRUCT,
    SYMBOL_ENUM
} JagSymbolKind;

typedef struct JagSymbol JagSymbol;

struct JagSymbol {
    char *name;
    JagType *type;
    JagSymbolKind kind;
    bool is_fixed;
    bool is_public;
    JagSourceLoc loc;
    JagSymbol *next;
};

typedef enum {
    SCOPE_GLOBAL,
    SCOPE_MODULE,
    SCOPE_FUNCTION,
    SCOPE_BLOCK,
    SCOPE_CLASS
} JagScopeKind;

typedef struct JagSymbolTable JagSymbolTable;

struct JagSymbolTable {
    JagScopeKind scope_kind;
    JagSymbolTable *parent;
    JagSymbol *head;
};

JagSymbolTable *jag_symbol_table_create(JagScopeKind scope_kind, JagSymbolTable *parent);
void jag_symbol_table_free(JagSymbolTable *table);
bool jag_symbol_table_insert(JagSymbolTable *table, const char *name, JagType *type, JagSymbolKind kind, bool is_fixed, bool is_public, JagSourceLoc loc);
JagSymbol *jag_symbol_table_lookup(JagSymbolTable *table, const char *name);
JagSymbol *jag_symbol_table_lookup_current(JagSymbolTable *table, const char *name);

#endif // JAG_SYMBOL_H
