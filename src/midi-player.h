#ifndef MIDI_PLAYER_H
#define MIDI_PLAYER_H

#include "track-data.h"
#include "midi-utils.h"

#ifdef __cplusplus
extern "C" {
#endif

// Function declarations
TrackData* load_midi_file(const char* filename, uint16_t* time_div, int* track_count);
void play_midi(TrackData* tracks, int track_count, uint16_t time_div, SendDirectDataFunc SendDirectData);

#ifdef __cplusplus
}
#endif

#endif // MIDI_PLAYER_H