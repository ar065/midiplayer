#include <stdio.h>
#include <alsa/asoundlib.h>
#include "alsa_output.h"

// ALSA handles (file-scoped)
static snd_seq_t* seq_handle = NULL;
static int out_port = -1;

bool alsa_initialize(const char* port_string) {
    if (snd_seq_open(&seq_handle, "default", SND_SEQ_OPEN_OUTPUT, 0) < 0) {
        fprintf(stderr, "Error opening ALSA sequencer.\n");
        return false;
    }

    snd_seq_set_client_name(seq_handle, "MIDI Player ALSA");

    out_port = snd_seq_create_simple_port(seq_handle, "Out",
        SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
        SND_SEQ_PORT_TYPE_APPLICATION);

    if (out_port < 0) {
        fprintf(stderr, "Failed to create ALSA port.\n");
        return false;
    }

    snd_seq_addr_t addr;
    if (snd_seq_parse_address(seq_handle, &addr, port_string) < 0) {
        fprintf(stderr, "Invalid ALSA port address: %s\n", port_string);
        return false;
    }

    if (snd_seq_connect_to(seq_handle, out_port, addr.client, addr.port) < 0) {
        fprintf(stderr, "Failed to connect to ALSA port %s\n", port_string);
        return false;
    }

    return true;
}

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
            return;
    }

    snd_seq_event_output_direct(seq_handle, &ev);
}

void alsa_shutdown(void) {
    if (seq_handle) {
        snd_seq_close(seq_handle);
        seq_handle = NULL;
    }
}
