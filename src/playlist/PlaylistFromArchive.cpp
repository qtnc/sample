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


struct ArchiveFormat: PlaylistFormat {
ArchiveFormat (): PlaylistFormat("", "") {}
virtual bool checkWrite (const string& dir) override { return false; }
virtual bool save (Playlist& list, const string& dir) override { return false; }
virtual bool checkRead (const string& archiveName) override {
bool result = false;
auto ar = archive_read_new();
if (!ar) goto end;
if (archive_read_support_format_all(ar)) goto end;
if (archive_read_support_filter_all(ar)) goto end;
if (archive_read_open_filename(ar, archiveName.c_str(), 16384)) goto end;
result = true;
end: if (ar) archive_read_free(ar);
return result;
}
virtual bool load (Playlist& list, const string& archiveName) {
bool result = false;
archive_entry* entry = nullptr;
auto ar = archive_read_new();
if (!ar) goto end;
if (archive_read_support_format_all(ar)) goto end;
if (archive_read_support_filter_all(ar)) goto end;
if (archive_read_open_filename(ar, archiveName.c_str(), 16384)) goto end;
while (archive_read_next_header(ar, &entry)>=0) {
if (!entry) break;
auto name = archive_entry_pathname_utf8(entry);
if (!name || !*name) break;
if (ends_with(name, "/")) continue;
string url = format("archive://{}?{}", archiveName, name);
list.add(url);
result = true;
}
end: if (ar) archive_read_free(ar);
return result;
}
};

void plAddArchive () {
Playlist::formats.push_back(make_shared<ArchiveFormat>());
}
