#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include "alsa_output.h"

#include "midi.h"
#include "midi-player.h"
#include "kdmapi.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("mplayer: Usage: ./midi_player <midi_file> [--alsa=client:port]\n");
        return 1;
    }

    const char* filename = argv[1];
    const char* alsa_port_arg = NULL;

    int min_velocity = 1; // default

    for (int i = 2; i < argc; ++i) {
        if (strncmp(argv[i], "--alsa=", 7) == 0) {
            alsa_port_arg = argv[i] + 7;
        } else if (strncmp(argv[i], "--minvel=", 9) == 0) {
            min_velocity = atoi(argv[i] + 9);
            if (min_velocity < 0 || min_velocity > 127) {
                fprintf(stderr, "Invalid minvel value. Must be between 0 and 127.\n");
                return 1;
            }
        }
    }
    
    uint16_t time_div = 0;
    int track_count = 0;

    TrackData* tracks = load_midi_file(filename, &time_div, &track_count);
    if (!tracks) {
        printf("mplayer: Failed to load MIDI file\n");
        return 1;
    }

    SendDirectDataFunc SendDirectData = NULL;

    if (alsa_port_arg) {
        if (!alsa_initialize(alsa_port_arg)) {
            return 1;
        }
        SendDirectData = alsa_send;
    } else {
        void* midi_lib = initialize_midi(&SendDirectData);
        if (!midi_lib) {
            printf("mplayer: Failed to initialize MIDI library\n");
            return 1;
        }

        play_midi(tracks, track_count, time_div, SendDirectData, min_velocity);

        unload_midi(midi_lib);
        return 0;
    }

    printf("mplayer: Playing MIDI file %s\n", filename);

    // Use ALSA output
    play_midi(tracks, track_count, time_div, SendDirectData, min_velocity);

    // This function checks if seq_handle exists or not, so we don't need to check it here
    alsa_shutdown();

    return 0;
}
