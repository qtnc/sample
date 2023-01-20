#include "Playlist.hpp"
#include "../common/PropertyMap.hpp"
#include "../common/stringUtils.hpp"
#include<fstream>
#include "../common/cpprintf.hpp"
using namespace std;

struct PLSFormat: PlaylistFormat {
PLSFormat (): PlaylistFormat("PLS Playlist", "pls") {}
virtual bool checkRead (const string& file) final override {
return iends_with(file, ".pls");
}
virtual bool checkWrite (const string& file) final override {
return checkRead(file);
}
bool load (Playlist& list, const string& file) final override {
ifstream in(file);
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
}
for (auto& p: map.kvmap()) {
auto key = p.first, value = p.second;
if (!starts_with(key, "tag.")) continue;
auto i = key.rfind(".");
if (i<=4 || string::npos==i) continue;
auto index = stoul(key.substr(i+1));
key = key.substr(4, i-4);
if (!key.size() || index<=0 || index>count) continue;
auto& item = list[index -1];
item.tags[key] = value;
}
list.curPosition = map.get("currentposition", 0);
if (wasEmpty || list.curIndex<0) list.curIndex = map.get("currententry", list.curIndex);
return true;
}
virtual bool save (Playlist& list, const string& file) final override {
ofstream out(file);
if (!out) return false;
out << "[Playlist]" << endl;
for (int i=0, N=list.size(); i<N; i++) {
int index=i+1;
auto& item = list[i];
out << "File" << index << '=' << item.file << endl;
if (item.title.size()) out << "Title" << index << '=' << item.title << endl;
out << "Length" << index << '=' << item.length << endl;
for (auto& tag: item.tags) {
if (!tag.second.size()) continue;
string value = tag.second;
replace_all(value, "\n", " ");
replace_all(value, "\r", " ");
replace_all(value, "\t", " ");
out << "tag." << tag.first << '.' << index << '=' << value << endl;
}}
if (list.curIndex>=0) out << "CurrentEntry=" << list.curIndex << endl;
if (list.curPosition>0) out << "CurrentPosition=" << list.curPosition << endl;
out << "NumberOfEntries=" << list.size() << endl;
out << "Version=2" << endl;
return true;
}
};

void plAddPLS () {
Playlist::formats.push_back(make_shared<PLSFormat>());
}
