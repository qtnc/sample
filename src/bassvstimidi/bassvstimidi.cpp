#include "VstiSequencer.hpp"
#include "../common/bass.h"
#include "../common/bass-addon.h"
#include<sstream>
#include<fstream>
#ifndef bassfunc
const BASS_FUNCTIONS *bassfunc=NULL;
#endif
using namespace std;

#define MThd 0x6468544D

#define BASS_CTYPE_STREAM_VSTIMIDI 0x10D01

#define BASS_CONFIG_VSTIMIDI_VST_FILE 0x10800

#define EXPORT __declspec(dllexport)

extern const ADDON_FUNCTIONS funcs;

static std::string vstDll;

static BOOL WINAPI VMConfig (DWORD opt, DWORD flags, void* val) {
if (flags&BASSCONFIG_SET) switch(opt){
case BASS_CONFIG_VSTIMIDI_VST_FILE:
if (val) vstDll = reinterpret_cast<const char*>(val);
return !!val;
}
else switch (opt){
case BASS_CONFIG_VSTIMIDI_VST_FILE:
*reinterpret_cast<char const**>(val) = vstDll.data();
return TRUE;
}
return FALSE;
}

static void WINAPI VMFree (void* vm) {
if (!vm) return;
delete static_cast<VstiSequencer*>(vm);
}

static DWORD CALLBACK StreamProc(HSTREAM handle, void* buffer, DWORD length, void* vm) {
return static_cast<VstiSequencer*>(vm) ->Decode(buffer, length);
}

static int readFully (BASSFILE file, void* buffer, int length) {
int pos=0, read=0;
while(pos<length && (read=bassfunc->file.Read(file, buffer+pos, length-pos))>0) pos+=read;
return pos;
}

static inline BOOL LoadMIDIFile (VstiSequencer* vsti, BASSFILE file) {
char* buffer = static_cast<char*>( malloc(4));
if (!buffer || readFully(file, buffer, 4)<4 || *reinterpret_cast<DWORD*>(buffer)!=MThd) {
free(buffer);
error(BASS_ERROR_FILEFORM); 
}
DWORD length = bassfunc->file.GetPos(file, BASS_FILEPOS_END);
buffer = static_cast<char*>( realloc(buffer, length));
if (!buffer) error(BASS_ERROR_FILEFORM);
if (readFully(file, buffer+4, length -4)<=0) {
free(buffer);
error(BASS_ERROR_FILEFORM); 
}
std::string s(buffer, buffer+length);
std::istringstream in(s, std::ios::binary);
free(buffer);
if (!vsti->LoadMIDI(in)) error(BASS_ERROR_FILEFORM);
noerrorn(TRUE);
}

static HSTREAM CreateStream (VstiSequencer* vsti, BASSFILE file, DWORD flags) {
	flags&=BASS_SAMPLE_FLOAT|BASS_SAMPLE_8BITS|BASS_SAMPLE_SOFTWARE|BASS_SAMPLE_LOOP|BASS_SAMPLE_3D|BASS_SAMPLE_FX 		|BASS_STREAM_DECODE|BASS_STREAM_AUTOFREE|0x3f000000; // 0x3f000000 = all speaker flags
HSTREAM handle = bassfunc->CreateStream(vsti->freq, (flags&BASS_SAMPLE_MONO)? 1 : 2, flags, &StreamProc, vsti, &funcs);
if (!handle) return 0;
	bassfunc->file.Close(file);
noerrorn(handle);
}

static HSTREAM WINAPI StreamCreateProc(BASSFILE file, DWORD flags) {
//flags |= BASS_SAMPLE_FLOAT;
VstiSequencer* vsti = new VstiSequencer(48000, flags);
DWORD handle = 0;
if (!LoadMIDIFile(vsti, file) 
|| !vsti->CreateVSTI(vstDll)
|| !(handle = CreateStream(vsti, file, flags))
) {
delete vsti;
}
return handle;
}

extern "C" export HSTREAM WINAPI BASS_VSTIMIDI_StreamCreateFile(BOOL mem, const void *file, QWORD offset, QWORD length, DWORD flags) {
	HSTREAM s;
	BASSFILE bfile =bassfunc->file.Open(mem,file,offset,length,flags,TRUE);
	if (!bfile) return 0; 
	s=StreamCreateProc(bfile,flags);
	if (!s) bassfunc->file.Close(bfile);
	return s;
}

