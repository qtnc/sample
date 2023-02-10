#include "Playlist.hpp"
#include "../common/PropertyMap.hpp"
#include "../common/stringUtils.hpp"
#include<fstream>
#include "../common/wxWidgets.hpp"
#include <wx/wfstream.h>
#include <wx/stdstream.h>
using namespace std;

struct PLSFormat: PlaylistFormat {
PLSFormat (): PlaylistFormat("PLS Playlist", "*.pls") {}
virtual bool checkRead (const string& file) final override {
return iends_with(file, ".pls");
}
virtual bool checkWrite (const string& file) final override {
return checkRead(file);
}
bool load (Playlist& list, const string& file) final override {
wxFileInputStream fIn(U(file));
wxStdInputStream in(fIn);
string line;
if (!in || !getline(in, line) || !iequals(line, "[playlist]")) return false;
bool wasEmpty = !list.size();
PropertyMap map(PM_LCKEYS);
map.load(in);
auto count = map.get("numberofentries", string::npos);
if (count<=0 || count==string::npos) return false;
for (size_t i=1; i<=count; i++) {
string num = to_string(i);
auto& entry = list.add(map.get("file" + num, string()));
entry.title = map.get("title" +num, string());
entry.length = map.get("length" + num, -1);
entry.replayGain = map.get("replaygain" + num, 0.0);
}
list.curPosition = map.get("currentposition", 0);
if (wasEmpty || list.curIndex<0) list.curIndex = map.get("currententry", list.curIndex);
if (wasEmpty || list.curSubindex<=0) list.curSubindex = map.get("currentsubentry", list.curSubindex);
return true;
}
virtual bool save (Playlist& list, const string& file) final override {
wxFileOutputStream fOut(U(file));
wxStdOutputStream out(fOut);
if (!out) return false;
out << "[Playlist]" << endl;
for (int i=0, N=list.size(); i<N; i++) {
int index=i+1;
auto& item = list[i];
out << "File" << index << '=' << item.file << endl;
if (item.title.size()) out << "Title" << index << '=' << item.title << endl;
if (item.length>0) out << "Length" << index << '=' << item.length << endl;
if (item.replayGain!=0) out << "ReplayGain" << index << '=' << item.replayGain << endl;
}
if (list.curIndex>=0) out << "CurrentEntry=" << list.curIndex << endl;
if (list.curSubindex>0) out << "CurrentSubEntry=" << list.curSubindex << endl;
if (list.curPosition>0) out << "CurrentPosition=" << list.curPosition << endl;
out << "NumberOfEntries=" << list.size() << endl;
out << "Version=2" << endl;
return true;
}
};

void plAddPLS () {
Playlist::formats.push_back(make_shared<PLSFormat>());
}
