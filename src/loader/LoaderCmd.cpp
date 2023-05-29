#include "Loader.hpp"
#include "../common/stringUtils.hpp"
#include "../common/WXWidgets.hpp"
#include <wx/wfstream.h>
#include <wx/zstream.h>
#include<wx/process.h>
#include <wx/buffer.h>
#include <wx/filename.h>
#include <wx/thread.h>
#include "../common/bass.h"
#include "../app/App.hpp"
#include <fmt/format.h>
#include<memory>
#include "../common/println.hpp"
using namespace std;

struct LoaderCommandReaderThread: wxThread {
struct LoaderCommandReader& r;
LoaderCommandReaderThread (LoaderCommandReader& r0): r(r0) {}
virtual wxThread::ExitCode Entry () override;
};

struct LoaderCommandReader {
wxMemoryBuffer buffer;
long long bufpos;
wxInputStream* in;
wxProcess* proc;
wxCriticalSection lock;


void close () {
//println("Close called");
wxCriticalSectionLocker locker(lock);
//delete in;
//delete proc;
//in = nullptr;
//proc = nullptr;
}

LoaderCommandReader (const wxString& cmd): proc(nullptr), in(nullptr), buffer(16384), bufpos(0), lock() {
wxCriticalSectionLocker locker(lock);
//println("Opening process");
proc = wxProcess::Open(cmd, wxEXEC_ASYNC | wxEXEC_HIDE_CONSOLE);
if (!proc) return;
in = proc->GetInputStream();
proc->Bind(wxEVT_END_PROCESS, [this](auto&e){ 
//println("Process ended"); 
close(); 
});
//println("Starting thread");
(new LoaderCommandReaderThread(*this)) ->Run();
}

bool waitAvailable () {
long long len = 0;
do {
wxCriticalSectionLocker locker(lock);
len = buffer.GetDataLen();
if (!len && in && proc) Sleep(1);
} while (!len && in && proc);
return in && proc;
}

long long read (void* out, long long max) {
wxCriticalSectionLocker locker(lock);
const void* bufptr = buffer.GetData();
long long len = buffer.GetDataLen();
long long count = std::min(len-bufpos, max);
if (count>0) memcpy(out, reinterpret_cast<const char*>(bufptr) + bufpos, count);
bufpos += count;
//println("Read: pos={}, max={}, len={}, count={}", bufpos, max, len, count);
return count;
}

long long getLen () {
wxCriticalSectionLocker locker(lock);
return buffer.GetDataLen();
}

bool seek (long long pos) {
wxCriticalSectionLocker locker(lock);
long long len = buffer.GetDataLen();
if (pos>len) return false;
bufpos = pos;
return true;
}

bool doReadChunk () {
wxCriticalSectionLocker locker(lock);
if (!in || !proc) return false;
int len = 4096;
char buf[len];
in->Read(buf, len);
auto count = in->LastRead();
if (count>0) buffer.AppendData(buf, count);
//println("Chunk: read={}, max={}, curlen={}", count, len, buffer.GetDataLen());
return count>0;
}

~LoaderCommandReader () { 
//println("Destructor called"); 
close(); 
}

};

wxThread::ExitCode LoaderCommandReaderThread::Entry () {
//println("Thread started");
while (!TestDestroy() && r.doReadChunk() );
//println("Reading thread finished reading");
//while (!TestDestroy()) Sleep(1);
//println("Thread ended");
return 0;
}

static void CALLBACK LCRClose (void* ptr) {
//println("LCRClose called");
auto r = reinterpret_cast<LoaderCommandReader*>(ptr);
r->close();
}

static QWORD CALLBACK LCRLen (void* ptr) {
return 0; 
//auto r = reinterpret_cast<LoaderCommandReader*>(ptr);
//return r->getLen();
}

static DWORD CALLBACK LCRRead (void* buf, DWORD len, void* ptr) {
auto r = reinterpret_cast<LoaderCommandReader*>(ptr);
if (!r) return -1;
auto count = r->read(buf, len);
//println("LCRRead called, len={}, count={}", len, count);
return count? count : -1;
}

static BOOL CALLBACK LCRSeek (QWORD pos, void* ptr) {
//println("LCRSeek called {}", pos);
return false;
//auto r = reinterpret_cast<LoaderCommandReader*>(ptr);
//return r->seek(pos);
}

struct LoaderCmd: Loader {
std::string cmd;

LoaderCmd (const std::string& desc, const std::string& ext, int fl, const std::string& c): Loader(desc, ext, fl), cmd(c) {}
virtual unsigned long load (const std::string& filename, unsigned long flags) final override {
wxLogNull logNull;
size_t dot = filename.find(':');
if ((dot<2 || dot==std::string::npos) && (!wxFileName::FileExists(U(filename)) || wxFileName::DirExists(U(filename)))) return 0;
std::string cmdline = fmt::format(cmd, filename);
//println("Filename={}, cmd={}", filename, cmdline);
auto proc = new LoaderCommandReader(U(cmdline));
if (!proc || !proc->proc || !proc->in) {
delete proc;
return 0;
}
if (!proc->waitAvailable()) {
delete proc;
return 0;
}
BASS_FILEPROCS procs = { LCRClose, LCRLen, LCRRead, LCRSeek };
//println("Creating stream");
auto stream = BASS_StreamCreateFileUser(STREAMFILE_BUFFER, flags, &procs, proc);
//println("Created BASS stream: {:#x}", stream);
return stream;
}};

void ldrAddFfmpeg () {
//Loader::loaders.push_back(make_shared<LoaderCmd>("FFMPEG", "", LF_FILE | LF_URL, "ffmpeg.exe -i \"{}\" -vn -f wav"));
}

void ldrAddYtdlp () {
Loader::loaders.push_back(make_shared<LoaderCmd>("YT-DLP", "", LF_URL, "yt-dlp.exe \"{}\" -o -"));
}


