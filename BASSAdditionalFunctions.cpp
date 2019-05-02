#include "WXWidgets.hpp"
#include "bass.h"
#include<string>
#include<vector>
using namespace std;

bool BASS_SimpleInit (int device) {
return BASS_Init(device, 48000, 0, 0, 0) || BASS_ERROR_ALREADY==BASS_ErrorGetCode();
}

bool BASS_RecordSimpleInit (int device) {
return BASS_RecordInit(device) || BASS_ERROR_ALREADY==BASS_ErrorGetCode();
}

vector<string> getDeviceList ( BOOL(*CALLBACK func)(DWORD, BASS_DEVICEINFO*) ) {
vector<string> list;
BASS_DEVICEINFO info;
for (int i=0; func(i, &info); i++) {
//wxString uName(reinterpret_cast<const wxChar*>(info.name));
//list.push_back(U(uName));
list.push_back(info.name);
}
return list;
}

vector<string> BASS_GetDeviceList () {
return getDeviceList(BASS_GetDeviceInfo);
}

vector<string> BASS_RecordGetDeviceList () {
return getDeviceList(BASS_RecordGetDeviceInfo);
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
