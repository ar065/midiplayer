import { parentPort, workerData } from 'worker_threads';
import { createRequire } from "module";
const require = createRequire(import.meta.url);
const midiPlayer = require("../../build/linux/x86_64/release/midi_player.node");

const { file, minimumVelocity } = workerData;

function midiDataCallback(data) {
    const buf = new Uint32Array([data]);
    parentPort.postMessage(buf.buffer, [buf.buffer]); // transfer ownership
}

const val = midiPlayer.playMIDI(file, midiDataCallback, minimumVelocity);
console.log("MIDIPLAYER VALUE:", val);