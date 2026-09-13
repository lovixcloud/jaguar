#ifndef JAG_DIAGNOSTICS_H
#define JAG_DIAGNOSTICS_H

#include "jag/common.h"

typedef struct {
    int error_count;
    const char *source;
} JagDiagnosticEngine;

void jag_diag_init(JagDiagnosticEngine *engine, const char *source);
void jag_diag_report(JagDiagnosticEngine *engine, JagSourceLoc loc, const char *code, const char *fmt, ...);

#endif // JAG_DIAGNOSTICS_H
