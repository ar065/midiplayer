#include "midi-utils.h"
#include "stats_logger.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

int64_t getTime100ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return ((int64_t)ts.tv_sec * 10000000ULL) +
           ((int64_t)ts.tv_nsec / 100ULL);
}

void delayExecution100Ns(const int64_t delayIn100Ns) {
    struct timespec req = {0};
    req.tv_sec = delayIn100Ns / 10000000;
    req.tv_nsec = (delayIn100Ns % 10000000) * 100;
    nanosleep(&req, NULL);
}

uint32_t fntohl(const uint32_t nlong) {
    return ((nlong & 0xFF000000) >> 24) |
           ((nlong & 0x00FF0000) >> 8)  |
           ((nlong & 0x0000FF00) << 8)  |
           ((nlong & 0x000000FF) << 24);
}

uint16_t fntohs(const uint16_t nshort) {
    return ((nshort & 0xFF00) >> 8) |
           ((nshort & 0x00FF) << 8);
}

void* log_notes_per_second(void* _args) {
    FrameLoggerArgs* args = (FrameLoggerArgs*)_args;
    StatsLogger* lg       = args->lg;
    bool* is_playing      = args->is_playing;

    // sleep interval = 1/fps seconds
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = (long)(1e9 / lg->fps);

    while (*is_playing) {
        nanosleep(&ts, NULL);

        // advance window
        stats_logger_next_frame(lg);

        // print the current EPS
        uint32_t eps = stats_logger_get_eps(lg);
        printf("mplayer: Notes per second (sliding): %u\n", eps);
        fflush(stdout);
    }

    return NULL;
}
