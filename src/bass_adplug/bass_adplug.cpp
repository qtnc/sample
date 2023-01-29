#include "adplug.h"
#include "emuopl.h"
#include "../common/bass.h"
#include "../common/bass-addon.h"
#ifndef bassfunc
const BASS_FUNCTIONS *bassfunc=NULL;
#endif

#define BASS_CTYPE_MUSIC_ADPLUG 0x20011

#define EXPORT __declspec(dllexport)

struct AdplugStream {
CPlayer* player;
CEmuopl* emuopl;
CFileProvider* fs;
DWORD handle;

AdplugStream (
CPlayer* player0, CEmuopl* emuopl0, CFileProvider* fs0): 
player(player0), emuopl(emuopl0), fs(fs0), handle(0)
 {}
~AdplugStream () {
delete player;
delete emuopl;
delete fs;
}
};

extern const ADDON_FUNCTIONS funcs;

static void WINAPI AdplugFree (AdplugStream  *stream) {
if (!stream) return;
delete stream;
}

static DWORD CALLBACK StreamProc(HSTREAM handle, BYTE *buffer, DWORD length, AdplugStream *stream) {
//##todo
return length;
}

static int readFully (BASSFILE file, void* buffer, int length) {
int pos=0, read=0;
while(pos<length && (read=bassfunc->file.Read(file, buffer+pos, length-pos))>0) pos+=read;
return pos;
}

static HSTREAM WINAPI StreamCreateProc(BASSFILE file, DWORD flags) {
std::string filename;
CFileProvider* fsProvider = nullptr;
CEmuopl* emuopl = new CEmuopl(48000, true, true);
CPlayer* player = CAdPlug::factory(filename, emuopl, CAdPlug::players, *fsProvider);
if (!player) {
delete emuopl;
delete fsProvider;
error(BASS_ERROR_FILEFORM);
}
/*if (!buffer || readFully(file, buffer, 1084)<4 || !checkModHeaders(buffer, 1084)) error(BASS_ERROR_FILEFORM);
DWORD length = bassfunc->file.GetPos(file, BASS_FILEPOS_END);
buffer = realloc(buffer, length);
if (!buffer) error(BASS_ERROR_FILEFORM);
readFully(file, buffer+1084, length -1084);*/
bassfunc->file.Close(file);
AdplugStream* stream = new AdplugStream(player, emuopl, fsProvider);
flags &= BASS_SAMPLE_SOFTWARE|BASS_SAMPLE_LOOP|BASS_SAMPLE_3D|BASS_SAMPLE_FX 		|BASS_STREAM_DECODE|BASS_STREAM_AUTOFREE|0x3f000000; // 0x3f000000 = all speaker flags
	stream->handle=bassfunc->CreateStream(48000, 2, flags, &StreamProc, stream, &funcs);
	if (!stream->handle) { 
delete stream;
		return 0;
	}
noerrorn(stream->handle);
}

extern "C" HSTREAM WINAPI EXPORT BASS_Adplug_StreamCreateFile(BOOL mem, const void *file, QWORD offset, QWORD length, DWORD flags) {
	HSTREAM s;
	BASSFILE bfile =bassfunc->file.Open(mem,file,offset,length,flags,TRUE);
	if (!bfile) return 0; 
	s=StreamCreateProc(bfile,flags);
	if (!s) bassfunc->file.Close(bfile);
	return s;
}

extern "C" HSTREAM WINAPI EXPORT BASS_Adplug_StreamCreateURL(const char *url, DWORD offset, DWORD flags, DOWNLOADPROC *proc, void *user) {
	HSTREAM s;
	BASSFILE bfile =bassfunc->file.OpenURL(url,offset,flags,proc,user,TRUE);
	if (!bfile) return 0; 
	s=StreamCreateProc(bfile,flags);
	if (!s) bassfunc->file.Close(bfile);
	return s;
}

extern "C" HSTREAM WINAPI EXPORT BASS_Adplug_StreamCreateFileUser(DWORD system, DWORD flags, const BASS_FILEPROCS *procs, void *user) {
	HSTREAM s;
	BASSFILE bfile =bassfunc->file.OpenUser(system,flags,procs,user,TRUE);
	s=StreamCreateProc(bfile,flags);
	if (!s) bassfunc->file.Close(bfile);
	return s;
}

