#include "Loader.hpp"
#include "cpprintf.hpp"
#include "stringUtils.hpp"
#include "WXWidgets.hpp"
#include <wx/wfstream.h>
#include <wx/zstream.h>
#include<memory>
#include "bass.h"
using namespace std;

struct LoaderZlibReader {
unique_ptr<wxInputStream> in;
LoaderZlibReader (unique_ptr<wxInputStream>&& in1): in(move(in1)) {}
};

static void CALLBACK LZRClose (void* ptr) {
auto r = reinterpret_cast<LoaderZlibReader*>(ptr);
delete r;
}

static QWORD CALLBACK LZRLen (void* ptr) {
return 0;
}

static DWORD CALLBACK LZRRead (void* buf, DWORD len, void* ptr) {
auto& r = *reinterpret_cast<LoaderZlibReader*>(ptr);
r.in->Read(buf, len);
auto count = r.in->LastRead();
return count? count : -1;
}

static BOOL CALLBACK LZRSeek (QWORD pos, void* ptr) {
return false;
}

struct LoaderZlib: Loader {

LoaderZlib () = default;
virtual unsigned long load (const std::string& filename, unsigned long flags) final override {
auto fs = new wxFFileInputStream(U(filename));
if (!fs) return 0;
auto zs = make_unique<wxZlibInputStream>(fs);
if (!zs) return 0;
BASS_FILEPROCS procs = { LZRClose, LZRLen, LZRRead, LZRSeek };
auto eptr = new LoaderZlibReader(move(zs));
return BASS_StreamCreateFileUser(STREAMFILE_BUFFER, flags, &procs, eptr);
}};

void ldrAddZlib () {
Loader::loaders.push_back(make_shared<LoaderZlib>());
}

