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

static void sleep_ms(int ms) {
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
#endif
}

int main(void) {
    const char *test_file = "test_live_watch_tmp.jag";
    FILE *f = fopen(test_file, "w");
    assert(f != NULL);
    fprintf(f, "live.on(\"v1\");\n");
    fflush(f);
    fclose(f);

    JagFileWatcher watcher;
    jag_filewatcher_init(&watcher);
    jag_filewatcher_add_file(&watcher, test_file);

    bool changed1 = jag_filewatcher_poll_changes(&watcher, 10);
    assert(changed1 == false);

    sleep_ms(150);

    f = fopen(test_file, "w");
    assert(f != NULL);
    fprintf(f, "live.on(\"v2\");\n");
    fflush(f);
    fclose(f);

    bool changed2 = false;
    for (int i = 0; i < 10; i++) {
        if (jag_filewatcher_poll_changes(&watcher, 50)) {
            changed2 = true;
            break;
        }
        sleep_ms(50);
    }
    assert(changed2 == true);

    jag_filewatcher_free(&watcher);
    unlink(test_file);

    printf("Live mode filewatcher tests passed successfully!\n");
    return 0;
}
