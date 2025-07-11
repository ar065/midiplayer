#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

typedef struct {
    int        fps;
    int        buffer_size;
    uint32_t*  history;
    int        current_frame;
    pthread_mutex_t lock;
} StatsLogger;

// Allocate and initialize a new StatsLogger
static inline StatsLogger* stats_logger_create(int fps) {
    StatsLogger* lg = malloc(sizeof(*lg));
    lg->fps          = fps;
    lg->buffer_size  = fps;
    lg->history      = calloc(fps, sizeof(uint32_t));
    lg->current_frame= 0;
    pthread_mutex_init(&lg->lock, NULL);
    return lg;
}

// Free it
static inline void stats_logger_destroy(StatsLogger* lg) {
    pthread_mutex_destroy(&lg->lock);
    free(lg->history);
    free(lg);
}

// Call this on every note-on
static inline void stats_logger_increment(StatsLogger* lg) {
    pthread_mutex_lock(&lg->lock);
      lg->history[lg->current_frame]++;
    pthread_mutex_unlock(&lg->lock);
}

// Call this once per frame
static inline void stats_logger_next_frame(StatsLogger* lg) {
    pthread_mutex_lock(&lg->lock);
      lg->current_frame = (lg->current_frame + 1) % lg->buffer_size;
      lg->history[lg->current_frame] = 0;
    pthread_mutex_unlock(&lg->lock);
}

// Sum the last one second’s worth of frames
static inline uint32_t stats_logger_get_eps(StatsLogger* lg) {
    uint32_t sum = 0;
    pthread_mutex_lock(&lg->lock);
      for (int i = 0; i < lg->buffer_size; i++)
          sum += lg->history[i];
    pthread_mutex_unlock(&lg->lock);
    return sum;
}

typedef struct {
    StatsLogger* lg;
    volatile bool* is_playing;
} FrameLoggerArgs;