#include "Loader.hpp"
#include "../common/stringUtils.hpp"
#include "../common/WXWidgets.hpp"
#include <wx/archive.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include<memory>
#include "../common/bass.h"
#include "../common/println.hpp"
using namespace std;

struct LoaderArchiveReader {
shared_ptr<wxArchiveInputStream> in;
long long size;

LoaderArchiveReader (shared_ptr<wxArchiveInputStream>& in1, long long sz): in(in1), size(sz)  {}
};

static void CALLBACK LARClose (void* ptr) {
auto r = reinterpret_cast<LoaderArchiveReader*>(ptr);
delete r;
}

static QWORD CALLBACK LARLen (void* ptr) {
auto& r = *reinterpret_cast<LoaderArchiveReader*>(ptr);
return r.size<=0? 0 : r.size;
}

static DWORD CALLBACK LARRead (void* buf, DWORD len, void* ptr) {
auto& r = *reinterpret_cast<LoaderArchiveReader*>(ptr);
r.in->Read(buf, len);
auto count = r.in->LastRead();
return count>0? count : -1;
}

static BOOL CALLBACK LARSeek (QWORD pos, void* ptr) {
return false;
}

static bool iszext (const string& s) {
char c = s[s.size() -1];
auto i = s.rfind('.');
return (c=='z' || c=='Z') 
&& (i>=s.size() -5) && (i!=string::npos) 
&& !iends_with(s, ".gz");
}

struct LoaderArchive: Loader {

LoaderArchive (): Loader("Archives", "", LF_FILE | LF_URL) {}
virtual unsigned long load (const std::string& url, unsigned long flags) final override {
wxLogNull logNull;
string entryName, archiveName;
const wxArchiveClassFactory* factory = nullptr;
if (starts_with(url, "zip://")) {
auto qm = url.find('?');
if (qm==string::npos) return 0;
archiveName = url.substr(6, qm -6);
entryName = url.substr(qm+1);
replace_all(entryName, "\\", "/");
factory = wxArchiveClassFactory::Find(U(archiveName), wxSTREAM_FILEEXT);
}
else if (iszext(url)) {
factory = wxArchiveClassFactory::Find(".zip", wxSTREAM_FILEEXT);
archiveName = url;
}

if (!factory || archiveName.empty()) return 0;
auto file = new wxFFileInputStream(U(archiveName));
if (!file || !file->IsOk()) return 0;
auto archive = shared_ptr<wxArchiveInputStream>(factory->NewStream(file));
if (!archive || !archive->IsOk()) return 0;

long long size = -1;
while(auto entry = shared_ptr<wxArchiveEntry>(archive->GetNextEntry())) {
if (entry->IsDir()) continue;
if (entryName.size()) {
string name = U(entry->GetName());
replace_all(name, "\\", "/");
if (!iequals(entryName, name)) continue;
}

size = entry->GetSize();
BASS_FILEPROCS procs = { LARClose, LARLen, LARRead, LARSeek };
auto lar = new LoaderArchiveReader(archive, size);
DWORD stream = BASS_StreamCreateFileUser(STREAMFILE_BUFFER, flags, &procs, lar);
if (stream) return stream;
else if (!entryName.empty()) break;
}
return 0;
}};

void ldrAddArchive () {
Loader::loaders.push_back(make_shared<LoaderArchive>());
}

