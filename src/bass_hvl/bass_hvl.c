#include <stdio.h>
#include <string.h>
#include "hvl_replay.h"
#include "../common/bass.h"
#include "../common/bass-addon.h"
#ifndef bassfunc
const BASS_FUNCTIONS *bassfunc=NULL;
#endif

#define BASS_CTYPE_MUSIC_HVL 0x20013
#define BASS_POS_SUBSONG 6
#define SAMPLE_RATE 48000
#define SAMPLE_BUFFER_LENGTH (SAMPLE_RATE / 25)

#define EXPORT __declspec(dllexport)

typedef struct {
HSTREAM handle;
struct hvl_tune* hvl;
DWORD curPos, inPos;
BOOL useFloat;
short buffer[SAMPLE_BUFFER_LENGTH];
} HVLStream;

extern const ADDON_FUNCTIONS funcs;

static void WINAPI HVLFree (HVLStream  *stream) {
if (!stream) return;
if (stream->hvl) hvl_FreeTune(stream->hvl);
free(stream);
}

static BOOL HVLStartTrack (HVLStream* stream, int n) {
if (n<0 || n>=stream->hvl->ht_SubsongNr) return FALSE;
stream->curPos = 0;
hvl_InitSubsong(stream->hvl, n);
return TRUE;
}

static DWORD CALLBACK StreamProc(HSTREAM handle, BYTE *buffer, DWORD length, HVLStream *stream) {
DWORD sLength = length / (stream->useFloat? 4 : 2);
short *buf = (short*)buffer, *bufEnd=buf+sLength;
while(buf<bufEnd){
if (stream->inPos<=0 || stream->inPos>=SAMPLE_BUFFER_LENGTH) {
hvl_DecodeFrame(stream->hvl, stream->buffer, stream->buffer+1, 4);
stream->inPos = 0;
}
DWORD rem = bufEnd - buf, avail = SAMPLE_BUFFER_LENGTH - stream->inPos;
DWORD filled = rem<avail? rem : avail;
memcpy(buf, stream->buffer + stream->inPos, filled * sizeof(short));
stream->inPos += filled;
buf += filled;
}
if (stream->useFloat) bassfunc->data.Int2Float(buffer, (float*)buffer, length/4, 2);
stream->curPos += length;
if (stream->hvl->ht_SongEndReached) {
stream->hvl->ht_SongEndReached = FALSE;
stream->curPos = 0;
length |= BASS_POS_END;
}
return length;
}

static BOOL isHVL (const char* buf) {
return (
(!strncmp(buf, "THX", 3) && buf[3]>=0 && buf[3]<3)
|| (!strncmp(buf, "HVL", 3) && buf[3]>=0 && buf[3]<2)
);
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
if (!isHVL(buffer)) error(BASS_ERROR_FILEFORM);
int length = bassfunc->file.GetPos(file, BASS_FILEPOS_END);
length = readFully(file, &buffer, 4, length);
if (!buffer || length<0) error(BASS_ERROR_FILEFORM);

static BOOL initialized = FALSE;
if (!initialized) {
hvl_InitReplayer();
initialized = TRUE;
}

struct hvl_tune* hvl = hvl_ParseTune(buffer, length, 48000, 2);
free(buffer);
if (!hvl) error(BASS_ERROR_FILEFORM);
bassfunc->file.Close(file);
HVLStream* stream = malloc(sizeof(HVLStream));
memset(stream, 0, sizeof(HVLStream));
stream->hvl = hvl;
stream->useFloat = flags&BASS_SAMPLE_FLOAT;
HVLStartTrack(stream, 0);

	flags&=BASS_SAMPLE_FLOAT|BASS_SAMPLE_SOFTWARE|BASS_SAMPLE_LOOP|BASS_SAMPLE_3D|BASS_SAMPLE_FX 		|BASS_STREAM_DECODE|BASS_STREAM_AUTOFREE|0x3f000000; // 0x3f000000 = all speaker flags
	stream->handle=bassfunc->CreateStream(48000, 2, flags, &StreamProc, stream, &funcs);
	if (!stream->handle) { 
HVLFree(stream);
return 0;
	}
noerrorn(stream->handle);
}

