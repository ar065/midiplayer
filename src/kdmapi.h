#ifndef MIDI_LOADER_H
#define MIDI_LOADER_H

#include <stdbool.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Define function pointer types for OmniMIDI/KDMAPI functions
typedef bool (*SendDirectDataFunc)(unsigned int);

// Initializes the MIDI library (OmniMIDI on Windows, libOmniMIDI.so on Linux)
// On success, returns a handle to the loaded library and sets SendDirectData
// On failure, returns NULL
void* initialize_midi(SendDirectDataFunc* SendDirectData);

// Unloads the MIDI library loaded by initialize_midi
void unload_midi(void* midi_lib);

#ifdef __cplusplus
}
#endif

#endif // MIDI_LOADER_H
