#include <stdio.h>
#include <string.h>
#include "libopenmpt.h"
#include "../common/bass.h"
#include "../common/bass-addon.h"
#ifndef bassfunc
const BASS_FUNCTIONS *bassfunc=NULL;
#endif

#define BASS_CTYPE_MUSIC_OPENMPT 0x20010

#define smagic(b,s) (!strncmp((b), (s), sizeof((s)) -1))
#define nmagic(b,n) ((*(unsigned int*)(b))==(n))
#define hmagic(b,n) ((*(unsigned short*)(b))==(n))
#define cmagic1(b,s,c) (smagic(b,s) && *(c)>='0' && *(c)<='9')
#define cmagic2(b,s,c,d) (smagic(b,s) && *(c)>='0' && *(c)<='9' && *(d)>='0' && *(d)<='9')

#define EXPORT __declspec(dllexport)

typedef struct {
HSTREAM handle;
openmpt_module* mod;
BOOL useFloat;
} MPTStream;

extern const ADDON_FUNCTIONS funcs;

static BOOL tryMusicFirst = TRUE;
static DWORD modFlags = BASS_MUSIC_POSRESET | BASS_MUSIC_POSRESETEX | BASS_MUSIC_SURROUND | BASS_MUSIC_SURROUND2 | BASS_MUSIC_PRESCAN | BASS_MUSIC_SINCINTER;
static DWORD modFlagsFwd = BASS_MUSIC_DECODE   | BASS_SAMPLE_FLOAT | BASS_MUSIC_AUTOFREE | BASS_SAMPLE_LOOP | BASS_MUSIC_NOSAMPLE | 0x3f000000;

static void WINAPI MPTFree (MPTStream  *stream) {
if (!stream) return;
if (stream->mod) openmpt_module_destroy(stream->mod);
free(stream);
}

static DWORD CALLBACK StreamProc(HSTREAM handle, BYTE *buffer, DWORD length, MPTStream *stream) {
size_t read = 0;
if (stream->useFloat) read = 8 * openmpt_module_read_interleaved_float_stereo(stream->mod, 48000, length/8, buffer);
else read = 4 * openmpt_module_read_interleaved_stereo(stream->mod, 48000, length/4, buffer);
if (read<=0) return BASS_STREAMPROC_END;
return read;
}

static int checkUMXHeader (const char* buf, int len) {
for (int pos=0x40; pos<0x500; pos++) {
if (smagic(buf+pos, "IMPM") || smagic(buf+pos, "Extended Module") || smagic(buf+pos, "SCRM") || nmagic(buf+pos, 0x2e4b2e4d)) return 1;
}
return 0;
}

static inline int checkModHeaders (const char* buf, int len, int fullLen) {
return
smagic(buf, "IMPM")
|| smagic(buf, "Extended Module")
|| smagic(buf+44, "SCRM")
|| smagic(buf, "MTM")
|| smagic(buf, "MO3")
|| smagic(buf, "AMF") || smagic(buf, "Extreme") || smagic(buf, "DBM0") || smagic(buf, "DDMF") || smagic(buf, "DMDL")
|| smagic(buf, "MMD") || smagic(buf, "MT20") || smagic(buf, "OKTA") || smagic(buf+44, "PTMF")
|| smagic(buf+20, "!SCREAM!")
|| hmagic(buf, 0x6669)  || hmagic(buf, 0x4e4a)
|| nmagic(buf, 0xFE524146) || nmagic(buf, 0x204d5350)
|| checkUMXHeader(buf, 1084)
|| smagic(buf, "MAS_UTrack_V00")
|| smagic(buf+1080, "M.K.")  || smagic(buf+1080, "M!K!") || smagic(buf+1080, "M&K!") || smagic(buf+1080, "N.T.") || smagic(buf+1080, "CD81")  || smagic(buf+1080, "OKTA")
|| cmagic1(buf+1080, "FLT", buf+1083)
|| cmagic1(buf+1081, "CHN", buf+1080)
|| cmagic2(buf+1082, "CH", buf+1080, buf+1081)
|| cmagic2(buf+1082, "CN", buf+1080, buf+1081)  
|| cmagic1(buf+1080, "TDZ", buf+1083)
|| openmpt_probe_file_header( OPENMPT_PROBE_FILE_HEADER_FLAGS_DEFAULT, buf, len, fullLen, NULL, NULL, NULL, NULL, NULL, NULL)
;//END
}

static int readFully (BASSFILE file, void* buffer, int length) {
int pos=0, read=0;
while(pos<length && (read=bassfunc->file.Read(file, buffer+pos, length-pos))>0) pos+=read;
return pos;
}

