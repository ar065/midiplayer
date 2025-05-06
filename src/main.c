#include <stdio.h>
#include "midi.h"
#include "midi-player.h"
#include "kdmapi.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: ./midi_player <midi_file>\n", argv[0]);
        return 1;
    }

    const char* filename = argv[1];

    uint16_t time_div = 0;
    int track_count = 0;

    TrackData* tracks = load_midi_file(filename, &time_div, &track_count);
    if (!tracks) {
        printf("Failed to load MIDI file\n");
        return 1;
    }

    SendDirectDataFunc SendDirectData = NULL;
    void* midi_lib = initialize_midi(&SendDirectData);
    if (!midi_lib) {
        printf("Failed to initialize MIDI library\n");
        return 1;
    }

    play_midi(tracks, track_count, time_div, SendDirectData);

    unload_midi(midi_lib);

    return 0;
}