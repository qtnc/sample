#include "../common/constants.h"
#include "Playlist.hpp"
#include "../common/stringUtils.hpp"
#include<algorithm>
#include<random>
#include<regex>
using namespace std;


void plAddDir ();
void plAddPLS ();
void plAddM3U();
void plAddXML ();
void plAddArchive ();

inline regex rcreatereg (const string& s0) {
static vector<pair<string,string>> replacements = {
{ ".", "\\." },
{ "*", ".*" },
{ "?", "." },
};
static regex specials("[\\[\\]\\(\\)\\{\\}\\\\+$^]");
string s = s0;
for (auto& p: replacements) replace_all(s, p.first, p.second);
s = regex_replace(s, specials, "\\$0");
return regex(s, regex::icase);
}

inline bool rmatch (const string& s, const regex& reg) {
smatch m;
return regex_search(s, m, reg);
}

bool PlaylistItem::match (const std::string& s, int index) {
if (!s.size()) return true;
if (index>=0 && string::npos==s.find_first_not_of("0123456789") && index==stoi(s)) return true;
regex reg = rcreatereg(s);
if (rmatch(file, reg) || rmatch(title, reg)) return true;
return false;
}

PlaylistItem& Playlist::add (const std::string& file, int n) {
if (n<0) n+=items.size() +1;
items.insert(items.begin() +n, std::make_shared<PlaylistItem>(file));
auto& item = (*this)[n];
item.length = -1;
return item;
}

void Playlist::sort (const std::function<bool(const std::shared_ptr<PlaylistItem>&, const std::shared_ptr<PlaylistItem>&)>& func) {
shared_ptr<PlaylistItem> item = curIndex>=0? items[curIndex] : nullptr;
std::stable_sort(items.begin(), items.end(), func);
auto it = std::find(items.begin(), items.end(), item);
if (it!=items.end()) curIndex = it-items.begin();
}

void Playlist::shuffle (int fromIndex, int toIndex) {
auto curItem = curIndex<0? nullptr : items[curIndex];
std::mt19937 rand;
if (toIndex<0) toIndex = items.size();
std::shuffle(items.begin() + fromIndex, items.begin() + toIndex, rand);
if (curIndex>=0) curIndex = std::find(items.begin(), items.end(), curItem) -items.begin();
}

bool Playlist::load (const string& file) {
initFormats();
for (auto& format: formats) {
if (format->checkRead(file) && format->load(*this, file)) {
this->file = file;
this->format = format;
return true;
}}
return false;
}

bool Playlist::save (const string& file0) {
auto format = this->format;
string file = file0;
if (!file.size()) file = this->file;
if (!file.size()) return false;
initFormats();
if (!format || !format->checkWrite(file)) for (auto& fmt: formats) {
if (fmt->checkWrite(file)) { format=fmt; break; }
}
if (!format || !format->checkWrite(file) || !format->save(*this, file)) return false;
this->format = format;
this->file = file;
return true;
}

void Playlist::initFormats () {
if (!formats.size()) {
plAddPLS();
plAddM3U();
plAddDir();
plAddArchive();
plAddXML();
}
}


vector<shared_ptr<PlaylistFormat>> Playlist::formats;
