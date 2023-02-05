#ifndef _____BASS_FONT_CONFIG_____1
#define _____BASS_FONT_CONFIG_____1
#include "bass.h"
#include "bassmidi.h"
#include<string>

struct BassFontConfig: BASS_MIDI_FONTEX {
float volume;
DWORD flags;
std::string file;

BassFontConfig () = default;
BassFontConfig (const std::string& fn, HSOUNDFONT fnt, int sp, int sb, int dp, int db, int dblsb, float vol, DWORD fl):
BASS_MIDI_FONTEX({ fnt, sp, sb, dp, db, dblsb }),
file(fn), volume(vol), flags(fl) {}
};

#endif
