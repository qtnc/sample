#include "../common/bass.h"
#include "LoaderFFMPEG.hpp"
#include "../common/println.hpp"

std::vector<std::pair<DWORD,std::string>> GetTrackList (DWORD handle) {
std::vector<std::pair<DWORD,std::string>> result;
println("GetTrackList: begin, handle={:#X}", handle);
do {
if (!handle) break;
BASS_CHANNELINFO info;
BASS_ChannelGetInfo(handle, &info);
println("Info: ct={:#X}, ffmpeg={:#X}", info.ctype, BASS_CTYPE_STREAM_FFMPEG);
if (info.ctype != BASS_CTYPE_STREAM_FFMPEG) break;
static BASS_FFMPEG_GetTracks getter = NULL;
if (!getter) {
auto dll = LoadLibraryA("bass_ffmpeg.dll");
println("DLL={:p}", (const void*)dll);
if (!dll) break;
getter = (BASS_FFMPEG_GetTracks) GetProcAddress(dll, "BASS_FFMPEG_GetTracks");
}
println("getter={:p}", (const void*)getter);
if (!getter) break;
DWORD len = 15; //getter(handle, NULL, 0);
FFMPEG_TRACK tracks[len] = { { 0, 0 } };
len = getter(handle, tracks, len);
println("len={}", len);
for (DWORD i=0; i<len; i++) {
auto& track = tracks[i];
println("track #{}: {}", track.index, track.title);
result.emplace_back(std::make_pair(track.index, track.title));
}
} while(false);
return result;
}
