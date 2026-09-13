#include "jag/codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool jag_codegen_compile_native(const char *c_filename, const char *bin_filename, JagCodegenOptions *opts) {
    char cmd[1024];
    const char *header_dir = (opts && opts->runtime_header_dir) ? opts->runtime_header_dir : "include";
    const char *lib_dir = (opts && opts->runtime_lib_dir) ? opts->runtime_lib_dir : "build";

    snprintf(cmd, sizeof(cmd),
             "gcc -O2 -std=c17 -I%s -I. -L%s %s -ljaguar_runtime -o %s > /dev/null 2>&1 || "
             "clang -O2 -std=c17 -I%s -I. -L%s %s -ljaguar_runtime -o %s > /dev/null 2>&1",
             header_dir, lib_dir, c_filename, bin_filename,
             header_dir, lib_dir, c_filename, bin_filename);

    int res = system(cmd);
    return (res == 0);
}
