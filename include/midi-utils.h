#ifndef MIDI_UTILS_H
#define MIDI_UTILS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Type definition for the OmniMIDI function
typedef void (*SendDirectDataFunc)(uint32_t);

// Endianness conversion functions
uint32_t fntohl(uint32_t nlong);
uint16_t fntohs(uint16_t nshort);

// Timing functions
int64_t getTime100ns();
void delayExecution100Ns(int64_t delayIn100Ns);

// Logger thread function and structure
typedef struct {
    bool* is_playing;
    uint64_t* note_on_count;
} LoggerArgs;

void* log_notes_per_second(void* arg);

#ifdef __cplusplus
}
#endif

#endif // MIDI_UTILS_H