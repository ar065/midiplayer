#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include <alsa/asoundlib.h>

#include "midi.h"
#include "midi-player.h"
#include "kdmapi.h"

typedef void (*SendDirectDataFunc)(uint32_t);

// ALSA globals
snd_seq_t* seq_handle = NULL;
int out_port = -1;
int client = -1;
int port = -1;

void alsa_send(uint32_t message) {
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);
    snd_seq_ev_set_source(&ev, out_port);
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);

    uint8_t status = message & 0xFF;
    uint8_t data1 = (message >> 8) & 0xFF;
    uint8_t data2 = (message >> 16) & 0xFF;

    uint8_t type = status & 0xF0;
    uint8_t channel = status & 0x0F;

    switch (type) {
        case 0x80: snd_seq_ev_set_noteoff(&ev, channel, data1, data2); break;
        case 0x90: snd_seq_ev_set_noteon(&ev, channel, data1, data2); break;
        case 0xA0: snd_seq_ev_set_keypress(&ev, channel, data1, data2); break;
        case 0xB0: snd_seq_ev_set_controller(&ev, channel, data1, data2); break;
        case 0xC0: snd_seq_ev_set_pgmchange(&ev, channel, data1); break;
        case 0xD0: snd_seq_ev_set_chanpress(&ev, channel, data1); break;
        case 0xE0:
            snd_seq_ev_set_pitchbend(&ev, channel, ((data2 << 7) | data1) - 8192);
            break;
        default:
            return; // Ignore unsupported types
    }

    snd_seq_event_output_direct(seq_handle, &ev);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("mplayer: Usage: ./midi_player <midi_file> [--alsa=client:port]\n");
        return 1;
    }

    const char* filename = argv[1];
    const char* alsa_port_arg = NULL;

    int min_velocity = 1; // default

    for (int i = 2; i < argc; ++i) {
        if (strncmp(argv[i], "--alsa=", 7) == 0) {
            alsa_port_arg = argv[i] + 7;
        } else if (strncmp(argv[i], "--minvel=", 9) == 0) {
            min_velocity = atoi(argv[i] + 9);
            if (min_velocity < 0 || min_velocity > 127) {
                fprintf(stderr, "Invalid minvel value. Must be between 0 and 127.\n");
                return 1;
            }
        }
    }
    
    uint16_t time_div = 0;
    int track_count = 0;

    TrackData* tracks = load_midi_file(filename, &time_div, &track_count);
    if (!tracks) {
        printf("mplayer: Failed to load MIDI file\n");
        return 1;
    }

    SendDirectDataFunc SendDirectData = NULL;

    if (alsa_port_arg) {
        if (snd_seq_open(&seq_handle, "default", SND_SEQ_OPEN_OUTPUT, 0) < 0) {
            fprintf(stderr, "Error opening ALSA sequencer.\n");
            return 1;
        }

        snd_seq_set_client_name(seq_handle, "MIDI Player ALSA");
        out_port = snd_seq_create_simple_port(seq_handle, "Out",
            SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
            SND_SEQ_PORT_TYPE_APPLICATION);

        if (out_port < 0) {
            fprintf(stderr, "Failed to create ALSA port.\n");
            return 1;
        }

        snd_seq_addr_t addr;
        if (snd_seq_parse_address(seq_handle, &addr, alsa_port_arg) < 0) {
            fprintf(stderr, "Invalid ALSA port address: %s\n", alsa_port_arg);
            return 1;
        }

        client = addr.client;
        port = addr.port;

        if (snd_seq_connect_to(seq_handle, out_port, client, port) < 0) {
            fprintf(stderr, "Failed to connect to ALSA port %s\n", alsa_port_arg);
            return 1;
        }

        SendDirectData = alsa_send;
    } else {
        void* midi_lib = initialize_midi(&SendDirectData);
        if (!midi_lib) {
            printf("mplayer: Failed to initialize MIDI library\n");
            return 1;
        }

        play_midi(tracks, track_count, time_div, SendDirectData, min_velocity);

        unload_midi(midi_lib);
        return 0;
    }

    printf("mplayer: Playing MIDI file %s\n", filename);

    // Use ALSA output
    play_midi(tracks, track_count, time_div, SendDirectData, min_velocity);

    if (seq_handle)
        snd_seq_close(seq_handle);

    return 0;
}