HSTREAM WINAPI EXPORT BASS_HVL_StreamCreateFile(BOOL mem, const void *file, QWORD offset, QWORD length, DWORD flags) {
	HSTREAM s;
	BASSFILE bfile =bassfunc->file.Open(mem,file,offset,length,flags,TRUE);
	if (!bfile) return 0; 
	s=StreamCreateProc(bfile,flags);
	if (!s) bassfunc->file.Close(bfile);
	return s;
}

HSTREAM WINAPI EXPORT BASS_HVL_StreamCreateURL(const char *url, DWORD offset, DWORD flags, DOWNLOADPROC *proc, void *user) {
	HSTREAM s;
	BASSFILE bfile =bassfunc->file.OpenURL(url,offset,flags,proc,user,TRUE);
	if (!bfile) return 0; 
	s=StreamCreateProc(bfile,flags);
	if (!s) bassfunc->file.Close(bfile);
	return s;
}

HSTREAM WINAPI EXPORT BASS_HVL_StreamCreateFileUser(DWORD system, DWORD flags, const BASS_FILEPROCS *procs, void *user) {
	HSTREAM s;
	BASSFILE bfile =bassfunc->file.OpenUser(system,flags,procs,user,TRUE);
	s=StreamCreateProc(bfile,flags);
	if (!s) bassfunc->file.Close(bfile);
	return s;
}

static QWORD WINAPI HVLGetLength(HVLStream* stream, DWORD mode) {
switch(mode){
case BASS_POS_BYTE:
return -1;
case BASS_POS_SUBSONG:
noerrorn( stream->hvl->ht_SubsongNr);
default:
error(BASS_ERROR_NOTAVAIL);
}}

static BOOL WINAPI HVLCanSetPosition(HVLStream *stream, QWORD pos, DWORD mode) {
switch(mode&0xFF){
case BASS_POS_BYTE: {
QWORD length = HVLGetLength(stream, BASS_POS_BYTE);
if (length==-1 || length<=0 || pos>length) error(BASS_ERROR_POSITION);
noerrorn(TRUE);
}
case BASS_POS_SUBSONG:
if (pos<0 || pos>=stream->hvl->ht_SubsongNr) error(BASS_ERROR_POSITION);
noerrorn(TRUE);
}
error(BASS_ERROR_NOTAVAIL);
}

static QWORD WINAPI HVLGetPosition (HVLStream* stream, QWORD pos, DWORD mode) {
switch(mode&0xFF){
case BASS_POS_BYTE:
noerrorn(stream->curPos);
case BASS_POS_SUBSONG:
noerrorn(stream->hvl->ht_SongNum);
default:
error(BASS_ERROR_NOTAVAIL);
}}

static QWORD WINAPI HVLSetPosition(HVLStream* stream, QWORD pos, DWORD mode) {
switch(mode){
case BASS_POS_BYTE: {
error(BASS_ERROR_POSITION);
//stream->curPos = pos;
//noerrorn(pos);
}
case BASS_POS_SUBSONG:
HVLStartTrack(stream, pos);
noerrorn(stream->hvl->ht_SongNum);
default:
error(BASS_ERROR_NOTAVAIL);
}}

static void WINAPI HVLGetInfo(HVLStream* stream, BASS_CHANNELINFO *info) {
info->freq = 48000;
info->chans = 2;
	info->ctype = BASS_CTYPE_MUSIC_HVL;
info->origres = 16; 
}

static const char* WINAPI HVLTags (HVLStream* stream, DWORD type) {
if (!stream || !stream->hvl) return NULL;
switch(type){
case BASS_TAG_MUSIC_NAME: return stream->hvl->ht_Name;
}
if (type>=BASS_TAG_MUSIC_INST && type<BASS_TAG_MUSIC_INST+stream->hvl->ht_InstrumentNr) return stream->hvl->ht_Instruments[type - BASS_TAG_MUSIC_INST] .ins_Name;
return NULL;
}

const ADDON_FUNCTIONS funcs={
ADDON_OWNPOS, // flags
HVLFree,
HVLGetLength,
HVLTags, 
	NULL, // let BASS handle file position
HVLGetInfo,
HVLCanSetPosition,
HVLSetPosition,
HVLGetPosition,
NULL, //	RAW_SetSync,
NULL, //	RAW_RemoveSync,
	NULL, // let BASS decide when to resume a stalled stream
	NULL, // no custom flags
NULL // attributes
};

static const BASS_PLUGINFORM frm[] = { 
{ BASS_CTYPE_MUSIC_HVL, "HivelyTracker", "*.ahx;*.hvl" }
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
