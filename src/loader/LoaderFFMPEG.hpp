#ifndef ___LOADER_1FZ___
#define ___LOADER_1FZ___
#include<string>
#include<vector>
#include<utility>

#define BASS_CTYPE_STREAM_FFMPEG 0x1f301
#define BASS_POS_TRACK 4
#define FFMPEG_TRACK_TITLE_LENGTH 30

typedef struct {
	DWORD index;
	char title[FFMPEG_TRACK_TITLE_LENGTH];
} FFMPEG_TRACK;

typedef DWORD(*WINAPI BASS_FFMPEG_GetTracks)(DWORD, FFMPEG_TRACK*, DWORD);

std::vector<std::pair<DWORD,std::string>> GetTrackList (DWORD handle);


#endif
