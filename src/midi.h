#include "track-data.h"
#include "midi-utils.h"

TrackData* load_midi_file(const char* filename, uint16_t* time_div, int* track_count);