extern "C" export HSTREAM WINAPI BASS_VSTIMIDI_StreamCreateURL(const char *url, DWORD offset, DWORD flags, DOWNLOADPROC *proc, void *user) {
	HSTREAM s;
	BASSFILE bfile =bassfunc->file.OpenURL(url,offset,flags,proc,user,TRUE);
	if (!bfile) return 0; 
	s=StreamCreateProc(bfile,flags);
	if (!s) bassfunc->file.Close(bfile);
	return s;
}

extern "C"  export HSTREAM WINAPI BASS_VSTIMIDI_StreamCreateFileUser(DWORD system, DWORD flags, const BASS_FILEPROCS *procs, void *user) {
	HSTREAM s;
	BASSFILE bfile =bassfunc->file.OpenUser(system,flags,procs,user,TRUE);
	s=StreamCreateProc(bfile,flags);
	if (!s) bassfunc->file.Close(bfile);
	return s;
}

static QWORD WINAPI VMGetLength(void* vm, DWORD mode) {
if (mode!=BASS_POS_BYTE) errorn(BASS_ERROR_NOTAVAIL); // only support byte positioning
VstiSequencer* vsti = static_cast<VstiSequencer*>(vm);
	noerrorn( vsti->midi.events.back().time );
}

static void WINAPI VMGetInfo(void* vm, BASS_CHANNELINFO *info) {
VstiSequencer* vsti = static_cast<VstiSequencer*>(vm);
info->freq = vsti->freq;
info->chans = (vsti->flags&BASS_SAMPLE_MONO)? 1 : 2;
	info->ctype = BASS_CTYPE_STREAM_VSTIMIDI;
info->origres = 16;
}

static BOOL WINAPI VMCanSetPosition(void* vm, QWORD pos, DWORD mode) {
	if ((BYTE)mode!=BASS_POS_BYTE) error(BASS_ERROR_NOTAVAIL); // only support byte positioning (BYTE = ignore flags)
QWORD length = VMGetLength(vm, mode);
	if (pos>length) error(BASS_ERROR_POSITION);
	return TRUE;
}

static QWORD WINAPI VMSetPosition(void* vm, QWORD pos, DWORD mode) {
if (!VMCanSetPosition(vm, pos, mode)) return 0;
static_cast<VstiSequencer*>(vm) ->Seek(pos);
return pos;
}

static const char* WINAPI VMTags (void* vm, DWORD type) {
return nullptr;
}

const ADDON_FUNCTIONS funcs={
	0, // flags
	VMFree,
	VMGetLength,
VMTags, 
	NULL, // let BASS handle file position
	VMGetInfo,
	VMCanSetPosition,
	VMSetPosition,
	NULL, // let BASS handle the position/looping/syncing (POS/END)
NULL, //	RAW_SetSync,
NULL, //	RAW_RemoveSync,
	NULL, // let BASS decide when to resume a stalled stream
	NULL, // no custom flags
	NULL // no attributes
};

static const BASS_PLUGINFORM frm[] = { 
{ BASS_CTYPE_STREAM_VSTIMIDI, "MIDI", "*.mid;*.midi;*.kar" },
};
static BASS_PLUGININFO plugininfo = {0x02040000, 1, frm };

extern "C" const void* WINAPI EXPORT BASSplugin(DWORD face) {
switch (face) {
		case BASSPLUGIN_INFO:
			return (void*)&plugininfo;
		case BASSPLUGIN_CREATE:
			return (void*)StreamCreateProc;
	}
	return NULL;
}

extern "C" BOOL WINAPI EXPORT DllMain(HANDLE hDLL, DWORD reason, LPVOID reserved)
{
	switch (reason) {
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls((HMODULE)hDLL);
			if (HIWORD(BASS_GetVersion())!=BASSVERSION || !GetBassFunc()) {
				MessageBoxA(0,"Incorrect BASS.DLL version (" BASSVERSIONTEXT " is required)", "BASS", MB_ICONERROR | MB_OK);
				return FALSE;
			}
bassfunc->RegisterPlugin((void*)&VMConfig, PLUGIN_CONFIG_ADD);
			break;
}
	return TRUE;
}
