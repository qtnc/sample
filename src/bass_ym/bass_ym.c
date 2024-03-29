#include <stdio.h>
#include <string.h>
#include "StSoundLibrary.h"
#include "../common/bass.h"
#include "../common/bass-addon.h"
#ifndef bassfunc
const BASS_FUNCTIONS *bassfunc=NULL;
#endif

#define BASS_CTYPE_MUSIC_YM 0x20015

#define EXPORT __declspec(dllexport)

typedef struct {
HSTREAM handle;
YMMUSIC* mod;
} YMStream;

extern const ADDON_FUNCTIONS funcs;

static DWORD modFlagsFwd = BASS_MUSIC_DECODE   | BASS_SAMPLE_FLOAT | BASS_MUSIC_AUTOFREE | BASS_SAMPLE_LOOP | BASS_MUSIC_NOSAMPLE | 0x3f000000;

static void WINAPI YMFree (YMStream  *stream) {
if (!stream) return;
if (stream->mod) ymMusicDestroy(stream->mod);
free(stream);
}

static DWORD CALLBACK StreamProc(HSTREAM handle, BYTE *buffer, DWORD length, YMStream *stream) {
if (!stream || !stream->mod) return BASS_STREAMPROC_END;
if (ymMusicCompute(stream->mod, (short*)buffer, length/sizeof(short) )) return length;
if (ymMusicIsOver(stream->mod)) ymMusicRestart(stream->mod);
return BASS_STREAMPROC_END;
}

static int readFully (BASSFILE file, void* buffer, int length) {
int pos=0, read=0;
while(pos<length && (read=bassfunc->file.Read(file, buffer+pos, length-pos))>0) pos+=read;
return pos;
}

static HSTREAM WINAPI StreamCreateProc(BASSFILE file, DWORD flags) {
DWORD length = bassfunc->file.GetPos(file, BASS_FILEPOS_END);
if (length>65535) error(BASS_ERROR_FILEFORM);
char* buffer = malloc(length);
if (!buffer || readFully(file, buffer, length)!=length)  error(BASS_ERROR_FILEFORM);
YMMUSIC* ym = ymMusicCreate();
if (!ym) error(BASS_ERROR_FILEFORM);
if (!ymMusicLoadMemory(ym, buffer, length)) {
free(buffer);
ymMusicDestroy(ym);
error(BASS_ERROR_FILEFORM);
}
ymMusicSetLowpassFiler(ym, TRUE);
ymMusicSetLoopMode(ym, flags&BASS_SAMPLE_LOOP);
ymMusicPlay(ym);
free(buffer);
bassfunc->file.Close(file);
YMStream* stream = malloc(sizeof(YMStream));
memset(stream, 0, sizeof(YMStream));
stream->mod = ym;
	flags&=BASS_SAMPLE_SOFTWARE|BASS_SAMPLE_LOOP|BASS_SAMPLE_3D|BASS_SAMPLE_FX 		|BASS_STREAM_DECODE|BASS_STREAM_AUTOFREE|0x3f000000; // 0x3f000000 = all speaker flags
	stream->handle=bassfunc->CreateStream(44100, 1, flags, &StreamProc, stream, &funcs);
	if (!stream->handle) { 
YMFree(stream);
ymMusicStop(ym);
ymMusicDestroy(ym);
		return 0;
	}
noerrorn(stream->handle);
}

HSTREAM WINAPI EXPORT BASS_YM_StreamCreateFile(BOOL mem, const void *file, QWORD offset, QWORD length, DWORD flags) {
	HSTREAM s;
	BASSFILE bfile =bassfunc->file.Open(mem,file,offset,length,flags,TRUE);
	if (!bfile) return 0; 
	s=StreamCreateProc(bfile,flags);
	if (!s) bassfunc->file.Close(bfile);
	return s;
}

HSTREAM WINAPI EXPORT BASS_YM_StreamCreateURL(const char *url, DWORD offset, DWORD flags, DOWNLOADPROC *proc, void *user) {
	HSTREAM s;
	BASSFILE bfile =bassfunc->file.OpenURL(url,offset,flags,proc,user,TRUE);
	if (!bfile) return 0; 
	s=StreamCreateProc(bfile,flags);
	if (!s) bassfunc->file.Close(bfile);
	return s;
}

