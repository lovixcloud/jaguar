#include "jag/common.h"
#include "jag/runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif

JagFile *jag_file_open(const char *path, const char *mode) {
    if (!path) return NULL;
    FILE *fp = fopen(path, mode ? mode : "r");
    if (!fp) return NULL;
    JagFile *file = jag_alloc(sizeof(JagFile));
    file->fp = fp;
    file->path = jag_strdup(path);
    return file;
}

char *jag_file_read(JagFile *file) {
    if (!file || !file->fp) return jag_strdup("");
    fseek(file->fp, 0, SEEK_END);
    long len = ftell(file->fp);
    fseek(file->fp, 0, SEEK_SET);

    if (len <= 0) return jag_strdup("");

    char *buf = jag_alloc(len + 1);
    size_t read_bytes = fread(buf, 1, len, file->fp);
    buf[read_bytes] = '\0';
    return buf;
}

void jag_file_close(JagFile *file) {
    if (!file) return;
    if (file->fp) fclose(file->fp);
    if (file->path) free(file->path);
    jag_free(file);
}

#ifdef _WIN32
typedef struct {
    HANDLE hFind;
    WIN32_FIND_DATAA findData;
    bool first;
} WinDir;
#endif

JagDir *jag_dir_on(const char *path) {
    if (!path) return NULL;
#ifdef _WIN32
    char search_path[MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%s\\*", path);
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(search_path, &findData);
    if (hFind == INVALID_HANDLE_VALUE) return NULL;

    WinDir *wd = jag_alloc(sizeof(WinDir));
    wd->hFind = hFind;
    wd->findData = findData;
    wd->first = true;

    JagDir *dir = jag_alloc(sizeof(JagDir));
    dir->path = jag_strdup(path);
    dir->dir_handle = wd;
    return dir;
#else
    DIR *d = opendir(path);
    if (!d) return NULL;
    JagDir *dir = jag_alloc(sizeof(JagDir));
    dir->path = jag_strdup(path);
    dir->dir_handle = d;
    return dir;
#endif
}

JagArray *jag_dir_read(JagDir *dir) {
    if (!dir || !dir->dir_handle) return jag_array_create(0);
    JagArray *arr = jag_array_create(8);

#ifdef _WIN32
    WinDir *wd = (WinDir *)dir->dir_handle;
    if (wd->first) {
        jag_array_append(arr, jag_val_string(wd->findData.cFileName));
        wd->first = false;
    }
    while (FindNextFileA(wd->hFind, &wd->findData)) {
        jag_array_append(arr, jag_val_string(wd->findData.cFileName));
    }
#else
    struct dirent *entry;
    DIR *d = (DIR *)dir->dir_handle;
    while ((entry = readdir(d)) != NULL) {
        jag_array_append(arr, jag_val_string(entry->d_name));
    }
#endif
    return arr;
}

void jag_dir_close(JagDir *dir) {
    if (!dir) return;
    if (dir->dir_handle) {
#ifdef _WIN32
        WinDir *wd = (WinDir *)dir->dir_handle;
        FindClose(wd->hFind);
        jag_free(wd);
#else
        closedir((DIR *)dir->dir_handle);
#endif
    }
    if (dir->path) free(dir->path);
    jag_free(dir);
}

void jag_dir_del(const char *path) {
    if (path) {
#ifdef _WIN32
        DeleteFileA(path);
#else
        unlink(path);
#endif
    }
}
