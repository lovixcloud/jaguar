#include "jag/codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool run_cmd(const char *cmd) {
    int res = system(cmd);
    return (res == 0);
}

bool jag_codegen_compile_native(const char *c_filename, const char *bin_filename, JagCodegenOptions *opts) {
    char cmd[2048];
    const char *header_dir = (opts && opts->runtime_header_dir) ? opts->runtime_header_dir : "include";
    const char *lib_dir = (opts && opts->runtime_lib_dir) ? opts->runtime_lib_dir : "build";
    const char *env_cc = getenv("JAG_CC");

    if (env_cc && strlen(env_cc) > 0) {
        snprintf(cmd, sizeof(cmd), "%s -I\"%s\" -I. -L\"%s\" -L\"%s/Debug\" -L\"%s/Release\" \"%s\" -ljaguar_runtime -o \"%s\"",
                 env_cc, header_dir, lib_dir, lib_dir, lib_dir, c_filename, bin_filename);
        if (run_cmd(cmd)) return true;
    }

#ifdef _WIN32
    // Try CL with libraries in build, build/Debug, build/Release, or current dir
    snprintf(cmd, sizeof(cmd), "cl /nologo /O2 /std:c17 /I\"%s\" /I. \"%s\" \"%s/jaguar_runtime.lib\" ws2_32.lib /Fe:\"%s\" > NUL 2>&1",
             header_dir, c_filename, lib_dir, bin_filename);
    if (run_cmd(cmd)) return true;

    snprintf(cmd, sizeof(cmd), "cl /nologo /O2 /std:c17 /I\"%s\" /I. \"%s\" \"%s/Debug/jaguar_runtime.lib\" ws2_32.lib /Fe:\"%s\" > NUL 2>&1",
             header_dir, c_filename, lib_dir, bin_filename);
    if (run_cmd(cmd)) return true;

    snprintf(cmd, sizeof(cmd), "cl /nologo /O2 /std:c17 /I\"%s\" /I. \"%s\" \"%s/Release/jaguar_runtime.lib\" ws2_32.lib /Fe:\"%s\" > NUL 2>&1",
             header_dir, c_filename, lib_dir, bin_filename);
    if (run_cmd(cmd)) return true;

    snprintf(cmd, sizeof(cmd), "cl /nologo /O2 /std:c17 /I\"%s\" /I. \"%s\" jaguar_runtime.lib ws2_32.lib /Fe:\"%s\" > NUL 2>&1",
             header_dir, c_filename, bin_filename);
    if (run_cmd(cmd)) return true;

    // Try GCC with libraries
    snprintf(cmd, sizeof(cmd), "gcc -O2 -std=c17 -I\"%s\" -I. -L\"%s\" -L\"%s/Debug\" -L\"%s/Release\" -L. \"%s\" -ljaguar_runtime -lws2_32 -o \"%s\" > NUL 2>&1",
             header_dir, lib_dir, lib_dir, lib_dir, c_filename, bin_filename);
    if (run_cmd(cmd)) return true;

    // Try Clang with libraries
    snprintf(cmd, sizeof(cmd), "clang -O2 -std=c17 -I\"%s\" -I. -L\"%s\" -L\"%s/Debug\" -L\"%s/Release\" -L. \"%s\" -ljaguar_runtime -lws2_32 -o \"%s\" > NUL 2>&1",
             header_dir, lib_dir, lib_dir, lib_dir, c_filename, bin_filename);
    if (run_cmd(cmd)) return true;
#else
    snprintf(cmd, sizeof(cmd), "gcc -O2 -std=c17 -I\"%s\" -I. -L\"%s\" \"%s\" -ljaguar_runtime -o \"%s\" > /dev/null 2>&1",
             header_dir, lib_dir, c_filename, bin_filename);
    if (run_cmd(cmd)) return true;

    snprintf(cmd, sizeof(cmd), "clang -O2 -std=c17 -I\"%s\" -I. -L\"%s\" \"%s\" -ljaguar_runtime -o \"%s\" > /dev/null 2>&1",
             header_dir, lib_dir, c_filename, bin_filename);
    if (run_cmd(cmd)) return true;

    snprintf(cmd, sizeof(cmd), "cc -O2 -std=c17 -I\"%s\" -I. -L\"%s\" \"%s\" -ljaguar_runtime -o \"%s\" > /dev/null 2>&1",
             header_dir, lib_dir, c_filename, bin_filename);
    if (run_cmd(cmd)) return true;
#endif

    return false;
}
