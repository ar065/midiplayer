#include <node_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>

#include "midi.h"
#include "midi-player.h"

#ifdef ENABLE_MIDI_DEBUG
  #define LOG(...) LOG(stderr, __VA_ARGS__)
#else
  #define LOG(...) (void)0
#endif

// —————————————————————————————————————————————————————————————————
// Globals
// —————————————————————————————————————————————————————————————————

// Our single threadsafe function handle
static napi_threadsafe_function g_tsfn = NULL;

// Worker arguments
typedef struct
{
    TrackData *tracks;
    int track_count;
    uint16_t time_div;
    uint8_t min_velocity;
} WorkerArgs;

// —————————————————————————————————————————————————————————————————
// Utility: free up TrackData[]
// —————————————————————————————————————————————————————————————————

static void free_tracks(TrackData *tracks, int track_count)
{
    if (!tracks)
        return;
    for (int i = 0; i < track_count; i++)
    {
        free_track_data(&tracks[i]);
    }
    free(tracks);
}

// —————————————————————————————————————————————————————————————————
// This will run ON the JS thread when an event is dequeued.
// —————————————————————————————————————————————————————————————————
static void CallJs(napi_env env,
                   napi_value js_cb,
                   void * /*context*/,
                   void *data)
{
    if (!env || !js_cb)
    {
        LOG(stderr, "[CallJs] invalid env or js_cb\n");
        return;
    }
    if (!data)
    {
        LOG(stderr, "[CallJs] no data to send\n");
        return;
    }

    uint32_t value = *(uint32_t *)data;
    free(data);

    LOG(stderr, "[CallJs] invoking JS callback with %u\n", value);

    // 1) Get the global object to use as 'this'
    napi_value global;
    napi_status st = napi_get_global(env, &global);
    if (st != napi_ok)
    {
        LOG(stderr, "[CallJs] napi_get_global failed\n");
        return;
    }

    // 2) Prepare the argument
    napi_value argv;
    st = napi_create_uint32(env, value, &argv);
    if (st != napi_ok)
    {
        LOG(stderr, "[CallJs] napi_create_uint32 failed\n");
        return;
    }

    // 3) Call the JS function with 'global' as the receiver
    st = napi_call_function(env, global, js_cb, 1, &argv, NULL);
    if (st != napi_ok)
    {
        LOG(stderr, "[CallJs] napi_call_function failed with code %d\n", st);
    }
}

// —————————————————————————————————————————————————————————————————
// This is passed into play_midi.  It runs on the MIDI thread.
// —————————————————————————————————————————————————————————————————

void ThreadSendDirectData(uint32_t data)
{
    if (!g_tsfn)
    {
        LOG(stderr, "[ThreadSend] no TSFN (was it created?)\n");
        return;
    }

    LOG(stderr, "[ThreadSend] queueing %u\n", data);

    uint32_t *boxed = malloc(sizeof(uint32_t));
    if (!boxed)
    {
        LOG(stderr, "[ThreadSend] malloc failed\n");
        return;
    }
    *boxed = data;

    napi_status st = napi_call_threadsafe_function(
        g_tsfn,
        boxed,
        napi_tsfn_nonblocking);
    if (st != napi_ok)
    {
        LOG(stderr, "[ThreadSend] napi_call_threadsafe_function failed\n");
        free(boxed);
    }
}

// —————————————————————————————————————————————————————————————————
// Worker entry point: calls your blocking play_midi.
// —————————————————————————————————————————————————————————————————

void *PlayMidiWorker(void *arg)
{
    WorkerArgs *w = (WorkerArgs *)arg;

    LOG(stderr, "[Worker] starting play_midi()\n");
    play_midi(
        w->tracks,
        w->track_count,
        w->time_div,
        ThreadSendDirectData,
        w->min_velocity);
    LOG(stderr, "[Worker] play_midi() returned\n");

    free_tracks(w->tracks, w->track_count);
    free(w);

    // let JS side know we're done with the TSFN
    napi_status st = napi_release_threadsafe_function(
        g_tsfn,
        napi_tsfn_release);
    if (st != napi_ok)
    {
        LOG(stderr, "[Worker] napi_release_threadsafe_function failed\n");
    }
    else
    {
        LOG(stderr, "[Worker] TSFN released\n");
    }
    g_tsfn = NULL;
    return NULL;
}

