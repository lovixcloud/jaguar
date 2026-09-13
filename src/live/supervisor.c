#include "jag/codegen.h"
#include "jag/live.h"
#include "jag/parser.h"
#include "jag/semantic.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

void jag_live_supervisor_init(JagLiveSupervisor *sup, const char *source_filename, const char *bin_filename) {
    sup->source_filename = jag_strdup(source_filename);
    sup->bin_filename = jag_strdup(bin_filename);
    sup->child_pid = -1;
    sup->active = true;

    jag_filewatcher_init(&sup->watcher);
    jag_filewatcher_add_file(&sup->watcher, source_filename);
}

static pid_t spawn_child(const char *bin_filename) {
    pid_t pid = fork();
    if (pid == 0) {
        execl(bin_filename, bin_filename, (char *)NULL);
        exit(1);
    }
    return pid;
}

static bool recompile_source(const char *source_filename, const char *bin_filename) {
    char *source = NULL;
    FILE *f = fopen(source_filename, "rb");
    if (!f) return false;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    source = malloc(len + 1);
    size_t read_bytes = fread(source, 1, len, f);
    source[read_bytes] = '\0';
    fclose(f);

    JagParser parser;
    jag_parser_init(&parser, source, source_filename);
    JagASTNode *ast = jag_parse_program(&parser);

    if (parser.had_error) {
        fprintf(stderr, "\n[jaguar:live] Compilation Error: %s\n", parser.error_msg);
        fprintf(stderr, "[jaguar:live] Preserving currently running process...\n\n");
        free(source);
        return false;
    }

    JagSemanticAnalyzer analyzer;
    jag_semantic_init(&analyzer, source);
    bool sem_ok = jag_semantic_analyze(&analyzer, ast);

    if (!sem_ok) {
        fprintf(stderr, "\n[jaguar:live] Type Check / Semantic Error detected!\n");
        fprintf(stderr, "[jaguar:live] Preserving currently running process...\n\n");
        jag_semantic_cleanup(&analyzer);
        jag_ast_free(ast);
        free(source);
        return false;
    }

    JagCodegenOptions opts = { 0 };
    opts.runtime_header_dir = "include";
    opts.runtime_lib_dir = "build";

    char tmp_c[512];
    snprintf(tmp_c, sizeof(tmp_c), "%s.c", bin_filename);

    bool gen_ok = jag_codegen_generate_c(ast, tmp_c, &opts);
    if (!gen_ok) {
        fprintf(stderr, "[jaguar:live] C Code Generation Error!\n");
        jag_semantic_cleanup(&analyzer);
        jag_ast_free(ast);
        free(source);
        return false;
    }

    bool compile_ok = jag_codegen_compile_native(tmp_c, bin_filename, &opts);
    unlink(tmp_c);

    jag_semantic_cleanup(&analyzer);
    jag_ast_free(ast);
    free(source);

    if (!compile_ok) {
        fprintf(stderr, "[jaguar:live] Native Compiler Linker Error!\n");
        return false;
    }

    return true;
}

void jag_live_supervisor_run(JagLiveSupervisor *sup) {
    printf("Jaguar live mode enabled.\n");
    printf("Watching project file: %s...\n", sup->source_filename);

    if (recompile_source(sup->source_filename, sup->bin_filename)) {
        sup->child_pid = spawn_child(sup->bin_filename);
    }

    while (sup->active) {
        if (jag_filewatcher_poll_changes(&sup->watcher, 200)) {
            printf("\n[jaguar:live] File modification detected in %s. Rebuilding...\n", sup->source_filename);

            if (recompile_source(sup->source_filename, sup->bin_filename)) {
                if (sup->child_pid > 0) {
                    kill(sup->child_pid, SIGTERM);
                    int status;
                    waitpid(sup->child_pid, &status, 0);
                }
                printf("[jaguar:live] Rebuild succeeded! Restarting application...\n\n");
                sup->child_pid = spawn_child(sup->bin_filename);
            }
        } else {
            if (sup->child_pid > 0) {
                int status;
                pid_t res = waitpid(sup->child_pid, &status, WNOHANG);
                if (res > 0) {
                    sup->child_pid = -1;
                }
            }
        }
    }
}

void jag_live_supervisor_cleanup(JagLiveSupervisor *sup) {
    if (sup->child_pid > 0) {
        kill(sup->child_pid, SIGTERM);
        int status;
        waitpid(sup->child_pid, &status, 0);
    }
    jag_filewatcher_free(&sup->watcher);
    free(sup->source_filename);
    free(sup->bin_filename);
}
