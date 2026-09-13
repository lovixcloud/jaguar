#ifndef JAG_SEMANTIC_H
#define JAG_SEMANTIC_H

#include "jag/ast.h"
#include "jag/diagnostics.h"
#include "jag/symbol.h"
#include "jag/types.h"

typedef struct {
    JagSymbolTable *current_scope;
    JagDiagnosticEngine diag;
    bool programmatic_live_enabled;
} JagSemanticAnalyzer;

void jag_semantic_init(JagSemanticAnalyzer *analyzer, const char *source);
void jag_semantic_cleanup(JagSemanticAnalyzer *analyzer);
bool jag_semantic_analyze(JagSemanticAnalyzer *analyzer, JagASTNode *program);
JagType *jag_semantic_get_expr_type(JagSemanticAnalyzer *analyzer, JagASTNode *expr);

#endif // JAG_SEMANTIC_H
