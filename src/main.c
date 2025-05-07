#include <stdio.h>
#include <stdint.h>

#include "alsa_output.h"
#include "midi.h"
#include "midi-player.h"
#include "kdmapi.h"
#include "arg_parser.h"

int main(int argc, char* argv[]) {
    Options opts;
    if (!parse_args(argc, argv, &opts)) {
        return 1;
    }

    uint16_t time_div = 0;
    int track_count = 0;
    TrackData* tracks = load_midi_file(opts.filename, &time_div, &track_count);
    if (!tracks) {
        fprintf(stderr, "Failed to load MIDI file: %s\n", opts.filename);
        return 1;
    }

    SendDirectDataFunc SendDirectData = NULL;

    if (opts.alsa_port) {
        if (!alsa_initialize(opts.alsa_port)) {
            return 1;
        }
        SendDirectData = alsa_send;
    } else {
        void* midi_lib = initialize_midi(&SendDirectData);
        if (!midi_lib) {
            fprintf(stderr, "Failed to initialize MIDI library\n");
            return 1;
        }

        play_midi(tracks, track_count, time_div, SendDirectData, opts.min_velocity);
        unload_midi(midi_lib);
        return 0;
    }

    printf("mplayer: Playing MIDI file: %s\n", opts.filename);
    play_midi(tracks, track_count, time_div, SendDirectData, opts.min_velocity);
    alsa_shutdown();

    return 0;
}