HSTREAM WINAPI EXPORT BASS_YM_StreamCreateFileUser(DWORD system, DWORD flags, const BASS_FILEPROCS *procs, void *user) {
	HSTREAM s;
	BASSFILE bfile =bassfunc->file.OpenUser(system,flags,procs,user,TRUE);
	s=StreamCreateProc(bfile,flags);
	if (!s) bassfunc->file.Close(bfile);
	return s;
}

static QWORD WINAPI YMGetLength(YMStream* stream, DWORD mode) {
if (mode!=BASS_POS_BYTE) errorn(BASS_ERROR_NOTAVAIL); // only support byte positioning
ymMusicInfo_t info;
memset(&info, 0, sizeof(info));
ymMusicGetInfo(stream->mod, &info);
	noerrorn(info.musicTimeInMs * 44100 * sizeof(short) / 1000);
}

static void WINAPI YMGetInfo(YMStream* stream, BASS_CHANNELINFO *info) {
info->freq = 44100;
info->chans = 1;
	info->ctype = BASS_CTYPE_MUSIC_YM;
info->origres = 16; 
}

static BOOL WINAPI YMCanSetPosition(YMStream *stream, QWORD pos, DWORD mode) {
	if ((BYTE)mode!=BASS_POS_BYTE) error(BASS_ERROR_NOTAVAIL); // only support byte positioning (BYTE = ignore flags)
if (!ymMusicIsSeekable(stream->mod)) error(BASS_ERROR_NOTAVAIL);
if (pos>YMGetLength(stream, mode)) error(BASS_ERROR_POSITION);
	return TRUE;
}

static QWORD WINAPI YMSetPosition(YMStream* stream, QWORD pos, DWORD mode) {
	if ((BYTE)mode!=BASS_POS_BYTE) error(BASS_ERROR_NOTAVAIL); // only support byte positioning (BYTE = ignore flags)
if (!ymMusicIsSeekable(stream->mod)) error(BASS_ERROR_NOTAVAIL);
ymMusicSeek(stream->mod, pos * 1000 / (44100 * sizeof(short)));
return pos;
}

static const char* WINAPI YMTags (YMStream* stream, DWORD type) {
if (!stream || !stream->mod) return NULL;
ymMusicInfo_t info;
memset(&info, 0, sizeof(info));
ymMusicGetInfo(stream->mod, &info);
switch(type){
case BASS_TAG_MUSIC_NAME: return info.pSongName;
case BASS_TAG_MUSIC_AUTH: return info.pSongAuthor;
case BASS_TAG_MUSIC_MESSAGE: return info.pSongComment;
}
return NULL;
}

const ADDON_FUNCTIONS funcs={
	0, // flags
	YMFree,
	YMGetLength,
YMTags, 
	NULL, // let BASS handle file position
YMGetInfo,
YMCanSetPosition,
YMSetPosition,
	NULL, // let BASS handle the position/looping/syncing (POS/END)
NULL, //	RAW_SetSync,
NULL, //	RAW_RemoveSync,
	NULL, // let BASS decide when to resume a stalled stream
	NULL, // no custom flags
	NULL // no attributes
};

static const BASS_PLUGINFORM frm[] = { 
{ BASS_CTYPE_MUSIC_YM, "YM Module", "*.ym" }
};
static BASS_PLUGININFO plugininfo = {0x02040000, 1, frm };

const void* WINAPI EXPORT BASSplugin(DWORD face) {
switch (face) {
		case BASSPLUGIN_INFO:
			return (void*)&plugininfo;
		case BASSPLUGIN_CREATE:
			return (void*)StreamCreateProc;
//		case BASSPLUGIN_CREATEURL:
//			return (void*)StreamCreateURLProc;
	}
	return NULL;
}

BOOL WINAPI EXPORT DllMain(HANDLE hDLL, DWORD reason, LPVOID reserved)
{
	switch (reason) {
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls((HMODULE)hDLL);
			if (HIWORD(BASS_GetVersion())!=BASSVERSION || !GetBassFunc()) {
				MessageBoxA(0,"Incorrect BASS.DLL version (" BASSVERSIONTEXT " is required)", "BASS", MB_ICONERROR | MB_OK);
				return FALSE;
			}
			break;
}
	return TRUE;
}
