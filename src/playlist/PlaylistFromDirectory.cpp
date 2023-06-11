#include "Playlist.hpp"
#include "../common/stringUtils.hpp"
#include "../common/WXWidgets.hpp"
#include <wx/dir.h>
#include<wx/filename.h>
using namespace std;

std::string makeAbsoluteIfNeeded (const std::string& path, const std::string& basefile) {
std::string re = path;
if (starts_with(re, "file:/")) re = re.substr(re.find_first_not_of("/:", 4));
if (re.find(':')!=std::string::npos) return re;
wxFileName file(U(re));
if (file.IsAbsolute()) return re;
wxFileName base(U(basefile));
file.MakeAbsolute(base.GetPath());
re = U(file.GetFullPath());
return re;
}



struct DirFormat: PlaylistFormat {
DirFormat (): PlaylistFormat("", "") {}
virtual bool checkWrite (const string& dir) override { return false; }
virtual bool save (Playlist& list, const string& dir) override { return false; }
virtual bool checkRead (const string& dirname) override {
wxDir dir(U(dirname));
return dir.IsOpened();
}
virtual bool load (Playlist& list, const string& dirname) {
wxLogNull logNull;
wxDir dir(U(dirname));
if (!dir.IsOpened()) return false;
wxString file;
if (dir.GetFirst(&file)) do {
list.add(dirname + "/" + U(file));
}  while(dir.GetNext(&file));
return true;
}
};

void plAddDir () {
Playlist::formats.push_back(make_shared<DirFormat>());
}
