#ifndef NAPI_BINDING_H
#define NAPI_BINDING_H

#include <node_api.h>
#include <stdint.h>

// Function declarations
napi_value PlayMIDI(napi_env env, napi_callback_info info);
void NapiSendDirectData(uint32_t data);

napi_value Init(napi_env env, napi_value exports);
void free_tracks(TrackData* tracks, int track_count);

#endif // NAPI_BINDING_H