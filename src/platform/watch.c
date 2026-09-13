#include "jag/common.h"
#include "jag/live.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#ifdef __linux__
#include <sys/inotify.h>
#endif
#endif

static uint64_t get_file_mtime(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0) {
        return (uint64_t)st.st_mtime;
    }
    return 0;
}

void jag_filewatcher_init(JagFileWatcher *watcher) {
    watcher->files = NULL;
    watcher->count = 0;
    watcher->capacity = 0;
    watcher->inotify_fd = -1;
    watcher->watch_fd = -1;

#if defined(__linux__) && !defined(_WIN32)
    watcher->inotify_fd = inotify_init1(IN_NONBLOCK);
#endif
}

void jag_filewatcher_add_file(JagFileWatcher *watcher, const char *filename) {
    if (!filename) return;
    if (watcher->count >= watcher->capacity) {
        watcher->capacity = watcher->capacity == 0 ? 4 : watcher->capacity * 2;
        watcher->files = realloc(watcher->files, watcher->capacity * sizeof(JagWatchedFile));
    }
    watcher->files[watcher->count].filename = jag_strdup(filename);
    watcher->files[watcher->count].last_mtime = get_file_mtime(filename);

#if defined(__linux__) && !defined(_WIN32)
    if (watcher->inotify_fd >= 0) {
        watcher->watch_fd = inotify_add_watch(watcher->inotify_fd, filename, IN_MODIFY | IN_ATTRIB);
    }
#endif

    watcher->count++;
}

bool jag_filewatcher_poll_changes(JagFileWatcher *watcher, int timeout_ms) {
    bool changed = false;

#if defined(__linux__) && !defined(_WIN32)
    if (watcher->inotify_fd >= 0) {
        char buf[1024];
        ssize_t len = read(watcher->inotify_fd, buf, sizeof(buf));
        if (len > 0) {
            changed = true;
        }
    }
#endif

    for (size_t i = 0; i < watcher->count; i++) {
        uint64_t current_mtime = get_file_mtime(watcher->files[i].filename);
        if (current_mtime > watcher->files[i].last_mtime) {
            watcher->files[i].last_mtime = current_mtime;
            changed = true;
        }
    }

    if (!changed && timeout_ms > 0) {
#ifdef _WIN32
        Sleep((DWORD)timeout_ms);
#else
        struct timespec ts;
        ts.tv_sec = timeout_ms / 1000;
        ts.tv_nsec = (timeout_ms % 1000) * 1000000L;
        nanosleep(&ts, NULL);
#endif
    }

    return changed;
}

void jag_filewatcher_free(JagFileWatcher *watcher) {
    if (!watcher) return;
#if defined(__linux__) && !defined(_WIN32)
    if (watcher->inotify_fd >= 0) {
        close(watcher->inotify_fd);
    }
#endif
    for (size_t i = 0; i < watcher->count; i++) {
        free(watcher->files[i].filename);
    }
    free(watcher->files);
}
