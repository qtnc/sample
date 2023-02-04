#include <stdio.h>
#include <string.h>
#include "gme.h"
#include "../common/bass.h"
#include "../common/bass-addon.h"
#ifndef bassfunc
const BASS_FUNCTIONS *bassfunc=NULL;
#endif

#define BASS_CTYPE_MUSIC_GME 0x20012

#define FADE_TIME 7000

#define EXPORT __declspec(dllexport)

typedef struct {
HSTREAM handle;
Music_Emu* emu;
int curTrack, nTracks;
BOOL useFloat;
gme_info_t** trackInfo;
} GMEStream;

extern const ADDON_FUNCTIONS funcs;

static int computePlayLength (gme_info_t* t) {
if (t->length>0) return t->length +FADE_TIME;
else if (t->intro_length>=0 && t->loop_length>=0) return t->intro_length + 3*t->loop_length;
else return 180000;
}


static void WINAPI GMEFree (GMEStream  *stream) {
if (!stream) return;
if (stream->trackInfo) {
for (int i=0; i<stream->nTracks; i++) if (stream->trackInfo[i]) gme_free_info(stream->trackInfo[i]);
free(stream->trackInfo);
}
if (stream->emu) gme_delete(stream->emu);
free(stream);
}

static DWORD CALLBACK StreamProc(HSTREAM handle, BYTE *buffer, DWORD length, GMEStream *stream) {
if (stream->curTrack<0 || gme_track_ended(stream->emu)) {
if (++stream->curTrack >= stream->nTracks) {
stream->curTrack = -1;
return BASS_STREAMPROC_END;
}
gme_start_track(stream->emu, stream->curTrack);
gme_set_fade(stream->emu, stream->trackInfo[stream->curTrack]->play_length -FADE_TIME);
}
if (stream->useFloat) {
gme_play(stream->emu, length/4, (short*)buffer);
bassfunc->data.Int2Float(buffer, (float*)buffer, length/4, 2);
}
else gme_play(stream->emu, length/2, (short*)buffer);
return length;
}

static int readFully (BASSFILE file, void* buffer, int length) {
int pos=0, read=0;
while(pos<length && (read=bassfunc->file.Read(file, buffer+pos, length-pos))>0) pos+=read;
return pos;
}

static HSTREAM WINAPI StreamCreateProc(BASSFILE file, DWORD flags) {
DWORD length = bassfunc->file.GetPos(file, BASS_FILEPOS_END);
if (length > 2*1024*1024) error(BASS_ERROR_FILEFORM);
char* buffer = malloc(length);
if (!buffer) error(BASS_ERROR_FILEFORM);
readFully(file, buffer, length );

Music_Emu* emu = NULL;
gme_err_t err = gme_open_data(buffer, length, &emu, 48000);
free(buffer);
if (err || !emu) error(BASS_ERROR_FILEFORM);
bassfunc->file.Close(file);
GMEStream* stream = malloc(sizeof(GMEStream));
memset(stream, 0, sizeof(GMEStream));
stream->emu = emu;
stream->curTrack = -1;
stream->nTracks = gme_track_count(emu);
stream->trackInfo = malloc( sizeof(gme_info_t*) * stream->nTracks );
stream->useFloat = flags&BASS_SAMPLE_FLOAT;
gme_set_autoload_playback_limit(emu, 1);
printf("GME nTracks=%d\n", stream->nTracks);
for (int i=0; i<stream->nTracks; i++) {
stream->trackInfo[i] = NULL;
gme_track_info(emu, &stream->trackInfo[i], i);
stream->trackInfo[i]->play_length = computePlayLength(stream->trackInfo[i]);
}


	flags&=BASS_SAMPLE_FLOAT|BASS_SAMPLE_SOFTWARE|BASS_SAMPLE_LOOP|BASS_SAMPLE_3D|BASS_SAMPLE_FX 		|BASS_STREAM_DECODE|BASS_STREAM_AUTOFREE|0x3f000000; // 0x3f000000 = all speaker flags
	stream->handle=bassfunc->CreateStream(48000, 2, flags, &StreamProc, stream, &funcs);
	if (!stream->handle) { 
GMEFree(stream);
		return 0;
	}
noerrorn(stream->handle);
}

HSTREAM WINAPI EXPORT BASS_GME_StreamCreateFile(BOOL mem, const void *file, QWORD offset, QWORD length, DWORD flags) {
	HSTREAM s;
	BASSFILE bfile =bassfunc->file.Open(mem,file,offset,length,flags,TRUE);
	if (!bfile) return 0; 
	s=StreamCreateProc(bfile,flags);
	if (!s) bassfunc->file.Close(bfile);
	return s;
}

