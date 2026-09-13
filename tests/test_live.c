#include "jag/live.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#define unlink _unlink
#else
#include <unistd.h>
#endif

int main(void) {
    const char *test_file = "test_live_watch_tmp.jag";
    FILE *f = fopen(test_file, "w");
    assert(f != NULL);
    fprintf(f, "live.on(\"v1\");\n");
    fclose(f);

    JagFileWatcher watcher;
    jag_filewatcher_init(&watcher);
    jag_filewatcher_add_file(&watcher, test_file);

    bool changed1 = jag_filewatcher_poll_changes(&watcher, 10);
    assert(changed1 == false);

#ifdef _WIN32
    Sleep(100);
#else
    struct timespec ts = { 0, 100000000L };
    nanosleep(&ts, NULL);
#endif

    f = fopen(test_file, "w");
    assert(f != NULL);
    fprintf(f, "live.on(\"v2\");\n");
    fclose(f);

    bool changed2 = jag_filewatcher_poll_changes(&watcher, 50);
    assert(changed2 == true);

    jag_filewatcher_free(&watcher);
    unlink(test_file);

    printf("Live mode filewatcher tests passed successfully!\n");
    return 0;
}
