#include "jag/live.h"
#include <stdio.h>

void jag_live_start_supervisor(const char *source_filename, const char *bin_filename) {
    JagLiveSupervisor sup;
    jag_live_supervisor_init(&sup, source_filename, bin_filename);
    jag_live_supervisor_run(&sup);
    jag_live_supervisor_cleanup(&sup);
}
