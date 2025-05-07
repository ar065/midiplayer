import { configDotenv } from "dotenv";
configDotenv();

import { Client } from "mpp-client-net";

// Instantiate a new client
const client = new Client("wss://mppclone.com", process.env.MPPNET_TOKEN);

// Connect to the server
client.start();
client.setChannel('✧𝓓𝓔𝓥 𝓡𝓸𝓸𝓶✧');
// client.setChannel("Room999156882061");

// Listen for chat messages
client.on('a', msg => {
    if (msg.a == "!ping") {
        // Send a chat message back
        client.sendArray([{
            m: "a",
            message: "Pong!"
        }]);
    }
});

// import { createRequire } from "module";
// const require = createRequire(import.meta.url);
// const midiPlayer = require("../../build/linux/x86_64/release/midi_player.node");

const file = '/run/media/ar06/74EAEFC8EAEF8528/Midis/AEIOU midis/midis/10nqf - tau2.5.9.mid';
const minimumVelocity = 1; // Minimum velocity (0-127)

import { Worker } from 'worker_threads';

const worker = new Worker('./midiWorker.js', {
    workerData: {
        file,
        minimumVelocity
    }
});

worker.on('message', msg => {
    const data = new Uint32Array(msg)[0];
    midiDataCallback(data);
});


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
            client.stopNote(MIDI_KEY_NAMES[noteNumber - 9 + MIDI_TRANSPOSE]);
        } else {
            client.startNote(
                MIDI_KEY_NAMES[noteNumber - 9 + MIDI_TRANSPOSE],
                velocity / 127
            );
        }
    } else if (command === NOTE_OFF) {
        client.stopNote(MIDI_KEY_NAMES[noteNumber - 9 + MIDI_TRANSPOSE]);
    }
}

// Play a MIDI file
// midiPlayer.playMIDI(file, midiDataCallback, minimumVelocity); 