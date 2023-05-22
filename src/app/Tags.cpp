#include "App.hpp"
#include "Tags.hpp"
#include "../playlist/Playlist.hpp"
#include "../common/stringUtils.hpp"
#include "../common/TagsLibraryDefs.h"
#include "../common/WXWidgets.hpp"
#include <wx/filename.h>
#include "../common/bass.h"
#include "../common/bassmidi.h"
#include "../common/println.hpp"
#include<fmt/format.h>
using namespace std;
using fmt::format;

bool App::initTags () {
return InitTagsLibrary();
}

std::vector<std::string> getAllMarks (DWORD handle, int type) {
std::vector<std::string> v;
BASS_MIDI_MARK mark;
for (int i=0; BASS_MIDI_StreamGetMark(handle, type, i, &mark) && mark.text; i++) v.push_back(mark.text);
return v;
}

void LoadTagsFromMIDI (DWORD handle, PropertyMap& tags) {
auto titles = getAllMarks(handle, BASS_MIDI_MARK_TRACK);
auto copy = getAllMarks(handle, BASS_MIDI_MARK_COPY);
auto texts = getAllMarks(handle, BASS_MIDI_MARK_TEXT);

std::string copyright, text, title;
bool textNL = true;
if (titles.size()==1) title = titles[0];
else if (texts.size()==1) title = texts[0];
for (auto& c: copy) {
trim(c);
copyright += c + "\n";
}
for (auto& t: texts) {
if (starts_with(t, "@T")) title += t.substr(2);
else if (starts_with(t, "@L")) tags.set("language", trim_copy(t.substr(2)));
if (starts_with(t, "\\")) t.erase(t.begin());
else if (starts_with(t, "/") || starts_with(t, "@")) {
t.replace(0, 1, "\n");
textNL = false;
}}
for (auto& t: texts) {
text += t;
if (textNL) text += "\n";
}
if (titles.size()>1) for (auto& t: titles) {
text += t + "\n";
}
trim(text);
if (!text.empty()) tags.set("comment", text);
trim(copyright);
if (!copyright.empty()) tags.set("copyright", copyright);
trim(title);
if (!title.empty()) tags.set("title", title);

BASS_MIDI_MARK mark;
for (int i=0; BASS_MIDI_StreamGetMark(handle, BASS_MIDI_MARK_MARKER, i, &mark); i++) {
if (!mark.text) continue;
if (iequals(mark.text, "loopstart")) tags.set("loopstart", mark.pos);
if (iequals(mark.text, "loopend")) tags.set("loopend", mark.pos);
}

}

void LoadTagsFromMod (DWORD handle, PropertyMap& tags) {
auto modTitle = BASS_ChannelGetTags(handle, BASS_TAG_MUSIC_NAME), modAuthor = BASS_ChannelGetTags(handle, BASS_TAG_MUSIC_AUTH), modComment = BASS_ChannelGetTags(handle, BASS_TAG_MUSIC_MESSAGE);
if (modTitle) tags.set("title", modTitle);
if (modAuthor) tags.set("artist", modAuthor);
string comment;
if (modComment) comment += modComment;
comment += "\n";
for (int i=0; i<256; i++) {
auto txt = BASS_ChannelGetTags(handle, BASS_TAG_MUSIC_INST +i);
if (!txt) break;
comment += txt;
comment += '\n';
}
for (int i=0; i<256; i++) {
auto txt = BASS_ChannelGetTags(handle, BASS_TAG_MUSIC_SAMPLE +i);
if (!txt) break;
comment += txt;
comment += '\n';
}
trim(comment);
if (comment.size()) tags.set("comment", comment);
}

void LoadTagsFromBASS (unsigned long handle, PropertyMap& tags, const std::string& file, std::string* displayTitle) {
BASS_CHANNELINFO info;
BASS_ChannelGetInfo(handle, &info);

if (info.ctype==BASS_CTYPE_STREAM_MIDI) {
LoadTagsFromMIDI(handle, tags);
}
else if (info.ctype&BASS_CTYPE_MUSIC_MOD) {
LoadTagsFromMod(handle, tags);
}
else {
auto th = TagsLibrary_Create();
TagsLibrary_LoadFromBASS(th, handle);
if (TagsLibrary_Loaded(th, ttAutomatic)){
for (int i=0, n=TagsLibrary_TagCount(th, ttAutomatic); i<n; i++) {
TExtTag tag;
if (!TagsLibrary_GetTagByIndexEx(th, i, ttAutomatic, &tag)) break;
std::string name = U(tag.Name), value = U(tag.Value);
trim(name); trim(value);
if (!name.empty() && !value.empty()) tags.set(to_lower_copy(name), value);
}}
else LoadTagsFromMod(handle, tags);
TagsLibrary_Free(th);
}

if (displayTitle) {
string sTitle = tags.get<std::string>("title"), sArtist = tags.get<std::string>("artist"), sAlbum = tags.get<std::string>("album");
if (!sArtist.size()) sArtist = tags.get<std::string>("composer");
if (!sTitle.empty() && !sArtist.empty() && !sAlbum.empty()) *displayTitle = format("{} - {} ({})", sTitle, sArtist, sAlbum);
else if (!sTitle.empty() && !sArtist.empty()) *displayTitle = format("{} - {}", sTitle, sArtist);
else if (!sTitle.empty()) *displayTitle = sTitle;
else if (!sArtist.empty()) *displayTitle = sArtist;
else if (!file.empty()) {
wxString stem;
wxFileName::SplitPath(UI(file), nullptr, &stem, nullptr);
*displayTitle = U(stem);
}
}

tags.set("length", BASS_ChannelBytes2Seconds(handle, BASS_ChannelGetLength(handle, BASS_POS_BYTE)));
}

void PlaylistItem::loadMetaData  (unsigned long stream) {
PropertyMap tags(PM_LCKEYS);
loadMetaData(stream, tags);
}

void PlaylistItem::loadMetaData  (unsigned long stream, PropertyMap& tags) {
LoadTagsFromBASS(stream, tags, file, &title);
length = BASS_ChannelBytes2Seconds(stream, BASS_ChannelGetLength(stream, BASS_POS_BYTE));
replayGain = tags.get("replaygain", replayGain);
}

bool SaveTags (const std::string& file, const PropertyMap& tags) {
auto uFile = U(file);
auto th = TagsLibrary_Create();
bool result = TagsLibrary_Load(th, const_cast<wxChar*>(uFile.wc_str()), ttAutomatic, TRUE);
if (result) {
for (auto& tag: tags.kvmap()) {
auto uKey = U(to_upper_copy(tag.first)), uValue = U(tag.second);
result = result && TagsLibrary_SetTag(th, const_cast<wxChar*>(uKey.wc_str()), const_cast<wxChar*>(uValue.wc_str()), ttAutomatic);
}
result = result && TagsLibrary_Save(th, const_cast<wxChar*>(uFile.wc_str()), ttAutomatic);
}
TagsLibrary_Free(th);
return result;
}