static HSTREAM WINAPI StreamCreateProc(BASSFILE file, DWORD flags) {
int problen = openmpt_probe_file_header_get_recommended_size();
if (problen<1084) problen=1084;
char* buffer = malloc(problen);
int firstlen = 0;
DWORD length = bassfunc->file.GetPos(file, BASS_FILEPOS_END);
if (!buffer || (firstlen=readFully(file, buffer, problen))<4 || !checkModHeaders(buffer, firstlen, length)) error(BASS_ERROR_FILEFORM);
buffer = realloc(buffer, length);
if (!buffer) error(BASS_ERROR_FILEFORM);
readFully(file, buffer+firstlen, length -firstlen);
if (tryMusicFirst) {
DWORD f = modFlags | (flags & modFlagsFwd);
if (!(f&BASS_SAMPLE_LOOP)) f|=BASS_MUSIC_STOPBACK; 
HMUSIC mod = BASS_MusicLoad(TRUE, buffer, 0, length, f, 0);
if (mod) {
BASS_CHANNELINFO info;
BASS_ChannelGetInfo(mod, &info);
//if (mod==BASS_CTYPE_MUSIC_XM) BASS_ChannelFlags(mod, BASS_MUSIC_FT2MOD, BASS_MUSIC_FT2MOD);
//else if (mod==BASS_CTYPE_MUSIC_MOD) BASS_ChannelFlags(mod, BASS_MUSIC_PT1MOD, BASS_MUSIC_PT1MOD);
bassfunc->file.Close(file);
free(buffer);
noerrorn(mod);
}}
openmpt_module_initial_ctl ctls[] = {
{ "seek.sync_samples", "1" },
{ "play.at_end", "continue" },
{ NULL, NULL }
};
openmpt_module* mod = openmpt_module_create_from_memory2(buffer, length, openmpt_log_func_silent, NULL, openmpt_error_func_default, NULL, NULL, NULL, ctls);
free(buffer);
if (!mod) error(BASS_ERROR_FILEFORM);
bassfunc->file.Close(file);
MPTStream* stream = malloc(sizeof(MPTStream));
memset(stream, 0, sizeof(MPTStream));
stream->mod = mod;
stream->useFloat = flags&BASS_SAMPLE_FLOAT;
	flags&=BASS_SAMPLE_FLOAT|BASS_SAMPLE_8BITS|BASS_SAMPLE_SOFTWARE|BASS_SAMPLE_LOOP|BASS_SAMPLE_3D|BASS_SAMPLE_FX 		|BASS_STREAM_DECODE|BASS_STREAM_AUTOFREE|0x3f000000; // 0x3f000000 = all speaker flags
	stream->handle=bassfunc->CreateStream(48000, 2, flags, &StreamProc, stream, &funcs);
	if (!stream->handle) { 
free(buffer);
MPTFree(stream);
		return 0;
	}
noerrorn(stream->handle);
}

HSTREAM WINAPI EXPORT BASS_OpenMPT_StreamCreateFile(BOOL mem, const void *file, QWORD offset, QWORD length, DWORD flags) {
	HSTREAM s;
	BASSFILE bfile =bassfunc->file.Open(mem,file,offset,length,flags,TRUE);
	if (!bfile) return 0; 
	s=StreamCreateProc(bfile,flags);
	if (!s) bassfunc->file.Close(bfile);
	return s;
}

HSTREAM WINAPI EXPORT BASS_OpenMPT_StreamCreateURL(const char *url, DWORD offset, DWORD flags, DOWNLOADPROC *proc, void *user) {
	HSTREAM s;
	BASSFILE bfile =bassfunc->file.OpenURL(url,offset,flags,proc,user,TRUE);
	if (!bfile) return 0; 
	s=StreamCreateProc(bfile,flags);
	if (!s) bassfunc->file.Close(bfile);
	return s;
}

HSTREAM WINAPI EXPORT BASS_OpenMPT_StreamCreateFileUser(DWORD system, DWORD flags, const BASS_FILEPROCS *procs, void *user) {
	HSTREAM s;
	BASSFILE bfile =bassfunc->file.OpenUser(system,flags,procs,user,TRUE);
	s=StreamCreateProc(bfile,flags);
	if (!s) bassfunc->file.Close(bfile);
	return s;
}

static QWORD WINAPI MPTGetLength(MPTStream* stream, DWORD mode) {
if (mode!=BASS_POS_BYTE) errorn(BASS_ERROR_NOTAVAIL); // only support byte positioning
	noerrorn(openmpt_module_get_duration_seconds(stream->mod) * 48000.0 * 2 * (stream->useFloat?sizeof(float) : sizeof(short))  );
}

