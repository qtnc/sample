#include "../playlist/Playlist.hpp"
#include "../loader/Loader.hpp"
#include "../common/WXWidgets.hpp"
#include "../common/BassPlugin.hpp"
#include "../common/bass.h"
#include "../common/stringUtils.hpp"
#include<string>
#include<vector>
using namespace std;


string BuildWildcardFilter (const std::vector<BassPlugin>& bassPlugins) {
vector<pair<string,string>> formats = {
{ "MPEG1 Layer3", "*.mp3;*.mp2;*.mp1" },
{ "OGG Vorbis", "*.ogg" },
{ "Apple interchange file format", "*.aif;*.aiff;*.aifc" },
{ "Wave Microsoft", "*.wav" }
};
for (const auto& plugin: bassPlugins) {
auto info = BASS_PluginGetInfo(plugin.plugin);
if (!info) continue;
for (size_t j=0; j<info->formatc; j++) {
auto& f = info->formats[j];
if (!f.name || !f.exts) continue;
formats.push_back(make_pair(f.name, f.exts));
}}
for (auto& f: Loader::loaders) {
if (f->extension.empty() || f->name.empty()) continue;
formats.push_back(make_pair(f->name, f->extension));
}
for (auto& f: Playlist::formats) {
if (f->extension.empty() || f->name.empty()) continue;
formats.push_back(make_pair(f->name, f->extension));
}

string allExtensions;
for (auto& p: formats) {
if (!allExtensions.empty()) allExtensions+=';';
allExtensions+=p.second;
}
string allfmt = "All files (*.*)|*.*|All supported files|" + allExtensions;
for (auto& p: formats) {
allfmt += '|';
allfmt += p.first;
allfmt += " (";
allfmt += p.second;
allfmt += ")|";
allfmt += p.second;
}
return allfmt;
}
