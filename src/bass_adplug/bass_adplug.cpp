#include "adplug.h"
#include "emuopl.h"
# include "binstr.h"
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
DWORD remaining;
DWORD lengthMs;
bool useFloat;
std::string title, author, description;

AdplugStream (
CPlayer* player0, CEmuopl* emuopl0, CFileProvider* fs0): 
player(player0), emuopl(emuopl0), fs(fs0), handle(0), remaining(0), lengthMs(0), useFloat(false)
 {}
~AdplugStream () {
delete player;
delete emuopl;
delete fs;
}

};//AdplugStream

struct BufferFileProvider: CFileProvider {
char* buffer;
int length;
BufferFileProvider (char* b, int l): buffer(b), length(l) {}
virtual ~BufferFileProvider () { delete buffer; }
  virtual binistream *open(std::string) const {
return new binisstream(buffer, length);
}
  virtual void close(binistream *b) const {
if (b) delete b;
}
};

extern const ADDON_FUNCTIONS funcs;

static void WINAPI AdplugFree (AdplugStream  *stream) {
if (!stream) return;
delete stream;
}

static DWORD CALLBACK StreamProc(HSTREAM handle, BYTE *buffer, DWORD length, AdplugStream *stream) {
if (stream->remaining<=0) {
if (!stream->player->update()) {
stream->player->rewind();
return BASS_STREAMPROC_END;
}
stream->remaining = 4 * 48000/stream->player->getrefresh();
}
DWORD mult = stream->useFloat? 2 : 1;
DWORD n = std::min(length/mult, stream->remaining);
stream->remaining -= n;
stream->emuopl->update( (short*)buffer, n/4);
if (stream->useFloat) bassfunc->data.Int2Float(buffer, (float*)buffer, length/mult, 2);
return n*mult;
}

static int readFully (BASSFILE file, void* buffer, int length) {
int pos=0, read=0;
while(pos<length && (read=bassfunc->file.Read(file, buffer+pos, length-pos))>0) pos+=read;
return pos;
}

static HSTREAM WINAPI StreamCreateProc(BASSFILE file, DWORD flags) {
int unicode = 0;
void* fnptr = bassfunc->file.GetFileName(file, &unicode);
if (!fnptr) error(BASS_ERROR_FILEFORM);
DWORD length = bassfunc->file.GetPos(file, BASS_FILEPOS_END);
if (length<=0 || length>=2*1024*1024) error(BASS_ERROR_FILEFORM);
char* buffer = new char[length];
if (!buffer) error(BASS_ERROR_FILEFORM);
readFully(file, buffer, length);

std::string filename;
if (unicode) {
auto sptr = reinterpret_cast<const unsigned short*>(fnptr);
while(sptr&&*sptr) filename += (char)*(sptr++);
}
else filename = static_cast<const char*>(fnptr);

CFileProvider* fsProvider = new BufferFileProvider(buffer, length);
CEmuopl* emuopl = new CEmuopl(48000, true, true);
CPlayer* player = CAdPlug::factory(filename, emuopl, CAdPlug::players, *fsProvider);
if (!player) {
delete emuopl;
delete fsProvider;
error(BASS_ERROR_FILEFORM);
}
bassfunc->file.Close(file);
AdplugStream* stream = new AdplugStream(player, emuopl, fsProvider);
flags &= BASS_SAMPLE_FLOAT|BASS_SAMPLE_SOFTWARE|BASS_SAMPLE_LOOP|BASS_SAMPLE_3D|BASS_SAMPLE_FX 		|BASS_STREAM_DECODE|BASS_STREAM_AUTOFREE|0x3f000000; // 0x3f000000 = all speaker flags
stream->useFloat = (flags&BASS_SAMPLE_FLOAT);
stream->lengthMs = player->songlength();
stream->title = player->gettitle();
stream->author = player->getauthor();
stream->description = player->getdesc();
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
double dpos = pos / (48000.0 * 2 * (stream->useFloat? sizeof(float) : sizeof(short)) );
if (dpos>=stream->lengthMs*1000) error(BASS_ERROR_POSITION);
	return TRUE;
}

static QWORD WINAPI AdplugSetPosition(AdplugStream* stream, QWORD pos, DWORD mode) {
double dpos = pos / (48000.0 * 2 * (stream->useFloat? sizeof(float) : sizeof(short)));
if (dpos<0 || dpos>=stream->lengthMs*1000) errorn(BASS_ERROR_POSITION);
stream->player->seek(dpos*1000);
pos = dpos * (48000.0 * 2 * (stream->useFloat? sizeof(float) : sizeof(short)));
pos>>=4; pos<<=4;
return pos;
}

static const char* WINAPI AdplugTags (AdplugStream* stream, DWORD type) {
if (!stream) return NULL;
switch(type){
case BASS_TAG_MUSIC_NAME: return stream->title.c_str();
case BASS_TAG_MUSIC_AUTH: return stream->author.c_str();
case BASS_TAG_MUSIC_MESSAGE: return stream->description.c_str();
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
			break;
}
	return TRUE;
}
