#include "Playlist.hpp"
#include "../common/stringUtils.hpp"
//#include "../common/WXWidgets.hpp"
//#include <wx/wfstream.h>
#include "archive.h"
#include "archive_entry.h"
#include<fmt/format.h>
#include<memory>
using namespace std;
using fmt::format;

typedef std::shared_ptr<archive> archive_ptr;

struct ArchiveFormat: PlaylistFormat {
ArchiveFormat (): PlaylistFormat("Archive files", "*.zip;*.jar;*.tar;*.bz2;*.tbz;*.7z;*.xz;*.gz;*.lzma;*.lz4;*.lzip;*.lha;*.lzh;*.iso;*.cab;*.ar;*.xar;*.cpio;*.pax") {}
virtual bool checkWrite (const string& dir) override { return false; }
virtual bool save (Playlist& list, const string& dir) override { return false; }
virtual bool checkRead (const string& archiveName) override {
auto ar = archive_ptr(archive_read_new(), &archive_read_free);
if (!ar) return false;
if (archive_read_support_format_all(ar.get())) return false;
if (archive_read_support_filter_all(ar.get())) return false;
if (archive_read_open_filename(ar.get(), archiveName.c_str(), 16384)) return false;
return true;
}
virtual bool load (Playlist& list, const string& archiveName) {
archive_entry* entry = nullptr;
auto ar = archive_ptr(archive_read_new(), &archive_read_free);
if (!ar) return false;
if (archive_read_support_format_all(ar.get())) return false;
if (archive_read_support_filter_all(ar.get())) return false;
if (archive_read_open_filename(ar.get(), archiveName.c_str(), 16384)) return false;
while (!archive_read_next_header(ar.get(), &entry)) {
if (!entry) break;
auto name = archive_entry_pathname_utf8(entry);
if (!name || !*name) break;
if (ends_with(name, "/")) continue;
string url = format("archive://{}?{}", archiveName, name);
list.add(url);
}
return true;
}
};

void plAddArchive () {
Playlist::formats.push_back(make_shared<ArchiveFormat>());
}
