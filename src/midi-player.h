#ifndef MIDI_PLAYER_H
#define MIDI_PLAYER_H

#include "track-data.h"
#include "midi-utils.h"

#ifdef __cplusplus
extern "C" {
#endif

void play_midi(TrackData* tracks, int track_count, uint16_t time_div, SendDirectDataFunc SendDirectData);

#ifdef __cplusplus
}
#endif

#endif // MIDI_PLAYER_H