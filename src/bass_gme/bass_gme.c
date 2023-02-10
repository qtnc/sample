#include <stdio.h>
#include <string.h>
#include "gme.h"
#include "../common/bass.h"
#include "../common/bass-addon.h"
#ifndef bassfunc
const BASS_FUNCTIONS *bassfunc=NULL;
#endif

#define BASS_CTYPE_MUSIC_GME 0x20012
#define BASS_POS_SUBSONG 6

#define MAX_BUFFER_LENGTH 1024
#define FADE_TIME 7000

#define EXPORT __declspec(dllexport)

typedef struct {
HSTREAM handle;
Music_Emu* emu;
int curTrack, nTracks, curPos;
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

static BOOL GMEStartTrack (GMEStream* stream, int n) {
if (n<0 || n>=stream->nTracks) return FALSE;
stream->curPos = 0;
stream->curTrack = n;
gme_start_track(stream->emu, n);
if (stream->trackInfo[n]->length<=0) gme_set_fade(stream->emu, stream->trackInfo[n]->play_length -FADE_TIME);
return TRUE;
}

static DWORD CALLBACK StreamProc(HSTREAM handle, BYTE *buffer, DWORD totalLength, GMEStream *stream) {
DWORD pos = 0;
while(pos<totalLength){
if (stream->curTrack<0 || gme_track_ended(stream->emu)) {
stream->curPos += pos;
return pos | BASS_STREAMPROC_END;
}
DWORD length = totalLength - pos;
if (length>MAX_BUFFER_LENGTH) length=MAX_BUFFER_LENGTH;
if (stream->useFloat) {
DWORD gen = gme_tell_samples(stream->emu);
gme_play(stream->emu, length/4, (short*)(buffer+pos));
length = 4 * (gme_tell_samples(stream->emu) -gen);
bassfunc->data.Int2Float(buffer+pos, (float*)(buffer+pos), length/4, 2);
}
else {
DWORD gen = gme_tell_samples(stream->emu);
gme_play(stream->emu, length/2, (short*)(buffer+pos));
length = 2 * (gme_tell_samples(stream->emu) -gen);
}
pos += length;
}
stream->curPos += totalLength;
return totalLength;
}

static int readFile (BASSFILE file, void* buffer, int length) {
int pos=0, read=0;
while(pos<length && (read=bassfunc->file.Read(file, buffer+pos, length-pos))>0) pos+=read;
return pos;
}

static int readFully (BASSFILE file, char** buffer, int pos, int length) {
if (length>0) {
*buffer = realloc(*buffer, length);
if (!*buffer) return -1;
return readFile(file, (*buffer)+pos, length-pos);
}

length = 2048;
do {
length *= 2;
*buffer = realloc(*buffer, length);
if (!*buffer) return -1;
int read = readFile(file, (*buffer)+pos, length-pos);
if (read<0) return -1;
pos += read;
} while(pos==length);
return pos;
}


static HSTREAM WINAPI StreamCreateProc(BASSFILE file, DWORD flags) {
char* buffer = malloc(4);
if (!buffer) error(BASS_ERROR_FILEFORM);
if (4!=readFile(file, buffer, 4)) error(BASS_ERROR_FILEFORM);
if (!*gme_identify_header(buffer)) error(BASS_ERROR_FILEFORM);
int length = bassfunc->file.GetPos(file, BASS_FILEPOS_END);
length = readFully(file, &buffer, 4, length);
if (!buffer || length<0) error(BASS_ERROR_FILEFORM);

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
gme_enable_accuracy(emu, 1);
for (int i=0; i<stream->nTracks; i++) {
stream->trackInfo[i] = NULL;
gme_track_info(emu, &stream->trackInfo[i], i);
stream->trackInfo[i]->play_length = computePlayLength(stream->trackInfo[i]);
}
GMEStartTrack(stream, 0);


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

static BOOL WINAPI GMEAttribute (GMEStream* stream, DWORD attr, float* value, BOOL set) {
if (set) switch(attr){
}
else switch(attr){
case BASS_ATTRIB_MUSIC_ACTIVE:
*value = gme_voice_count(stream->emu);
return TRUE;
}
return FALSE;
}

static QWORD WINAPI GMEGetLength(GMEStream* stream, DWORD mode) {
switch(mode){
case BASS_POS_BYTE:
noerrorn( stream->trackInfo[stream->curTrack]->play_length * 4 * 48 * (stream->useFloat? 2 : 1) );
case BASS_POS_SUBSONG:
noerrorn( stream->nTracks );
default:
error(BASS_ERROR_NOTAVAIL);
}}

static BOOL WINAPI GMECanSetPosition(GMEStream *stream, QWORD pos, DWORD mode) {
switch(mode&0xFF){
case BASS_POS_BYTE: {
QWORD length = GMEGetLength(stream, BASS_POS_BYTE);
if (pos>length) error(BASS_ERROR_POSITION);
noerrorn(TRUE);
}
case BASS_POS_SUBSONG:
if (pos<0 || pos>=stream->nTracks) error(BASS_ERROR_POSITION);
noerrorn(TRUE);
}
error(BASS_ERROR_NOTAVAIL);
}

static QWORD WINAPI GMEGetPosition (GMEStream* stream, QWORD pos, DWORD mode) {
switch(mode&0xFF){
case BASS_POS_BYTE:
noerrorn(stream->curPos);
case BASS_POS_SUBSONG:
noerrorn(stream->curTrack);
default:
error(BASS_ERROR_NOTAVAIL);
}}

static QWORD WINAPI GMESetPosition(GMEStream* stream, QWORD pos, DWORD mode) {
switch(mode){
case BASS_POS_BYTE: {
int msec = pos / (4 * 48 * (stream->useFloat? 2 : 1));
gme_seek(stream->emu, msec);
stream->curPos = pos;
noerrorn(pos);
}
case BASS_POS_SUBSONG:
GMEStartTrack(stream, pos);
noerrorn(stream->curTrack);
default:
error(BASS_ERROR_NOTAVAIL);
}}

static void WINAPI GMEGetInfo(GMEStream* stream, BASS_CHANNELINFO *info) {
info->freq = 48000;
info->chans = 2;
	info->ctype = BASS_CTYPE_MUSIC_GME;
info->origres = 16; 
}

static const char* WINAPI GMETags (GMEStream* stream, DWORD type) {
if (!stream || !stream->emu) return NULL;
int tn = stream->curTrack;
if (tn<0 || tn>=stream->nTracks) tn=0;
gme_info_t* t = stream->trackInfo[tn];
switch(type){
case BASS_TAG_MUSIC_NAME: return t->song&&*t->song? t->song : t->game;
case BASS_TAG_MUSIC_AUTH: return t->author;
case BASS_TAG_MUSIC_MESSAGE: return t->comment;
}
if (type>=BASS_TAG_MUSIC_SAMPLE && type<BASS_TAG_MUSIC_SAMPLE+gme_voice_count(stream->emu)) {
return gme_voice_name(stream->emu, type-BASS_TAG_MUSIC_SAMPLE);
}
if (type>=BASS_TAG_MUSIC_INST && type<BASS_TAG_MUSIC_INST+6) switch(type-BASS_TAG_MUSIC_INST){
case 0: return t->system;
case 1: return t->game;
case 2: return t->song;
case 3: return t->author;
case 4: return t->copyright;
case 5: return t->dumper;
}
return NULL;
}

const ADDON_FUNCTIONS funcs={
ADDON_OWNPOS, // flags
GMEFree,
GMEGetLength,
GMETags, 
	NULL, // let BASS handle file position
GMEGetInfo,
GMECanSetPosition,
GMESetPosition,
GMEGetPosition,
NULL, //	RAW_SetSync,
NULL, //	RAW_RemoveSync,
	NULL, // let BASS decide when to resume a stalled stream
	NULL, // no custom flags
GMEAttribute
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