// —————————————————————————————————————————————————————————————————
// N‑API binding for playMIDI(filePath, callback, [minVelocity])
// —————————————————————————————————————————————————————————————————

napi_value PlayMIDI(napi_env env, napi_callback_info info)
{
    size_t argc = 3;
    napi_value argv[3];
    napi_status st;

    st = napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
    if (st != napi_ok || argc < 2)
    {
        napi_throw_error(env, NULL,
                         "Expected (filePath: string, callback: function, [minVelocity: number])");
        return NULL;
    }

    // — extract file path
    char filepath[512];
    size_t path_len;
    st = napi_get_value_string_utf8(env, argv[0],
                                    filepath, sizeof(filepath),
                                    &path_len);
    if (st != napi_ok)
    {
        napi_throw_error(env, NULL, "Invalid filePath");
        return NULL;
    }

    // — extract min velocity
    uint8_t min_velocity = 0;
    if (argc >= 3)
    {
        uint32_t tmp;
        st = napi_get_value_uint32(env, argv[2], &tmp);
        if (st == napi_ok)
        {
            min_velocity = tmp > 127 ? 127 : (uint8_t)tmp;
        }
    }

    // — create TSFN
    napi_value resource_name;
    st = napi_create_string_utf8(env, "midi_callback",
                                 NAPI_AUTO_LENGTH,
                                 &resource_name);
    if (st != napi_ok)
    {
        napi_throw_error(env, NULL, "Failed to create resource name");
        return NULL;
    }

    st = napi_create_threadsafe_function(
        env,
        argv[1],       // JS callback
        NULL,          // async_resource
        resource_name, // resource name
        0,             // max queue size (0 = unlimited)
        1,             // initial thread count
        NULL,          // thread finalize data
        NULL,          // thread finalize cb
        NULL,          // context for CallJs
        CallJs,        // call_js_cb
        &g_tsfn);
    if (st != napi_ok)
    {
        napi_throw_error(env, NULL, "Failed to create TSFN");
        return NULL;
    }

    // MUST acquire it before using
    st = napi_acquire_threadsafe_function(g_tsfn);
    if (st != napi_ok)
    {
        napi_throw_error(env, NULL, "Failed to acquire TSFN");
        return NULL;
    }

    // — load the MIDI file
    uint16_t time_div;
    int track_count;
    TrackData *tracks = load_midi_file(filepath, &time_div, &track_count);
    if (!tracks)
    {
        napi_throw_error(env, NULL, "Failed to load MIDI file");
        napi_release_threadsafe_function(g_tsfn, napi_tsfn_release);
        g_tsfn = NULL;
        return NULL;
    }

    // — spawn the worker
    WorkerArgs *w = malloc(sizeof(*w));
    w->tracks = tracks;
    w->track_count = track_count;
    w->time_div = time_div;
    w->min_velocity = min_velocity;

    pthread_t tid;
    if (pthread_create(&tid, NULL, PlayMidiWorker, w) != 0)
    {
        napi_throw_error(env, NULL, "Failed to create worker thread");
        free_tracks(tracks, track_count);
        free(w);
        napi_release_threadsafe_function(g_tsfn, napi_tsfn_release);
        g_tsfn = NULL;
        return NULL;
    }
    pthread_detach(tid);

    // return undefined
    napi_value undef;
    napi_get_undefined(env, &undef);
    return undef;
}

// —————————————————————————————————————————————————————————————————
// Module init
// —————————————————————————————————————————————————————————————————

napi_value Init(napi_env env, napi_value exports)
{
    napi_value fn;
    napi_create_function(env, NULL, 0, PlayMIDI, NULL, &fn);
    napi_set_named_property(env, exports, "playMIDI", fn);
    return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
