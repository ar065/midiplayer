#include <stdio.h>
#include <pthread.h>

#include "midi-player.h"
#include "stats_logger.h"

void play_midi(TrackData* tracks, int track_count, uint16_t time_div, SendDirectDataFunc SendDirectData, int min_velocity) {
    uint64_t tick = 0;
    double multiplier = 0;
    uint64_t bpm = 500000; // Default tempo: 120 BPM
    uint64_t delta_tick = 0;
    uint64_t last_time = 0;
    const uint64_t max_drift = 100000;
    int64_t delta = 0;
    uint64_t old = 0;
    int64_t temp = 0;

    uint64_t note_on_count = 0;
    bool is_playing = true;

    uint64_t now = getTime100ns();
    last_time = now;

    // Setup and start logger thread
    StatsLogger*   logger       = stats_logger_create(144);
    pthread_t      logger_thread;
    FrameLoggerArgs logger_args = { .lg = logger, .is_playing = &is_playing };

    pthread_create(&logger_thread, NULL, log_notes_per_second, &logger_args);

    bool has_active_tracks = true;

    while (true) {
        // Check if there are any active tracks
        has_active_tracks = false;
        int active_track_count = 0;
        int* active_tracks = malloc(track_count * sizeof(int));

        for (int i = 0; i < track_count; i++) {
            if (tracks[i].data != NULL) {
                if (tracks[i].tick <= tick) {
                    // Process events in this track
                    while (tracks[i].data != NULL && tracks[i].tick <= tick) {
                        update_command(&tracks[i]);
                        update_message(&tracks[i]);

                        uint32_t message = tracks[i].message;
                        uint8_t msg_type = message & 0xFF;
                        if (msg_type < 0xF0) {
                            // Check if it's a note-on message
                            if (msg_type >= 0x90 && msg_type <= 0x9F) {
                                uint8_t velocity = (message >> 16) & 0xFF;
                                // note_on_count++;
                                stats_logger_increment(logger);

                                if (velocity > min_velocity) {
                                    SendDirectData(message);
                                }
                            } else {
                                // Pass through all other message types
                                SendDirectData(message);
                            }
                        }
                        else if (msg_type == 0xFF) {
                            process_meta_event(&tracks[i], &multiplier, &bpm, time_div);
                        }
                        else if (msg_type == 0xF0) {
                            printf("mplayer: TODO: Handle SysEx\n");
                        }

                        if (tracks[i].data != NULL) {
                            update_tick(&tracks[i]);
                        }
                    }
                }

                if (tracks[i].data != NULL) {
                    active_tracks[active_track_count++] = i;
                    has_active_tracks = true;
                }
            }
        }

        if (!has_active_tracks) {
            free(active_tracks);
            break;
        }

        // Find next tick
        delta_tick = UINT64_MAX;
        for (int i = 0; i < active_track_count; i++) {
            int idx = active_tracks[i];
            if (tracks[idx].data != NULL) {
                uint64_t track_delta = tracks[idx].tick - tick;
                if (track_delta < delta_tick) {
                    delta_tick = track_delta;
                }
            }
        }

        free(active_tracks);

        tick += delta_tick;

        now = getTime100ns();
        temp = now - last_time;
        last_time = now;
        temp -= old;
        old = delta_tick * multiplier;
        delta += temp;

        temp = (delta > 0) ? (old - delta) : old;

        if (temp <= 0) {
            delta = (delta < (int64_t)max_drift) ? delta : (int64_t)max_drift;
        } else {
            delayExecution100Ns(temp);
        }
    }

    is_playing = false;
    pthread_join(logger_thread, NULL);
    stats_logger_destroy(logger);
}