#ifndef JAG_LIVE_H
#define JAG_LIVE_H

#include "jag/common.h"
#include <sys/types.h>

typedef struct {
    char *filename;
    uint64_t last_mtime;
} JagWatchedFile;

typedef struct {
    JagWatchedFile *files;
    size_t count;
    size_t capacity;
    int inotify_fd;
    int watch_fd;
} JagFileWatcher;

void jag_filewatcher_init(JagFileWatcher *watcher);
void jag_filewatcher_add_file(JagFileWatcher *watcher, const char *filename);
bool jag_filewatcher_poll_changes(JagFileWatcher *watcher, int timeout_ms);
void jag_filewatcher_free(JagFileWatcher *watcher);

typedef struct {
    char *source_filename;
    char *bin_filename;
    pid_t child_pid;
    JagFileWatcher watcher;
    bool active;
} JagLiveSupervisor;

void jag_live_supervisor_init(JagLiveSupervisor *sup, const char *source_filename, const char *bin_filename);
void jag_live_supervisor_run(JagLiveSupervisor *sup);
void jag_live_supervisor_cleanup(JagLiveSupervisor *sup);
void jag_live_start_supervisor(const char *source_filename, const char *bin_filename);

#endif // JAG_LIVE_H
