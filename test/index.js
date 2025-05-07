const midiPlayer = require("../build/linux/x86_64/release/midi_player.node");

// Play a MIDI file
midiPlayer.playMIDI('/run/media/ar06/74EAEFC8EAEF8528/Midis/A-1/Coldplay - Viva La Vida black final.mid', function(data) {
  console.log(`MIDI data received: 0x${data.toString(16)}`);
}, 1);  // Minimum velocity (0-127)