#include "App.hpp"
#include "ItemInfoDlg.hpp"
#include "Playlist.hpp"
#include "cpprintf.hpp"
#include "stringUtils.hpp"
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
SetSizerAndFit(sizer);
lcInfo->SetFocus();
}

static string getOrigresName (BASS_CHANNELINFO& ci) {
bool isfloat = ci.origres&BASS_ORIGRES_FLOAT;
int depth = ci.origres&0xFF;
return format("%d bit%s", depth, isfloat?" float":"");
}

static string getFormatName (BASS_CHANNELINFO& ci) {
return "To be done";
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
