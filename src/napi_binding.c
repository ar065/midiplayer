#include <node_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "midi.h"
#include "midi-player.h"

// Define a structure to wrap our JS callback
typedef struct {
    napi_ref callback_ref;
    napi_env env;
} SendDirectDataContext;

// Global context to maintain the callback reference
static SendDirectDataContext callback_context = {NULL, NULL};

// Forward declaration of the function that will be called from C to JS
void NapiSendDirectData(uint32_t data);

// Method to free TrackData resources
void free_tracks(TrackData* tracks, int track_count) {
    if (tracks) {
        for (int i = 0; i < track_count; i++) {
            free_track_data(&tracks[i]);
        }
        free(tracks);
    }
}

// Method to play a MIDI file
napi_value PlayMIDI(napi_env env, napi_callback_info info) {
    size_t argc = 3; // Expected arguments: file path, callback function, min velocity
    napi_value args[3];
    napi_value this_arg;
    napi_status status;
    
    // Get the arguments passed to the function
    status = napi_get_cb_info(env, info, &argc, args, &this_arg, NULL);
    if (status != napi_ok || argc < 2) {
        napi_throw_error(env, NULL, "Invalid arguments. Expected: (filePath, callback[, minVelocity])");
        return NULL;
    }
    
    // Extract the file path
    char filepath[256];
    size_t filepath_len;
    status = napi_get_value_string_utf8(env, args[0], filepath, sizeof(filepath), &filepath_len);
    if (status != napi_ok) {
        napi_throw_error(env, NULL, "Invalid file path");
        return NULL;
    }
    
    // Extract the callback function
    napi_valuetype valuetype;
    status = napi_typeof(env, args[1], &valuetype);
    if (status != napi_ok || valuetype != napi_function) {
        napi_throw_error(env, NULL, "Invalid callback function");
        return NULL;
    }
    
    // Store the callback reference
    if (callback_context.callback_ref != NULL) {
        napi_delete_reference(env, callback_context.callback_ref);
    }
    status = napi_create_reference(env, args[1], 1, &callback_context.callback_ref);
    callback_context.env = env;
    
    // Extract min_velocity (optional argument)
    uint8_t min_velocity = 0; // Default value
    if (argc >= 3) {
        uint32_t temp_velocity;
        status = napi_get_value_uint32(env, args[2], &temp_velocity);
        if (status != napi_ok) {
            napi_throw_error(env, NULL, "Invalid min_velocity value");
            return NULL;
        }
        // Convert uint32_t to uint8_t, clamping the value to 0-127 for MIDI
        min_velocity = (temp_velocity > 127) ? 127 : (uint8_t)temp_velocity;
    }
    
    // Load the MIDI file
    uint16_t time_div = 0;
    int track_count = 0;
    TrackData* tracks = load_midi_file(filepath, &time_div, &track_count);
    if (!tracks) {
        napi_throw_error(env, NULL, "Failed to load MIDI file");
        return NULL;
    }
    
    // Play the MIDI file
    play_midi(tracks, track_count, time_div, NapiSendDirectData, min_velocity);
    
    // Free the track data after playing
    free_tracks(tracks, track_count);
    
    napi_value result;
    napi_get_undefined(env, &result);
    return result;
}

// Function that will be called from C to JS
void NapiSendDirectData(uint32_t data) {
    if (callback_context.callback_ref == NULL || callback_context.env == NULL) {
        return;
    }
    
    napi_env env = callback_context.env;
    napi_value callback;
    napi_value global;
    napi_value args[1];
    napi_value result;
    
    // Get the global object
    napi_get_global(env, &global);
    
    // Get the JavaScript callback function from the reference
    napi_get_reference_value(env, callback_context.callback_ref, &callback);
    
    // Create the argument to pass to the callback (the data value)
    napi_create_uint32(env, data, &args[0]);
    
    // Call the JavaScript function
    napi_call_function(env, global, callback, 1, args, &result);
}

// Initialize the module
napi_value Init(napi_env env, napi_value exports) {
    napi_status status;
    napi_value play_midi_fn;
    
    // Create the PlayMIDI function
    status = napi_create_function(env, NULL, 0, PlayMIDI, NULL, &play_midi_fn);
    if (status != napi_ok) {
        napi_throw_error(env, NULL, "Failed to create function");
        return NULL;
    }
    
    // Add the function to exports
    status = napi_set_named_property(env, exports, "playMIDI", play_midi_fn);
    if (status != napi_ok) {
        napi_throw_error(env, NULL, "Failed to set exports");
        return NULL;
    }
    
    return exports;
}

// Register the module
NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)