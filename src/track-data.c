#include "track-data.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void init_track_data(TrackData* track) {
    track->data = NULL;
    track->long_msg = NULL;
    track->tick = 0;
    track->offset = 0;
    track->length = 0;
    track->message = 0;
    track->temp = 0;
    track->long_msg_len = 0;
    track->long_msg_capacity = 0;
    track->data_capacity = 0;
}

void free_track_data(TrackData* track) {
    if (track->data) free(track->data);
    if (track->long_msg) free(track->long_msg);
    track->data = NULL;
    track->long_msg = NULL;
}

int decode_variable_length(TrackData* track) {
    int result = 0;
    uint8_t byte;
    if (track->offset >= track->length) return 0; // Avoid out-of-bounds
    do {
        byte = track->data[track->offset++];
        result = (result << 7) | (byte & 0x7F);
    } while ((byte & 0x80) && track->offset < track->length); // Prevent overflow
    return result;
}

void update_tick(TrackData* track) {
    track->tick += decode_variable_length(track);
}

void update_command(TrackData* track) {
    if (track->length == 0 || track->data == NULL)
        return;
    const uint8_t temp = track->data[track->offset];
    if (temp >= 0x80) {
        track->offset++;
        track->message = temp;
    }
}

void update_message(TrackData* track) {
    if (track->length == 0 || track->data == NULL) return;

    const uint8_t msg_type = track->message & 0xFF;

    if (msg_type < 0xC0) {
        track->temp = track->data[track->offset] << 8 | track->data[track->offset + 1] << 16;
        track->offset += 2;
    } else if (msg_type < 0xE0) {
        track->temp = track->data[track->offset] << 8;
        track->offset += 1;
    } else if (msg_type < 0xF0) {
        track->temp = track->data[track->offset] << 8 | track->data[track->offset + 1] << 16;
        track->offset += 2;
    } else if (msg_type == 0xFF || msg_type == 0xF0) {
        track->temp = (msg_type == 0xFF) ? track->data[track->offset] << 8 : 0;
        track->offset += 1;
        track->long_msg_len = decode_variable_length(track);

        // Ensure we have enough capacity
        if (track->long_msg_capacity < track->long_msg_len) {
            uint8_t* new_buf = realloc(track->long_msg, track->long_msg_len);
            if (new_buf == NULL) {
                fprintf(stderr, "Memory allocation failed\n");
                exit(1);
            }
            track->long_msg = new_buf;
            track->long_msg_capacity = track->long_msg_len;
        }

        memcpy(track->long_msg, &track->data[track->offset], track->long_msg_len);
        track->offset += track->long_msg_len;
    }

    track->message |= track->temp;
}

void process_meta_event(TrackData* track, double* multiplier, uint64_t* bpm, uint16_t time_div) {
    const uint8_t meta_type = (track->message >> 8) & 0xFF;
    if (meta_type == 0x51) { // Tempo change
        *bpm = (track->long_msg[0] << 16) | (track->long_msg[1] << 8) | track->long_msg[2];
        *multiplier = (double)(*bpm * 10) / (double)time_div;
        *multiplier = (*multiplier < 1.0) ? 1.0 : *multiplier; // Ensure minimum multiplier of 1
    }
    else if (meta_type == 0x2F) { // End of track
        free(track->data);
        track->data = NULL;
        track->length = 0;
    }
}