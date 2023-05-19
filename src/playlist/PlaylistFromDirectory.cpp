#include "Playlist.hpp"
#include "../common/stringUtils.hpp"
#include "../common/WXWidgets.hpp"
#include <wx/dir.h>
using namespace std;


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
list.add(dirname + "/" + UFN(file));
}  while(dir.GetNext(&file));
return true;
}
};

void plAddDir () {
Playlist::formats.push_back(make_shared<DirFormat>());
}
