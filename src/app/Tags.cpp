#include "App.hpp"
#include "../playlist/Playlist.hpp"
#include "../common/stringUtils.hpp"
#include "../common/TagsLibraryDefs.h"
#include "../common/WXWidgets.hpp"
#include "../common/bass.h"
#include "../common/println.hpp"
#include<fmt/format.h>
using namespace std;
using fmt::format;

static vector<string> taglist = {
"title", "artist", "album",
"composer", "publisher",
"year", "recorddate", 
"genre", "comment"
};

bool App::initTags () {
return InitTagsLibrary();
}

void PlaylistItem::loadTagsFromBASS (unsigned long handle) {
BASS_CHANNELINFO info;
BASS_ChannelGetInfo(handle, &info);
if (info.ctype&BASS_CTYPE_MUSIC_MOD) {
auto modTitle = BASS_ChannelGetTags(handle, BASS_TAG_MUSIC_NAME), modAuthor = BASS_ChannelGetTags(handle, BASS_TAG_MUSIC_AUTH), modComment = BASS_ChannelGetTags(handle, BASS_TAG_MUSIC_MESSAGE);
if (modTitle) tags["title"] = modTitle;
if (modAuthor) tags["artist"] = modAuthor;
string comment = modComment? modComment : "";
comment += "\r\n";
for (int i=0; i<256; i++) {
auto txt = BASS_ChannelGetTags(handle, BASS_TAG_MUSIC_INST +i);
if (!txt) break;
comment += txt;
comment += ' ';
}
for (int i=0; i<256; i++) {
auto txt = BASS_ChannelGetTags(handle, BASS_TAG_MUSIC_SAMPLE +i);
if (!txt) break;
comment += txt;
comment += ' ';
}
trim(comment);
if (comment.size()) tags["comment"] = comment;
}
else {
auto th = TagsLibrary_Create();
TagsLibrary_LoadFromBASS(th, handle);
if (TagsLibrary_Loaded(th, ttAutomatic)){
for (auto& tagName: taglist) {
auto wTagName = U(to_upper_copy(tagName));
auto wTagValue = TagsLibrary_GetTag(th, const_cast<wxChar*>(wTagName.wc_str()) , ttAutomatic);
if (!wTagValue) continue;
string tagValue = U(wTagValue);
trim(tagValue);
if (!tagValue.size()) continue;
tags[tagName] = tagValue;
}
for (int i=0, n=TagsLibrary_TagCount(th, ttAutomatic); i<n; i++) {
TExtTag tag;
if (!TagsLibrary_GetTagByIndexEx(th, i, ttAutomatic, &tag)) break;
println("Tag {}: {}={}",
i, U(tag.Name), U(tag.Value));
}
}
TagsLibrary_Free(th);
}

string sTitle = tags["title"], sArtist = tags["artist"], sAlbum = tags["album"];
if (!sArtist.size()) sArtist = tags["composer"];
if (sTitle.size() && sArtist.size() && sAlbum.size()) title = format("{} - {} ({})", sTitle, sArtist, sAlbum);
else if (sTitle.size() && sArtist.size()) title = format("{} - {}", sTitle, sArtist);
else if (sTitle.size()) title = sTitle;
else if (sArtist.size()) title = sArtist;
else {
int lastSlash = file.find_last_of("/\\");
int lastDot = file.find_last_of(".");
if (lastDot==string::npos) lastDot = file.size();
if (lastSlash==string::npos) lastSlash = -1;
title = file.substr(lastSlash+1, lastDot);
}
length = BASS_ChannelBytes2Seconds(handle, BASS_ChannelGetLength(handle, BASS_POS_BYTE));
}

bool PlaylistItem::saveTags () {
auto uFile = U(file);
auto th = TagsLibrary_Create();
bool result = TagsLibrary_Load(th, const_cast<wxChar*>(uFile.wc_str()), ttAutomatic, TRUE);
if (result) {
for (auto& tag: tags) {
trim(tag.second);
if (!tag.second.size()) continue;
auto uKey = U(to_upper_copy(tag.first)), uValue = U(tag.second);
result = result && TagsLibrary_SetTag(th, const_cast<wxChar*>(uKey.wc_str()), const_cast<wxChar*>(uValue.wc_str()), ttAutomatic);
}
result = result && TagsLibrary_Save(th, const_cast<wxChar*>(uFile.wc_str()), ttAutomatic);
}
TagsLibrary_Free(th);
return result;
}