HSTREAM WINAPI EXPORT BASS_GME_StreamCreateURL(const char *url, DWORD offset, DWORD flags, DOWNLOADPROC *proc, void *user) {
	HSTREAM s;
	BASSFILE bfile =bassfunc->file.OpenURL(url,offset,flags,proc,user,TRUE);
	if (!bfile) return 0; 
	s=StreamCreateProc(bfile,flags);
	if (!s) bassfunc->file.Close(bfile);
	return s;
}

HSTREAM WINAPI EXPORT BASS_GME_StreamCreateFileUser(DWORD system, DWORD flags, const BASS_FILEPROCS *procs, void *user) {
	HSTREAM s;
	BASSFILE bfile =bassfunc->file.OpenUser(system,flags,procs,user,TRUE);
	s=StreamCreateProc(bfile,flags);
	if (!s) bassfunc->file.Close(bfile);
	return s;
}

static QWORD WINAPI GMEGetLength(GMEStream* stream, DWORD mode) {
if (mode!=BASS_POS_BYTE) errorn(BASS_ERROR_NOTAVAIL); // only support byte positioning
QWORD length = 0;
for (int i=0; i<stream->nTracks; i++) {
length += stream->trackInfo[i]->play_length * 4 * 48 * (stream->useFloat? 2 : 1);
}
	noerrorn(length);
}

static void WINAPI GMEGetInfo(GMEStream* stream, BASS_CHANNELINFO *info) {
info->freq = 48000;
info->chans = 2;
	info->ctype = BASS_CTYPE_MUSIC_GME;
info->origres = 16; 
}

static BOOL WINAPI GMECanSetPosition(GMEStream *stream, QWORD pos, DWORD mode) {
	if ((BYTE)mode!=BASS_POS_BYTE) error(BASS_ERROR_NOTAVAIL); // only support byte positioning (BYTE = ignore flags)
QWORD length = GMEGetLength(stream, BASS_POS_BYTE);
if (pos>length) error(BASS_ERROR_POSITION);
	return TRUE;
}

static QWORD WINAPI GMESetPosition(GMEStream* stream, QWORD pos, DWORD mode) {
	if ((BYTE)mode!=BASS_POS_BYTE) error(BASS_ERROR_NOTAVAIL); // only support byte positioning (BYTE = ignore flags)
int msec = pos / (4 * 48 * (stream->useFloat? 2 : 1));
int track =  -1;
while (++track < stream->nTracks && msec > stream->trackInfo[track]->play_length) msec -= stream->trackInfo[track]->play_length;
if (track<0 || track>=stream->nTracks) error(BASS_ERROR_POSITION);
if (track != stream->curTrack) {
gme_start_track(stream->emu, track);
stream->curTrack = track;
}
gme_seek(stream->emu, msec);
gme_set_fade(stream->emu, stream->trackInfo[stream->curTrack]->play_length -FADE_TIME);
return pos;
}

static const char* WINAPI GMETags (GMEStream* stream, DWORD type) {
if (!stream || !stream->emu) return NULL;
int tn = stream->curTrack;
if (tn<0 || tn>=stream->nTracks) tn=0;
gme_info_t* t = stream->trackInfo[tn];
switch(type){
case BASS_TAG_MUSIC_NAME: return t->song&&*t->song? t->song : t->game;
case BASS_TAG_MUSIC_AUTH: return t->author;
case BASS_TAG_MUSIC_MESSAGE: return t->comment&&*t->comment? t->comment : t->copyright;
}
return NULL;
}

const ADDON_FUNCTIONS funcs={
	0, // flags
GMEFree,
GMEGetLength,
GMETags, 
	NULL, // let BASS handle file position
GMEGetInfo,
GMECanSetPosition,
GMESetPosition,
	NULL, // let BASS handle the position/looping/syncing (POS/END)
NULL, //	RAW_SetSync,
NULL, //	RAW_RemoveSync,
	NULL, // let BASS decide when to resume a stalled stream
	NULL, // no custom flags
	NULL // no attributes
};

static const BASS_PLUGINFORM frm[] = { 
{ BASS_CTYPE_MUSIC_GME, "Game Music Emu", "*.gbs;*.nsf;*.nsfe;*.gym;*.hes;*.ay;*.kss;*.sap;*.spc;*.vgm;*.vgz" }
};
static BASS_PLUGININFO plugininfo = {0x02040000, 1, frm };

const void* WINAPI EXPORT BASSplugin(DWORD face) {
switch (face) {
		case BASSPLUGIN_INFO:
			return (void*)&plugininfo;
		case BASSPLUGIN_CREATE:
			return (void*)StreamCreateProc;
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
