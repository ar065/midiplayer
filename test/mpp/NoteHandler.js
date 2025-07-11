const isNodeJS = () => typeof process !== 'undefined' && process.versions?.node;

export default class NoteHandler {
    constructor() {
        this.noteBuffer = [];
        this.noteBufferTime = 0;
        this.isNodeJS = isNodeJS();
    }

    readUint48(dataView, byteOffset) {
        return dataView.getUint32(byteOffset, true) + 
               (dataView.getUint16(byteOffset + 4, true) * 0x100000000);
    }

    writeUint48(dataView, byteOffset, value) {
        dataView.setUint32(byteOffset, value & 0xFFFFFFFF, true);
        dataView.setUint16(byteOffset + 4, Math.floor(value / 0x100000000) & 0xFFFF, true);
    }

    base64ToArrayBuffer(base64) {
        const binaryString = atob(base64);
        const bytes = new Uint8Array(binaryString.length);
        
        for (let i = 0; i < bytes.length; i++) {
            bytes[i] = binaryString.charCodeAt(i);
        }
        
        return bytes.buffer;
    }

    arrayBufferToBase64(buffer) {
        if (this.isNodeJS) {
            return Buffer.from(buffer).toString("base64");
        }
        
        const bytes = new Uint8Array(buffer);
        let binary = '';
        for (let i = 0; i < bytes.byteLength; i++) {
            binary += String.fromCharCode(bytes[i]);
        }
        return btoa(binary);
    }

    notesToBase64(notes, timestamp) {
        const buffer = new ArrayBuffer(6 + 3 * notes.length);
        const dv = new DataView(buffer);

        this.writeUint48(dv, 0, timestamp);

        notes.forEach((note, index) => {
            const offset = 6 + index * 3;
            dv.setUint8(offset, note.note);
            dv.setUint8(offset + 1, note.delay);
            dv.setUint8(offset + 2, note.velocity);
        });

        return this.arrayBufferToBase64(buffer);
    }

    flush(serverTimeOffset) {
        if (!this.noteBufferTime || this.noteBuffer.length === 0) return null;
        
        const notes = this.notesToBase64(this.noteBuffer, this.noteBufferTime + serverTimeOffset);
        const msg = `[{"m":"custom","data":{"n":"${notes}"},"target":{"mode":"subscribed"}}]`;

        this.noteBufferTime = 0;
        this.noteBuffer = [];

        return msg;
    }

    _addNote(note, velocity, emergCb) {
        if (!this.noteBufferTime) {
            this.noteBufferTime = Date.now();
        }
        
        this.noteBuffer.push({
            note,
            delay: this.noteBufferTime ? Date.now() - this.noteBufferTime : 0,
            velocity
        });

        if (this.noteBuffer.length >= 65000 && emergCb) {
            console.log("EMERGENCY CALLBACK DUE TO MAX BUFFER LENGTH", this.noteBuffer.length);
            emergCb();
        }
    }

    startNote(note, velocity, emergCb) {
        this._addNote(note, Math.round(velocity * 127), emergCb);
    }

    stopNote(note, emergCb) {
        this._addNote(note, 0, emergCb);
    }

    parseBinaryNotes(base64) {
        const binary = this.base64ToArrayBuffer(base64);
        const dv = new DataView(binary);
        const timestamp = this.readUint48(dv, 0);
        const notes = [];

        for (let offset = 6; offset + 2 < dv.byteLength; offset += 3) {
            notes.push({
                note: dv.getUint8(offset),
                delay: dv.getUint8(offset + 1),
                velocity: dv.getUint8(offset + 2)
            });
        }

        return { timestamp, notes };
    }
}