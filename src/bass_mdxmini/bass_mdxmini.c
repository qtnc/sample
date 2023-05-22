#include <stdio.h>
#include <string.h>
#include "mdxmini.h"
#include "../common/bass.h"
#include "../common/bass-addon.h"
#ifndef bassfunc
const BASS_FUNCTIONS *bassfunc=NULL;
#endif

#define BASS_CTYPE_MUSIC_MDX 0x20014
#define BASS_POS_SUBSONG 6

#define MAX_BUFFER_LENGTH 1024

#define EXPORT __declspec(dllexport)

typedef struct {
HSTREAM handle;
t_mdxmini mdx;
BOOL useFloat;
QWORD length;
} MDXStream;

extern const ADDON_FUNCTIONS funcs;

static void WINAPI MDXFree (MDXStream  *stream) {
if (!stream) return;
mdx_close(&stream->mdx);
free(stream);
}

static DWORD CALLBACK StreamProc(HSTREAM handle, BYTE *buffer, DWORD totalLength, MDXStream *stream) {
DWORD pos = 0;
while(pos<totalLength){
DWORD length = totalLength - pos;
if (length>MAX_BUFFER_LENGTH) length=MAX_BUFFER_LENGTH;
if (stream->useFloat) {
mdx_calc_sample(&stream->mdx, (short*)(buffer+pos), length/8);
bassfunc->data.Int2Float(buffer+pos, (float*)(buffer+pos), length/4, 2);
}
else {
mdx_calc_sample(&stream->mdx, (short*)(buffer+pos), length/4);
}
pos += length;
}
return totalLength;
}

static char* getFileName (BASSFILE file, char* buffer) {
int unicode = 0;
const void* fnptr = bassfunc->file.GetFileName(file, &unicode);
if (!fnptr) return buffer;
if (!unicode) return strcpy(buffer, fnptr);
for (char *c = fnptr, *d = buffer; *c; ) *(d++) = *(c++);
return buffer;
}

static char* getFileDir (const char* filename, char* filedir) {
if (!filename || !*filename) return filedir;
strcpy(filedir, filename);
char *c = filedir + strlen(filedir);
while(c>=filedir && c!='/' && c!='\\') c--;
if (c>filedir) *(++c)=0;
return filedir;
}

static HSTREAM WINAPI StreamCreateProc(BASSFILE file, DWORD flags) {
char filename[512] = {0};
char filedir[512]={0};
getFileName(file, filename);
getFileDir(filename, filedir);
if (!*filename) error(BASS_ERROR_FILEFORM);
if (!strstr(filename, ".mdx") && !strstr(filename, ".MDX")) error(BASS_ERROR_FILEFORM);
MDXStream* stream = malloc(sizeof(MDXStream));
if (!stream) error(BASS_ERROR_FILEFORM);
memset(stream, 0, sizeof(MDXStream));
mdx_set_rate(48000);
if (mdx_open(&stream->mdx, filename, filedir)) {
free(stream);
error(BASS_ERROR_FILEFORM);
}

	flags&=BASS_SAMPLE_FLOAT|BASS_SAMPLE_SOFTWARE|BASS_SAMPLE_LOOP|BASS_SAMPLE_3D|BASS_SAMPLE_FX 		|BASS_STREAM_DECODE|BASS_STREAM_AUTOFREE|0x3f000000; // 0x3f000000 = all speaker flags
	stream->handle=bassfunc->CreateStream(48000, 2, flags, &StreamProc, stream, &funcs);
	if (!stream->handle) { 
MDXFree(stream);
		return 0;
	}
stream->length = mdx_get_length(&stream->mdx) * 48000 * (stream->useFloat? 8 : 4);
bassfunc->file.Close(file);
noerrorn(stream->handle);
}

HSTREAM WINAPI EXPORT BASS_MDX_StreamCreateFile(BOOL mem, const void *file, QWORD offset, QWORD length, DWORD flags) {
	HSTREAM s;
	BASSFILE bfile =bassfunc->file.Open(mem,file,offset,length,flags,TRUE);
	if (!bfile) return 0; 
	s=StreamCreateProc(bfile,flags);
	if (!s) bassfunc->file.Close(bfile);
	return s;
}

static QWORD WINAPI MDXGetLength(MDXStream* stream, DWORD mode) {
if (mode!=BASS_POS_BYTE) error(BASS_ERROR_NOTAVAIL);
return stream->length;
}

static BOOL WINAPI MDXCanSetPosition(MDXStream *stream, QWORD pos, DWORD mode) {
error(BASS_ERROR_POSITION);
}

static QWORD WINAPI MDXSetPosition(MDXStream* stream, QWORD pos, DWORD mode) {
error(BASS_ERROR_POSITION);
}

static void WINAPI MDXGetInfo(MDXStream* stream, BASS_CHANNELINFO *info) {
info->freq = 48000;
info->chans = 2;
	info->ctype = BASS_CTYPE_MUSIC_MDX;
info->origres = 16; 
}

static const char* WINAPI MDXTags (MDXStream* stream, DWORD type) {
if (!stream) return NULL;
return NULL;
}

const ADDON_FUNCTIONS funcs={
0, // flags
MDXFree,
MDXGetLength,
MDXTags, 
	NULL, // let BASS handle file position
MDXGetInfo,
MDXCanSetPosition,
MDXSetPosition,
NULL, // GetPosition
NULL, //	RAW_SetSync,
NULL, //	RAW_RemoveSync,
	NULL, // let BASS decide when to resume a stalled stream
	NULL, // no custom flags
NULL
};

static const BASS_PLUGINFORM frm[] = { 
{ BASS_CTYPE_MUSIC_MDX, "MDXMini", "*.mdx" }
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
