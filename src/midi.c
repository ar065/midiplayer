#include "midi.h"
#include <stdio.h>
#include <time.h>
#include <string.h>

TrackData* load_midi_file(const char* filename, uint16_t* time_div, int* track_count) {
    TrackData* tracks = NULL;
    FILE* file = fopen(filename, "rb");

    if (!file) {
        fprintf(stderr, "Could not open file\n");
        return NULL;
    }

    clock_t start_time = clock();

    char header[4];
    fread(header, 1, 4, file);
    if (strncmp(header, "MThd", 4) != 0) {
        fprintf(stderr, "Not a MIDI file\n");
        fclose(file);
        return NULL;
    }

    uint32_t header_length;
    fread(&header_length, 4, 1, file);
    header_length = fntohl(header_length);
    if (header_length != 6) {
        fprintf(stderr, "Invalid header length\n");
        fclose(file);
        return NULL;
    }

    uint16_t format;
    fread(&format, 2, 1, file);
    format = fntohs(format);

    uint16_t num_tracks;
    fread(&num_tracks, 2, 1, file);
    num_tracks = fntohs(num_tracks);

    fread(time_div, 2, 1, file);
    *time_div = fntohs(*time_div);

    if (*time_div >= 0x8000) {
        fprintf(stderr, "SMPTE timing not supported\n");
        fclose(file);
        return NULL;
    }

    printf("mplayer: %d tracks\n", num_tracks);

    tracks = malloc(num_tracks * sizeof(TrackData));
    if (!tracks) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(file);
        return NULL;
    }

    // Initialize tracks
    for (int i = 0; i < num_tracks; i++) {
        init_track_data(&tracks[i]);
    }

    int valid_tracks = 0;
    for (int i = 0; i < num_tracks; ++i) {
        fread(header, 1, 4, file);
        if (strncmp(header, "MTrk", 4) != 0) {
            continue;
        }

        uint32_t length;
        fread(&length, 4, 1, file);
        length = fntohl(length);

        // Allocate memory for track data
        tracks[valid_tracks].data = malloc(length);
        if (!tracks[valid_tracks].data) {
            fprintf(stderr, "Memory allocation failed\n");
            for (int j = 0; j < valid_tracks; j++) {
                free_track_data(&tracks[j]);
            }
            free(tracks);
            fclose(file);
            return NULL;
        }

        // Allocate initial long message buffer
        tracks[valid_tracks].long_msg = malloc(256);
        if (!tracks[valid_tracks].long_msg) {
            fprintf(stderr, "Memory allocation failed\n");
            free(tracks[valid_tracks].data);
            for (int j = 0; j < valid_tracks; j++) {
                free_track_data(&tracks[j]);
            }
            free(tracks);
            fclose(file);
            return NULL;
        }

        tracks[valid_tracks].long_msg_capacity = 256;
        tracks[valid_tracks].length = length;
        tracks[valid_tracks].data_capacity = length;
        tracks[valid_tracks].tick = 0;
        tracks[valid_tracks].offset = 0;
        tracks[valid_tracks].message = 0;
        tracks[valid_tracks].temp = 0;

        fread(tracks[valid_tracks].data, 1, length, file);
        update_tick(&tracks[valid_tracks]);
        valid_tracks++;
    }

    *track_count = valid_tracks;

    clock_t end_time = clock();
    double duration_seconds = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    long duration_milliseconds = (long)(duration_seconds * 1000);

    printf("mplayer: Parsed in %ldms.\n", duration_milliseconds);

    fclose(file);
    return tracks;
}
