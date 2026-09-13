#include "jag/diagnostics.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void jag_diag_init(JagDiagnosticEngine *engine, const char *source) {
    engine->error_count = 0;
    engine->source = source;
}

static void print_source_context(const char *source, int line, int col) {
    if (!source) return;
    int curr_line = 1;
    const char *line_start = source;
    const char *p = source;

    while (*p != '\0') {
        if (curr_line == line) {
            line_start = p;
            while (*p != '\n' && *p != '\0') p++;
            int line_len = (int)(p - line_start);
            fprintf(stderr, "\n    %.*s\n    ", line_len, line_start);
            for (int i = 1; i < col; i++) {
                fprintf(stderr, " ");
            }
            fprintf(stderr, "^\n");
            return;
        }
        if (*p == '\n') {
            curr_line++;
        }
        p++;
    }
}

void jag_diag_report(JagDiagnosticEngine *engine, JagSourceLoc loc, const char *code, const char *fmt, ...) {
    engine->error_count++;
    fprintf(stderr, "%s:%d:%d: error %s\n", loc.file ? loc.file : "<unknown>", loc.line, loc.column, code ? code : "JAG-ERR");

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    print_source_context(engine->source, loc.line, loc.column);
    fprintf(stderr, "\n");
}