static void WINAPI MPTGetInfo(MPTStream* stream, BASS_CHANNELINFO *info) {
info->freq = 48000;
info->chans = 2;
	info->ctype = BASS_CTYPE_MUSIC_OPENMPT;
info->origres = 16; 
}

static BOOL WINAPI MPTCanSetPosition(MPTStream *stream, QWORD pos, DWORD mode) {
	if ((BYTE)mode!=BASS_POS_BYTE) error(BASS_ERROR_NOTAVAIL); // only support byte positioning (BYTE = ignore flags)
double dpos = pos / (48000.0 * 2 * (stream->useFloat? sizeof(float) : sizeof(short)) );
	if (dpos>=openmpt_module_get_duration_seconds(stream->mod)) error(BASS_ERROR_POSITION);
	return TRUE;
}

static QWORD WINAPI MPTSetPosition(MPTStream* stream, QWORD pos, DWORD mode) {
double dpos = pos / (48000.0 * 2 * (stream->useFloat? sizeof(float) : sizeof(short)));
if (dpos<0 || dpos>=openmpt_module_get_duration_seconds(stream->mod)) errorn(BASS_ERROR_POSITION);
dpos = openmpt_module_set_position_seconds(stream->mod, dpos);
pos = dpos * (48000.0 * 2 * (stream->useFloat? sizeof(float) : sizeof(short)));
pos>>=4; pos<<=4;
return pos;
}

static const char* WINAPI MPTTags (MPTStream* stream, DWORD type) {
if (!stream || !stream->mod) return NULL;
static const char* str = NULL;
if (str) openmpt_free_string(str);
str = NULL;
switch(type){
case BASS_TAG_MUSIC_NAME: return str = openmpt_module_get_metadata(stream->mod, "title");
case BASS_TAG_MUSIC_AUTH: return str = openmpt_module_get_metadata(stream->mod, "artist");
case BASS_TAG_MUSIC_MESSAGE: return str = openmpt_module_get_metadata(stream->mod, "message_raw");
}
if (type>=BASS_TAG_MUSIC_INST && type<BASS_TAG_MUSIC_INST+openmpt_module_get_num_instruments(stream->mod)) {
return str = openmpt_module_get_instrument_name(stream->mod, type - BASS_TAG_MUSIC_INST);
}
if (type>=BASS_TAG_MUSIC_SAMPLE && type<BASS_TAG_MUSIC_SAMPLE+openmpt_module_get_num_samples(stream->mod)) {
return str = openmpt_module_get_sample_name(stream->mod, type-BASS_TAG_MUSIC_SAMPLE);
}
return NULL;
}

static const char* buildExtList () {
static char exts[4096] = {0};
char* mExts = openmpt_get_supported_extensions();
if (mExts) {
int n = 0;
char* ext = strtok(mExts, ";");
while(ext && *ext){
strcat(exts, *exts?";*.":"*.");
strcat(exts, ext);
ext = strtok(NULL, ";");
}
openmpt_free_string(mExts);
}
return exts;
}

const ADDON_FUNCTIONS funcs={
	0, // flags
	MPTFree,
	MPTGetLength,
MPTTags, 
	NULL, // let BASS handle file position
MPTGetInfo,
MPTCanSetPosition,
MPTSetPosition,
	NULL, // let BASS handle the position/looping/syncing (POS/END)
NULL, //	RAW_SetSync,
NULL, //	RAW_RemoveSync,
	NULL, // let BASS decide when to resume a stalled stream
	NULL, // no custom flags
	NULL // no attributes
};

static BASS_PLUGINFORM frm[] = { 
{ BASS_CTYPE_MUSIC_IT, "Impulse tracker module", "*.it" },
{ BASS_CTYPE_MUSIC_XM, "Extended module", "*.xm" },
{ BASS_CTYPE_MUSIC_S3M, "ScreamTracker3 Module", "*.s3m" },
{ BASS_CTYPE_MUSIC_MOD, "ProTracker Module", "*.mod" },
{ BASS_CTYPE_MUSIC_MTM, "MultiTracker module", "*.mtm;*.nst" },
{ BASS_CTYPE_MUSIC_MOD, "Unreal extended module", "*.umx" },
{ BASS_CTYPE_MUSIC_MO3, "BASS MP3 compressed module", "*.mo3" },
{ BASS_CTYPE_MUSIC_OPENMPT, "Module files supported by OpenMPT", NULL  }
};
static BASS_PLUGININFO plugininfo = {0x02040000, 8, frm };

const void* WINAPI EXPORT BASSplugin(DWORD face) {
switch (face) {
		case BASSPLUGIN_INFO:
if (!frm[7].exts) frm[7].exts = buildExtList();
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
//modPlugInit();
			break;
}
	return TRUE;
}
