#include "bass.h"
#include "bassenc.h"
#include<cstdlib>
#include<cstring>
#include<string>
#include<vector>
#include<cmath>
#include "println.hpp"
#include "stringUtils.hpp"
using namespace std;


bool BASS_SimpleInit (int device) {
return BASS_Init(device, 48000, 0, 0, 0) || BASS_ERROR_ALREADY==BASS_ErrorGetCode();
}

bool BASS_RecordSimpleInit (int device) {
return BASS_RecordInit(device) || BASS_ERROR_ALREADY==BASS_ErrorGetCode();
}

vector<pair<int,string>> getDeviceList ( BOOL(*CALLBACK func)(DWORD, BASS_DEVICEINFO*) , bool includeLoopback = false) {
vector<pair<int,string>> list;
BASS_DEVICEINFO info;
for (int i=0; func(i, &info); i++) {
if (!(info.flags&BASS_DEVICE_ENABLED)) continue;
if ((info.flags&BASS_DEVICE_LOOPBACK) && !includeLoopback) continue;
list.push_back(make_pair(i, info.name));
}
return list;
}

vector<pair<int,string>> BASS_GetDeviceList (bool includeLoopback = false) {
return getDeviceList(BASS_GetDeviceInfo, includeLoopback);
}

vector<pair<int,string>> BASS_RecordGetDeviceList (bool includeLoopback = false) {
return getDeviceList(BASS_RecordGetDeviceInfo, includeLoopback);
}

static void CALLBACK DSPCopyProc (HDSP dsp, DWORD source, void* buffer, DWORD length, void* dest) {
BASS_StreamPutData(reinterpret_cast<DWORD>(dest), buffer, length);
}

static void CALLBACK SyncFreeProc (HSYNC sync, DWORD channel, DWORD param, void* dest) {
BASS_StreamFree(reinterpret_cast<DWORD>(dest));
}

DWORD BASS_StreamCreateCopy (DWORD source, bool decode) {
BASS_CHANNELINFO info;
BASS_ChannelGetInfo(source, &info);
DWORD dest = BASS_StreamCreate(info.freq, info.chans, BASS_SAMPLE_FLOAT | (decode? BASS_STREAM_DECODE : BASS_STREAM_AUTOFREE), STREAMPROC_PUSH, nullptr);
BASS_ChannelSetDSP(source, DSPCopyProc, reinterpret_cast<void*>(dest), 0);
BASS_ChannelSetSync(source, BASS_SYNC_FREE, 0, SyncFreeProc, reinterpret_cast<void*>(dest));
return dest;
}


int BASS_CastGetListenerCount (DWORD encoder) {
const char *stats;
int listeners = -1;
if (stats=BASS_Encode_CastGetStats(encoder, BASS_ENCODE_STATS_SHOUT, NULL)) {
    const char *t=strstr(stats, "<CURRENTLISTENERS>"); // Shoutcast listener count
    if (t) listeners=atoi(t+18);
} else if (stats=BASS_Encode_CastGetStats(encoder, BASS_ENCODE_STATS_ICE, NULL)) {
    const char *t=strstr(stats, "<Listeners>"); // Icecast listener count
    if (t) listeners=atoi(t+11);
}
return listeners;
}

double linear2db (double x) {
return x<=0? -120 : 20*log10(x);
}

float computeReplayGain (DWORD stream) {
std::vector<float> gains;
float level=0, length=0.1f;
while (BASS_ChannelGetLevelEx(stream, &level, length, BASS_LEVEL_MONO | BASS_LEVEL_RMS)) {
gains.push_back(level);
}
std::stable_sort(gains.begin(), gains.end() );
return -15 -linear2db(gains[gains.size() * 19 / 20]);
}

