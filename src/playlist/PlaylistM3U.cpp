#include "Playlist.hpp"
#include "../common/stringUtils.hpp"
#include<fstream>
#include "../common/wxWidgets.hpp"
#include <wx/wfstream.h>
#include <wx/stdstream.h>
using namespace std;

struct M3UFormat: PlaylistFormat {
M3UFormat (): PlaylistFormat("M3U Playlist", "m3u") {}
virtual bool checkRead (const string& file) final override {
return iends_with(file, ".m3u") || iends_with(file, ".m3u8");
}
virtual bool checkWrite (const string& file) final override {
return checkRead(file);
}
bool load (Playlist& list, const string& file) final override {
wxFileInputStream fIn(U(file));
wxStdInputStream in(fIn);
if (!in) return false;
string line;
bool wasEmpty = !list.size();
int i=0, curidx=-1, curpos=-1;
list.add(string());
while(getline(in, line)) {
auto& item = list[i];
trim(line);
if (!line.size()) continue;
bool notFile = starts_with(line, "#");
if (starts_with(line, "#EXTINF:")) {
line = line.substr(8);
trim(line);
auto j = line.find(',');
if (j!=string::npos) {
string sLen = line.substr(0, j), sTitle = line.substr(j+1);
trim(sLen); trim(sTitle);
item.length = stoi(sLen);
item.title = sTitle;
}}
else if (starts_with(line, "#EXTCURIDX:")) {
line = line.substr(11);
trim(line);
curidx = stoi(line);
}
else if (starts_with(line, "#EXTCURPOS:")) {
line = line.substr(11);
trim(line);
curpos = stoi(line);
}
if (notFile) continue;
item.file = line;
list.add(string());
i++;
}
list.erase(list.size() -1);
if (curpos>=0) list.curPosition = curpos;
if (wasEmpty || list.curIndex<0) if (curidx>=0) list.curIndex = curidx;
return true;
}
virtual bool save (Playlist& list, const string& file) final override {
wxFileOutputStream fOut(U(file));
wxStdOutputStream out(fOut);
if (!out) return false;
out << "#EXTM3U" << endl;
for (auto& item: list.items) {
if (item->title.size()) out << "#EXTINF: " << item->length << ", " << item->title << endl;
out << item->file << endl;
}
if (list.curIndex>=0) out << "#EXTCURIDX: " << list.curIndex << endl;
if (list.curPosition>0) out << "#EXTCURPOS: " << list.curPosition << endl;
return true;
}
};

void plAddM3U () {
Playlist::formats.push_back(make_shared<M3UFormat>());
}
