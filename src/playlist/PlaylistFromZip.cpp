#include "Playlist.hpp"
#include "../common/stringUtils.hpp"
#include "../common/WXWidgets.hpp"
#include <wx/archive.h>
#include <wx/wfstream.h>
#include<fmt/format.h>
#include<memory>
using namespace std;
using fmt::format;


struct ZipArchiveFormat: PlaylistFormat {
ZipArchiveFormat (): PlaylistFormat("", "") {}
virtual bool checkWrite (const string& dir) override { return false; }
virtual bool save (Playlist& list, const string& dir) override { return false; }
virtual bool checkRead (const string& name) override {
return !!wxArchiveClassFactory::Find(U(name), wxSTREAM_FILEEXT);
}
virtual bool load (Playlist& list, const string& archiveName) {
wxLogNull logNull;
auto factory = wxArchiveClassFactory::Find(U(archiveName), wxSTREAM_FILEEXT);
if (!factory) return false;
auto file = new wxFFileInputStream(U(archiveName));
if (!file || !file->IsOk()) return false;
auto archive = unique_ptr<wxArchiveInputStream>(factory->NewStream(file));
if (!archive || !archive->IsOk()) return false;
while(auto entry = unique_ptr<wxArchiveEntry>(archive->GetNextEntry())) {
if (entry->IsDir()) continue;
auto name = entry->GetName();
string url = format("zip://{}?{}", archiveName, U(name));
list.add(url);
}
return true;
}
};

void plAddZip () {
Playlist::formats.push_back(make_shared<ZipArchiveFormat>());
}
