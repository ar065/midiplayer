#ifndef TRACK_DATA_H
#define TRACK_DATA_H

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t* data;
    uint8_t* long_msg;
    int tick;
    size_t offset, length;
    uint32_t message, temp;
    size_t long_msg_len;
    size_t long_msg_capacity;
    size_t data_capacity;
} TrackData;

void init_track_data(TrackData* track);
void free_track_data(TrackData* track);

void update_tick(TrackData* track);
void update_command(TrackData* track);
void update_message(TrackData* track);
void process_meta_event(TrackData* track, double* multiplier, uint64_t* bpm, uint16_t time_div);

int decode_variable_length(TrackData* track);

#ifdef __cplusplus
}
#endif

#endif // TRACK_DATA_H