static QWORD WINAPI AdplugGetLength(AdplugStream* stream, DWORD mode) {
if (mode!=BASS_POS_BYTE) errorn(BASS_ERROR_NOTAVAIL); // only support byte positioning
//### todo
	noerrorn(0);
}

static void WINAPI AdplugGetInfo(AdplugStream* stream, BASS_CHANNELINFO *info) {
info->freq = 48000;
info->chans = 2;
	info->ctype = BASS_CTYPE_MUSIC_ADPLUG;
info->origres = 16; 
}

static BOOL WINAPI AdplugCanSetPosition(AdplugStream *stream, QWORD pos, DWORD mode) {
	if ((BYTE)mode!=BASS_POS_BYTE) error(BASS_ERROR_NOTAVAIL); // only support byte positioning (BYTE = ignore flags)
//###todo
//double dpos = pos / (48000.0 * 2 * (stream->useFloat? sizeof(float) : sizeof(short)) );
//	if (dpos>=openmpt_module_get_duration_seconds(stream->mod)) error(BASS_ERROR_POSITION);
	return TRUE;
}

static QWORD WINAPI AdplugSetPosition(AdplugStream* stream, QWORD pos, DWORD mode) {
//###todo
//double dpos = pos / (48000.0 * 2 * (stream->useFloat? sizeof(float) : sizeof(short)));
//if (dpos<0 || dpos>=openmpt_module_get_duration_seconds(stream->mod)) errorn(BASS_ERROR_POSITION);
//dpos = openmpt_module_set_position_seconds(stream->mod, dpos);
//pos = dpos * (48000.0 * 2 * (stream->useFloat? sizeof(float) : sizeof(short)));
//pos>>=4; pos<<=4;
return pos;
}

static const char* WINAPI AdplugTags (AdplugStream* stream, DWORD type) {
if (!stream) return NULL;
switch(type){
//case BASS_TAG_MUSIC_NAME: return str = openmpt_module_get_metadata(stream->mod, "title");
//case BASS_TAG_MUSIC_AUTH: return str = openmpt_module_get_metadata(stream->mod, "artist");
//case BASS_TAG_MUSIC_MESSAGE: return str = openmpt_module_get_metadata(stream->mod, "message_raw");
}
//if (type>=BASS_TAG_MUSIC_INST && type<BASS_TAG_MUSIC_INST+openmpt_module_get_num_instruments(stream->mod)) {
//return str = openmpt_module_get_instrument_name(stream->mod, type - BASS_TAG_MUSIC_INST);
//}
//if (type>=BASS_TAG_MUSIC_SAMPLE && type<BASS_TAG_MUSIC_SAMPLE+openmpt_module_get_num_samples(stream->mod)) {
//return str = openmpt_module_get_sample_name(stream->mod, type-BASS_TAG_MUSIC_SAMPLE);
//}
return NULL;
}

const ADDON_FUNCTIONS funcs={
	0, // flags
AdplugFree,
AdplugGetLength,
AdplugTags, 
	NULL, // let BASS handle file position
AdplugGetInfo,
AdplugCanSetPosition,
AdplugSetPosition,
	NULL, // let BASS handle the position/looping/syncing (POS/END)
NULL, //	RAW_SetSync,
NULL, //	RAW_RemoveSync,
	NULL, // let BASS decide when to resume a stalled stream
	NULL, // no custom flags
	NULL // no attributes
};

static const BASS_PLUGINFORM frm[] = { 
{ BASS_CTYPE_MUSIC_ADPLUG, "AdPlug", "*.a2m;*.adl;*.agd;*.amd;*.bam;*.bmf;*.cff;*.cmf;*.d00;*.dfm;*.dmo;*.dro;*.dtm;*.got;*.ha2;*.hsc;*.hsp;*.hsq;*.imf;*.ims;*.ksm;*.laa;*.lds;*.m;*.mad;*.mdi;*.mkj;*.msc;*.mtk;*.mus;*.rad;*.raw;*.rix;*.rol;*.sa2;*.sat;*.sci;*.sdb;*.sng;*.sop;*.sqx;*.vgm;*.xad;*.xms;*.xsm" },
};
static BASS_PLUGININFO plugininfo = {0x02040A00, 1, frm };

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
