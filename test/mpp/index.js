import { configDotenv } from "dotenv";
configDotenv();

import { Client } from "mpp-client-net";

// Instantiate a new client
const client = new Client("wss://mppclone.com", process.env.MPPNET_TOKEN);

// Connect to the server
client.start();
// client.setChannel('✧𝓓𝓔𝓥 𝓡𝓸𝓸𝓶✧');
// client.setChannel("Room369758172347");
client.setChannel("cheez");

// Listen for chat messages
client.on('a', msg => {
    if (msg.a == "!ping") {
        // Send a chat message back
        client.sendArray([{
            m: "a",
            message: "Pong!"
        }]);
    }
    if (msg.a == "!overdrive") {
        client.sendArray([{
            m: "a",
            message: "ok 💥"
        }]);
    }
});

let hue = 0;

setInterval(() => {
    hue = (hue + 1) % 360; // Cycle hue from 0 to 359
    const hex = hslToHex(hue, 100, 50);
    client.setColor(hex);
}, 1000 / 60);

function hslToHex(h, s, l) {
    s /= 100;
    l /= 100;
    const k = n => (n + h / 30) % 12;
    const a = s * Math.min(l, 1 - l);
    const f = n =>
        Math.round(255 * (l - a * Math.max(-1, Math.min(k(n) - 3, Math.min(9 - k(n), 1)))));
    return "#" + [f(0), f(8), f(4)].map(x => x.toString(16).padStart(2, '0')).join('');
}

import { createRequire } from "module";
const require = createRequire(import.meta.url);
const midiPlayer = require("../../build/linux/x86_64/release/midi_player.node");

const file = '/run/media/ar06/74EAEFC8EAEF8528/Midis/Folder1/DISCO_sb_arrange.mid';
const minimumVelocity = 0; // Minimum velocity (0-127)

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
midiPlayer.playMIDI(file, midiDataCallback, minimumVelocity); 
