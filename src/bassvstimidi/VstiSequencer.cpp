#include "VstiSequencer.hpp"
#include<algorithm>
#include<sstream>
#include<fstream>
using namespace std;

static inline bool processMidiMessage (DWORD vsti, int status, int data1, int data2) {
DWORD data = ((status&0xFF)<<16) | ((data1&0x7F)<<8) | (data2&0x7F);
bool re = BASS_VST_ProcessEventRaw(vsti, (void*)data, 0);
return true;
}

static inline bool processEvent (DWORD vsti, MidiEvent& e, bool ignoreNotes) {
switch(e.status){
case 0x80 ... 0x9F:
return ignoreNotes? true : processMidiMessage(vsti, e.status, e.data1, e.data2);
case 0xA0 ... 0xBF:
return processMidiMessage(vsti, e.status, e.data1, e.data2);
case 0xC0 ... 0xDF:
return processMidiMessage(vsti, e.status, e.data1, 0);
case 0xE0 ... 0xEF:
return processMidiMessage(vsti, e.status, e.data1&0x7F, (e.data1>>7)&0x7F);
case 0xF0: case 0xF7: {
unsigned char buf[e.data1+1];
buf[0] = e.status;
memcpy(buf+1, &e.data[0], e.data1);
return BASS_VST_ProcessEventRaw(vsti, buf, e.data1+1);
}
case 0xFF:
// Ignore meta events
return true;
default: 
return false;
}}

bool VstiSequencer::Advance (DWORD length, bool ignoreNotes) {
DWORD nEvents = midi.events.size();
if (iPos>=nEvents) {
iPos = 0;
bytePos = 0;
return false;
}
while (midi.events[iPos].time <= bytePos && iPos<nEvents) iPos++;
bytePos += length;
while (midi.events[iPos].time <= bytePos && iPos<nEvents) {
auto& e = midi.events[iPos++];
processEvent(vsti, e, ignoreNotes);
}
return true;
}

bool VstiSequencer::Seek (DWORD pos) {
// Send all sounds off to all channels
for (int i=0; i<16; i++) processMidiMessage(vsti, 0xB0 +i, 120, 0);
if (pos<bytePos) {
iPos=0;
bytePos = 0;
// Send control reset to all channels
for (int i=0; i<16; i++) processMidiMessage(vsti, 0xB0 +i, 121, 0);
}
Advance(pos-bytePos, true);
return true;
}

DWORD VstiSequencer::Decode (void* buffer, DWORD length) {
DWORD pos=0, granul = this->granul;
for (pos=0; pos<length; pos+=granul) {
if (!Advance(granul)) return pos | BASS_STREAMPROC_END;
BASS_ChannelGetData(vsti, static_cast<char*>(buffer) + pos, granul);
}
if (pos<length) {
if (!Advance(length-pos)) return pos | BASS_STREAMPROC_END;
BASS_ChannelGetData(vsti, static_cast<char*>(buffer) + pos, length-pos);
}
return length;
}

VstiSequencer::VstiSequencer (DWORD sr, DWORD fl):
midi(), vsti(0), iPos(0), bytePos(0),
freq(sr), flags(fl), granul(64)
{
}

VstiSequencer::~VstiSequencer () {
if (vsti) BASS_VST_ChannelFree(vsti);
}

bool VstiSequencer::LoadMIDI (std::istream& in) {
if (!midi.open(in)) return false;
midi.sortEvents();
DWORD bps = 4;
if (flags&BASS_SAMPLE_FLOAT) bps*=2;
else if (flags&BASS_SAMPLE_8BITS) bps/=2;
if (flags&BASS_SAMPLE_MONO) bps/=2;
midi.computeTimes(freq * bps);
return true;
}

bool VstiSequencer::CreateVSTI (const string& vstiDll) {
if (vstiDll.empty()) return false;
DWORD fwdFlags = flags & (BASS_SAMPLE_FLOAT | BASS_SAMPLE_8BITS | BASS_SAMPLE_MONO);
vsti = BASS_VST_ChannelCreate(freq, (flags&BASS_SAMPLE_MONO)? 1 : 2, vstiDll.c_str(), BASS_STREAM_DECODE | fwdFlags);
return !!vsti;
}

