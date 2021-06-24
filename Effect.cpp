#include "Effect.hpp"
#include<unordered_map>
#include<sstream>
#include "cpprintf.hpp"
#include "stringUtils.hpp"
#include "bass.h"
#include "bass_fx.h"
using namespace std;

unordered_map<std::string, EffectDesc> effectDescs = {
#define E(I,N,T,...) { N, { I, N, T, { __VA_ARGS__ } } },
E(BASS_FX_BFX_ROTATE, "rotate", "fi", "rate", "channels")
E(BASS_FX_BFX_PHASER, "phaser", "ffffffi", "dry", "wet", "feedback", "rate", "range", "freq", "channels")
E(BASS_FX_BFX_CHORUS, "chorus","ffffffi", "dry", "wet", "feedback", "min", "max", "rate", "channels")
E(BASS_FX_BFX_DISTORTION, "distortion", "fffffi", "drive", "dry", "wet", "feedback", "volume", "channels")
E(BASS_FX_BFX_COMPRESSOR2, "compressor", "fffffi", "gain", "treshhold", "ratio", "attack", "release", "channels")
E(BASS_FX_BFX_BQF, "bqf", "ifffffi", "type", "freq", "gain", "bandwidth", "quality", "slope", "channels")
E(BASS_FX_BFX_ECHO4, "echo", "ffffbi", "dry", "wet", "feedback", "delay", "stereo", "channels")
E(BASS_FX_BFX_PITCHSHIFT, "pitchshift", "ffiii", "pitch", "semitones", "fftsize", "oversampling", "channels")
E(BASS_FX_BFX_FREEVERB, "freeverb", "fffffbi", "dry", "wet", "roomsize", "damping", "width", "freeze", "channels")
E(BASS_FX_DX8_CHORUS, "dx8chorus", "ffffifi", "wetdry", "depth", "feedback", "rate", "waveform", "delay", "phase")
E(BASS_FX_DX8_DISTORTION, "dx8distortion", "fffff", "gain", "edge", "freq", "bandwidth", "cutoff")
E(BASS_FX_DX8_ECHO, "dx8echo", "ffffb", "wetdry", "feedback", "leftdelay", "rightdelay", "pandelay")
E(BASS_FX_DX8_FLANGER, "flanger", "ffffifi", "wetdry", "depth", "feedback", "rate", "waveform", "delay", "phase")
E(BASS_FX_DX8_GARGLE, "gargle", "ii", "rate", "shape")
E(BASS_FX_DX8_REVERB, "dx8reverb", "ffff", "ingain", "outgain", "time", "hfrtratio")
E(BASS_FX_DX8_I3DL2REVERB, "dx8i3dl2reverb", "iifffififfff", "attenuation", "attenuationhf", "rolloff", "decay", "decayhfratio", "reflection", "refelectiondelay", "reverb", "reverbdelay", "diffusion", "density", "hfreference")
E(BASS_FX_DX8_COMPRESSOR, "dx8compressor", "ffffff", "gain", "attack", "release", "treshhold", "ratio", "predelay")
#undef E
{ "", { 0, "", "", {} }}
};

static double parseDouble (const std::string& s) {
istringstream in(s);
double d = 0;
in >> d;
return d;
}

EffectParams::EffectParams ():
fxType(0), name(), buf{0}, on(false), handle(0)
{}

void EffectParams::setFloat (int index, float value) {
reinterpret_cast<float*>(buf) [index] = value;
}

void EffectParams::setInt (int index, int value) {
reinterpret_cast<int*>(buf) [index] = value;
}

std::vector<EffectParams> EffectParams::read (std::istream& in) {
std::vector<EffectParams> effects;
EffectDesc* desc = nullptr;
string s;
while(std::getline(in, s)) {
trim(s);
if (starts_with(s, "[") && ends_with(s, "]")) {
s = trim_copy(string(s.begin()+1, s.end()-1));
effects.emplace_back();
effects.back().name = s;
desc = nullptr;
}
auto i = s.find('=');
if (i==string::npos) continue;
string key = s.substr(0, i), value = s.substr(i+1);
trim(key); trim(value); to_lower(key);
auto& effect = effects.back();
if (key=="effect") {
auto it = effectDescs.find(value);
if (it!=effectDescs.end()) {
desc = &it->second;
effect.fxType = desc->fxID;
}
else desc=nullptr;
continue;
}
if (!desc) continue;
auto it = std::find(desc->paramNames.begin(), desc->paramNames.end(), key);
if (it==desc->paramNames.end()) continue;
auto index = it-desc->paramNames.begin();
auto valtype = desc->paramTypes[index];
switch(valtype){
case 'f': effect.setFloat(index, parseDouble(value)); break;
case 'i': effect.setInt(index, std::stoi(value)); break;
}
}
return effects;
}
