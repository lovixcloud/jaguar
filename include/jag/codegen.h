#ifndef JAG_CODEGEN_H
#define JAG_CODEGEN_H

#include "jag/ast.h"
#include "jag/common.h"

typedef struct {
    char *output_c_file;
    char *output_bin_file;
    const char *runtime_header_dir;
    const char *runtime_lib_dir;
    bool debug_mode;
} JagCodegenOptions;

bool jag_codegen_generate_c(JagASTNode *ast, const char *output_c_filename, JagCodegenOptions *opts);
bool jag_codegen_compile_native(const char *c_filename, const char *bin_filename, JagCodegenOptions *opts);

#endif // JAG_CODEGEN_H
