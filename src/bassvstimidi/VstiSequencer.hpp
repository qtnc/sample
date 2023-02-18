#ifndef ____VSTIMIDISEQUENCER_1
#define ____VSTIMIDISEQUENCER_1
#include "MidiFile.hpp"
#include "../common/bass.h"
#include "../common/bassmidi.h"
#include "../common/bass_vst.h"
#include<iosfwd>

struct VstiSequencer {
MidiFile midi;
DWORD vsti;
DWORD bytePos, iPos;
DWORD freq, flags, granul;

VstiSequencer (DWORD freq, DWORD flags);
bool Advance (DWORD length, bool ignoreNotes = false);
bool Seek (DWORD pos);
DWORD Decode (void* buffer, DWORD length);
bool LoadMIDI (std::istream& in);
bool CreateVSTI (const std::string& vstiDll);
~VstiSequencer();
};

#endif
