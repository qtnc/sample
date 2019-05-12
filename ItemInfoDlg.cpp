#include "App.hpp"
#include "ItemInfoDlg.hpp"
#include "Playlist.hpp"
#include "MainWindow.hpp"
#include <wx/listctrl.h>
#include "cpprintf.hpp"
#include "stringUtils.hpp"
#include "UniversalSpeech.h"
#include "bass.h"
#include "bassmidi.h"
using namespace std;

vector<string> ItemInfoDlg::taglist = {
"title", "artist", "album",
"composer", "publisher",
"year", "recorddate", "genre"
};

ItemInfoDlg::ItemInfoDlg (App& app, wxWindow* parent, PlaylistItem& it, unsigned long ch):
wxDialog(parent, -1, U(translate("ItemInfoDlg")) ),
item(it), app(app)
{
auto lblInfo = new wxStaticText(this, wxID_ANY, U(translate("ItInfoList")) );
lcInfo = new wxListBox(this, 501, wxDefaultPosition, wxDefaultSize, 0, nullptr, 0);
auto lblComment = new wxStaticText(this, wxID_ANY, U(translate("ItComment")) );
taComment = new wxTextCtrl(this, 500, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
btnSave = new wxButton(this, wxID_SAVE);

if (!ch) {
ch = app.loadFileOrURL(item.file, false, true);
item.loadTagsFromBASS(ch);
fillList(ch);
BASS_StreamFree(ch);
}
else fillList(ch);
taComment->SetValue(item.tags["comment"]);

auto sizer = new wxBoxSizer(wxVERTICAL);
sizer->Add(lblInfo);
sizer->Add(lcInfo, 1, wxEXPAND);
sizer->Add(lblComment);
sizer->Add(taComment, 1, wxEXPAND);
auto btnSizer = new wxStdDialogButtonSizer();
btnSizer->AddButton(btnSave);
btnSizer->AddButton(new wxButton(this, wxID_CANCEL));
btnSizer->Realize();
sizer->Add(btnSizer, 0, wxEXPAND);

lcInfo->Bind(wxEVT_LISTBOX_DCLICK, [&](auto& e){ OnListDoubleClick(); });
lcInfo->Bind(wxEVT_CHAR_HOOK, &ItemInfoDlg::OnListKeyDown, this);
lcInfo->Bind(wxEVT_CONTEXT_MENU, &ItemInfoDlg::OnListContextMenu, this);
btnSave->Bind(wxEVT_BUTTON, [&](auto& e){ OnSave(); EndDialog(wxID_CANCEL); });

SetSizerAndFit(sizer);
lcInfo->SetFocus();
}

void ItemInfoDlg::OnSave () {
item.tags["comment"] = U(taComment->GetValue());
if (item.saveTags()) speechSay(U(translate("Saved")).wc_str(), true);
else {
wxMessageBox(U(translate("ItSaveErrorMsg")), U(translate("ItSaveErrorDlg")), wxOK | wxICON_STOP, this);
}}

void ItemInfoDlg::OnListContextMenu (wxContextMenuEvent& e) {
int sel = lcInfo->GetSelection();
if (sel<0) return;
bool istag = sel>=tagIndex;
vector<string> items = {
translate("Copy")
};
vector<function<void(ItemInfoDlg&)>> actions = {
&ItemInfoDlg::OnListCopySel
};
if (istag) {
items.push_back(translate("Modify"));
actions.push_back(&ItemInfoDlg::OnListDoubleClick);
}
int re = app.win->popupMenu(items);
if (re>=0 && re<actions.size()) actions[re](*this);
}

void ItemInfoDlg::OnListKeyDown (wxKeyEvent& e) {
int key = e.GetKeyCode(), mod = e.GetModifiers();
if (key==WXK_RETURN && mod==0) OnListDoubleClick();
else if (key=='C' && mod==wxMOD_CONTROL) OnListCopySel();
else if (key=='S' && mod==wxMOD_CONTROL) OnSave();
else e.Skip();
}

void ItemInfoDlg::OnListCopySel () {
int sel = lcInfo->GetSelection();
if (sel<0) return;
string text = U(lcInfo->GetString(sel));
int pos = text.find(':');
text = trim_copy(text.substr(pos+1));
setClipboardText(text);
speechSay(U(translate("Copied")).wc_str(), true);
}

void ItemInfoDlg::OnListDoubleClick () {
int sel = lcInfo->GetSelection();
if (sel<0) return;
else if (sel<tagIndex) OnListCopySel();
else if (sel>=tagIndex) {
auto& text = item.tags[taglist[sel - tagIndex]];
wxTextEntryDialog ted(this, U(translate("ItEditTagDlg")), U(translate("ItEditTagPrompt")), U(text));
if (wxID_OK==ted.ShowModal()) {
text = U(ted.GetValue());
string sText = U(lcInfo->GetString(sel));
sText = sText.substr(0, sText.find('\t')+1) + text;
lcInfo->SetString(sel, sText);
}}
}

static string getOrigresName (BASS_CHANNELINFO& ci) {
bool isfloat = ci.origres&BASS_ORIGRES_FLOAT;
int depth = ci.origres&0xFF;
return format("%d bit%s", depth, isfloat?" float":"");
}

static string getFormatName (BASS_CHANNELINFO& ci) {
switch(ci.ctype){
case BASS_CTYPE_STREAM_OGG: return "OGG Vorbis";
case BASS_CTYPE_STREAM_MP1: return "MPEG1 Layer 1 (MP1)";
case BASS_CTYPE_STREAM_MP2: return "MPEG1 Layer 2 (MP2)";
case BASS_CTYPE_STREAM_MP3: return "MPEG1 Layer 3 (MP3)";
case BASS_CTYPE_STREAM_AIFF: return "Apple Interchange File Format (AIFF)";
case BASS_CTYPE_STREAM_WAV_PCM: return "Wave PCM";
case BASS_CTYPE_STREAM_WAV_FLOAT: return "Wave Float";
case BASS_CTYPE_MUSIC_MOD: return "ModTracker Module";
case BASS_CTYPE_MUSIC_MTM: return "MultiTracker Module";
case BASS_CTYPE_MUSIC_S3M: return "ScreamTracker3 Module";
case BASS_CTYPE_MUSIC_XM: return "FastTracker II Module";
case BASS_CTYPE_MUSIC_IT: return "ImpulseTracker Module";
case BASS_CTYPE_MUSIC_MO3 | BASS_CTYPE_MUSIC_MOD:
case BASS_CTYPE_MUSIC_MO3 | BASS_CTYPE_MUSIC_MTM:
case BASS_CTYPE_MUSIC_MO3 | BASS_CTYPE_MUSIC_S3M:
case BASS_CTYPE_MUSIC_MO3 | BASS_CTYPE_MUSIC_XM:
case BASS_CTYPE_MUSIC_MO3 | BASS_CTYPE_MUSIC_IT:
return "Mo3 Module";
}
if (ci.plugin) {
auto info = BASS_PluginGetInfo(ci.plugin);
if (info) for (int i=0; i<info->formatc; i++) {
auto& fmt = info->formats[i];
if (fmt.ctype==ci.ctype) return fmt.name;
}}
return "Unknown";
}

void ItemInfoDlg::fillList (unsigned long ch) {
lcInfo->Clear();

auto fnidx = item.file.find_last_of("/\\"), cidx=item.file.find(':');
string filename = fnidx==string::npos? item.file : item.file.substr(fnidx+1);
string filepath = cidx>=2&&cidx<7? string() : item.file.substr(0, fnidx);
if (filename.size()) lcInfo->Append(U(format("%s:\t%s", translate("ItFileName"), filename)));
if (filepath.size()) lcInfo->Append(U(format("%s:\t%s", translate("ItFilePath"), filepath)));
lcInfo->Append(U(format("%s:\t%s", translate("ItFileFull"), item.file)));

BASS_CHANNELINFO ci;
if (BASS_ChannelGetInfo(ch,&ci)) {
float bitrate = 0;
BASS_ChannelGetAttribute(ch, BASS_ATTRIB_BITRATE, &bitrate);
auto filesize = BASS_StreamGetFilePosition(ch, BASS_FILEPOS_SIZE);
auto length = BASS_ChannelBytes2Seconds(ch, BASS_ChannelGetLength(ch, BASS_POS_BYTE));
if (bitrate<=0 && filesize>0 && filesize!=-1 && length>=0) bitrate = (filesize>>7) / length;
lcInfo->Append(U(format("%s:\t%s", translate("ItFormat"), getFormatName(ci))));
if (length>0) lcInfo->Append(U(format("%s:\t%s", translate("ItLength"), formatTime(length))));
if (ci.freq>0) lcInfo->Append(U(format("%s:\t%d Hz", translate("ItFreq"), ci.freq)));
if (ci.chans>0) lcInfo->Append(U(format("%s:\t%d (%s)", translate("ItChannels"), ci.chans, translate("ItChannels" + to_string(ci.chans)))));
if (ci.origres>0) lcInfo->Append(U(format("%s:\t%s", translate("ItOrigres"), getOrigresName(ci))));
if (bitrate>0) lcInfo->Append(U(format("%s:\t%sbps", translate("ItBitrate"), formatSize(bitrate*1024.0))));
if (filesize>0 && filesize!=-1) lcInfo->Append(U(format("%s:\t%s", translate("ItFilesize"), formatSize(filesize))));
}

tagIndex = lcInfo->GetCount();
for (auto& tag: taglist) {
string sItem  = format("%s:\t%s", translate("tag_" + tag), item.tags[tag]);
lcInfo->Append(U(sItem));
}

}
