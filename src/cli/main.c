#include "jag/codegen.h"
#include "jag/common.h"
#include "jag/live.h"
#include "jag/parser.h"
#include "jag/semantic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void print_usage(void) {
    printf("Jaguar Compiler %s\n\n", JAG_VERSION);
    printf("Usage:\n");
    printf("  jag <file.jag>               Compile and run Jaguar source file\n");
    printf("  jag -live=1 <file.jag>       Run file in live-reload supervision mode\n");
    printf("  jag --live=1 <file.jag>      Run file in live-reload supervision mode\n");
    printf("  jag run <file.jag>           Compile and run Jaguar source file\n");
    printf("  jag build <file.jag>         Compile source file into standalone native binary\n");
    printf("  jag check <file.jag>         Lex, parse, resolve, and type-check only\n");
    printf("  jag watch <file.jag>         Watch source file and rebuild on change\n");
    printf("  jag --version                Show version information\n");
    printf("  jag --help                   Show this help message\n");
}

static char *read_file_contents(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = malloc(len + 1);
    size_t read_bytes = fread(buf, 1, len, f);
    buf[read_bytes] = '\0';
    fclose(f);
    return buf;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
        printf("Jaguar Compiler %s\n", JAG_VERSION);
        return 0;
    }

    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_usage();
        return 0;
    }

    const char *command = "run";
    const char *filename = NULL;
    bool live_mode = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-live=1") == 0 || strcmp(argv[i], "--live=1") == 0 ||
            strcmp(argv[i], "--live") == 0 || strcmp(argv[i], "-live") == 0) {
            live_mode = true;
        } else if (strcmp(argv[i], "build") == 0 || strcmp(argv[i], "check") == 0 ||
                   strcmp(argv[i], "watch") == 0 || strcmp(argv[i], "run") == 0) {
            command = argv[i];
        } else if (argv[i][0] != '-') {
            filename = argv[i];
        }
    }

    if (strcmp(command, "watch") == 0) {
        live_mode = true;
    }

    if (!filename) {
        fprintf(stderr, "Error: No .jag source file specified.\n");
        return 1;
    }

    char *source = read_file_contents(filename);
    if (!source) {
        fprintf(stderr, "Error: Could not open source file '%s'.\n", filename);
        return 1;
    }

    JagParser parser;
    jag_parser_init(&parser, source, filename);
    JagASTNode *ast = jag_parse_program(&parser);

    if (parser.had_error) {
        fprintf(stderr, "Compilation Error: %s\n", parser.error_msg);
        free(source);
        return 1;
    }

    JagSemanticAnalyzer analyzer;
    jag_semantic_init(&analyzer, source);
    bool sem_ok = jag_semantic_analyze(&analyzer, ast);

    if (!sem_ok) {
        jag_semantic_cleanup(&analyzer);
        jag_ast_free(ast);
        free(source);
        return 1;
    }

    if (analyzer.programmatic_live_enabled) {
        live_mode = true;
    }

    if (strcmp(command, "check") == 0) {
        printf("Type checking succeeded for %s.\n", filename);
        jag_semantic_cleanup(&analyzer);
        jag_ast_free(ast);
        free(source);
        return 0;
    }

    char out_bin[1024];
    snprintf(out_bin, sizeof(out_bin), "/tmp/jag_app_%d", (int)getpid());

    char out_c[1024];
    snprintf(out_c, sizeof(out_c), "/tmp/jag_app_%d.c", (int)getpid());

    JagCodegenOptions opts = { 0 };
    opts.runtime_header_dir = "include";
    opts.runtime_lib_dir = "build";

    bool gen_ok = jag_codegen_generate_c(ast, out_c, &opts);
    if (!gen_ok) {
        fprintf(stderr, "Code generation failed.\n");
        jag_semantic_cleanup(&analyzer);
        jag_ast_free(ast);
        free(source);
        return 1;
    }

    if (strcmp(command, "build") == 0) {
        char target_bin[1024];
        int fname_len = (int)strlen(filename);
        if (fname_len > 4 && strcmp(filename + fname_len - 4, ".jag") == 0) {
            snprintf(target_bin, sizeof(target_bin), "%.*s", fname_len - 4, filename);
        } else {
            snprintf(target_bin, sizeof(target_bin), "%s_bin", filename);
        }

        bool compile_ok = jag_codegen_compile_native(out_c, target_bin, &opts);
        unlink(out_c);
        jag_semantic_cleanup(&analyzer);
        jag_ast_free(ast);
        free(source);

        if (compile_ok) {
            printf("Built target binary: %s\n", target_bin);
            return 0;
        } else {
            fprintf(stderr, "Build failed.\n");
            return 1;
        }
    }

    bool compile_ok = jag_codegen_compile_native(out_c, out_bin, &opts);
    unlink(out_c);

    jag_semantic_cleanup(&analyzer);
    jag_ast_free(ast);
    free(source);

    if (!compile_ok) {
        fprintf(stderr, "Native compilation failed.\n");
        return 1;
    }

    if (live_mode) {
        jag_live_start_supervisor(filename, out_bin);
        unlink(out_bin);
        return 0;
    }

    int run_res = system(out_bin);
    unlink(out_bin);
    return WEXITSTATUS(run_res);
}
