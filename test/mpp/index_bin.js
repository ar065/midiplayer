import { configDotenv } from "dotenv";
configDotenv();

import { Client } from "mpp-client-net";
import NoteHandler from "./NoteHandler.js";

const client = new Client("wss://mppclone.com", process.env.MPPNET_TOKEN);
client.start();
client.setChannel('✧𝓓𝓔𝓥 𝓡𝓸𝓸𝓶✧');
// client.setChannel("Room369758172347");

import { createRequire } from "module";
const require = createRequire(import.meta.url);
const midiPlayer = require("../../build/linux/x86_64/release/midi_player.node");

const file = '/run/media/ar06/74EAEFC8EAEF8528/Midis/(ATLAS) midis2/[Tikronix] Kirby Super Star - Gourmet Race 140k.mid';
const minimumVelocity = 1; // Minimum velocity (0-127)
// const minimumVelocity = 100; // Minimum velocity (0-127)

function flush() {
    if (!client.isConnected()) return;
    // const msg = noteHandler.flush(client.serverTimeOffset);
    const msg = noteHandler.flush(5000);
    if (msg) client.ws.send(msg);
}

const noteHandler = new NoteHandler();

const flushInterval = setInterval(() => {
    flush();
}, 200);

let MIDI_TRANSPOSE = -12;
let MIDI_KEY_NAMES = ["a-1", "as-1", "b-1"];
let bare_notes = "c cs d ds e f fs g gs a as b".split(" ");
for (let oct = 0; oct < 7; oct++) {
    for (let i in bare_notes) {
        MIDI_KEY_NAMES.push(bare_notes[i] + oct);
    }
}
MIDI_KEY_NAMES.push("c7");

const NOTE_ON = 0x90;
const NOTE_OFF = 0x80;

function midiDataCallback(data) {
    const status = data & 0xFF;
    const noteNumber = (data >> 8) & 0xFF;
    const velocity = (data >> 16) & 0xFF;

    const command = status & 0xF0;
    const channel = status & 0x0F;

    if (command === NOTE_ON) {
        if (velocity === 0) {
            noteHandler.stopNote(noteNumber - 9 + MIDI_TRANSPOSE, flush);
        } else {
            noteHandler.startNote(noteNumber - 9 + MIDI_TRANSPOSE, velocity / 127, flush);
        }
    } else if (command === NOTE_OFF) {
        noteHandler.stopNote(noteNumber - 9 + MIDI_TRANSPOSE, flush);
    }
}

// Play a MIDI file
midiPlayer.playMIDI(file, midiDataCallback, minimumVelocity); 