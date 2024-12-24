#include "Loader.hpp"
#include "../common/stringUtils.hpp"
#include "archive.h"
#include "archive_entry.h"
#include<memory>
#include "../common/bass.h"
#include "../common/println.hpp"
using namespace std;

typedef std::shared_ptr<archive> archive_ptr;

struct LoaderArchiveReader {
archive_ptr ar;
long long size;

LoaderArchiveReader (const archive_ptr& in1, long long sz): ar(in1), size(sz)  {}
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
auto count = archive_read_data(r.ar.get(), buf, len);
return count>0? count : -1;
}

static BOOL CALLBACK LARSeek (QWORD pos, void* ptr) {
return false;
}

struct LoaderArchive: Loader {

LoaderArchive (): Loader("Archive files", "", LF_FILE | LF_URL) {}
virtual unsigned long load (const std::string& url, unsigned long flags) final override {
if (!starts_with(url, "archive://")) return 0;
auto qm = url.find('?');
if (qm==string::npos) return 0;
auto archiveName = url.substr(10, qm -10);
auto entryName = url.substr(qm+1);
replace_all(entryName, "\\", "/");

if (archiveName.empty()) return 0;

auto ar = archive_ptr(archive_read_new(), &archive_read_free);
archive_entry* entry = nullptr;
long long size = -1;

if (!ar) goto end;
if (archive_read_support_format_all(ar.get())) goto end;
if (archive_read_support_filter_all(ar.get())) goto end;
if (archive_read_open_filename(ar.get(), archiveName.c_str(), 16384)) goto end;

while (!archive_read_next_header(ar.get(), &entry)) {
if (!entry) break;
auto ename = archive_entry_pathname_utf8(entry);
if (!ename || !*ename) break;;
string name = ename;
replace_all(name, "\\", "/");
if (!iequals(entryName, name)) continue;

size = archive_entry_size_is_set(entry)? archive_entry_size(entry) : 0;
BASS_FILEPROCS procs = { LARClose, LARLen, LARRead, LARSeek };
auto lar = new LoaderArchiveReader(ar, size);
DWORD stream = BASS_StreamCreateFileUser(STREAMFILE_BUFFER, flags, &procs, lar);
if (stream) return stream;
}
end: return 0;
}};

void ldrAddArchive () {
Loader::loaders.push_back(make_shared<LoaderArchive>());
}

