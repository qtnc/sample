#ifdef __WIN32
#define UNICODE
#include<winsock2.h>
#endif
#include "Encoder.hpp"
#include "../common/bass.h"
#include "../common/bassenc.h"
#include "../common/bassenc_aac.h"
#include "../common/wxWidgets.hpp"
#include "../app/App.hpp"
#include<fmt/format.h>
using namespace std;
using fmt::format;

#define CBR 0
#define VBR 1

struct FormatOptionsDlgAAC: wxDialog {
static vector<int> bitrates;

wxComboBox *cbMode, *cbBitrate, *cbVbr;


void OnModeChange (wxCommandEvent& e) {
int mode = cbMode->GetSelection();
cbVbr->Enable(mode==1);
cbBitrate->Enable(mode!=1);
}

FormatOptionsDlgAAC (App& app, wxWindow* parent):
wxDialog(parent, -1, U(translate("FormatOptionsDlg"))) 
{
wxString sBitrates[bitrates.size()], sVbr[5], sModes[2] = { U(translate("MP3CBR")), U(translate("MP3VBR")) };
for (int i=0; i<5; i++) sVbr[i] = U(format("VBR Level {}", i+1));
for (int i=0, n=bitrates.size(); i<n; i++) sBitrates[i] = U(format("{}kbps", bitrates[i]));
auto lblMode = new wxStaticText(this, wxID_ANY, U(translate("MP3Mode")) );
cbMode = new wxComboBox(this, 501, wxEmptyString, wxDefaultPosition, wxDefaultSize, 3, sModes, wxCB_DROPDOWN | wxCB_READONLY);
auto lblBitrate = new wxStaticText(this, wxID_ANY, U(translate("lblBitrate")) );
cbBitrate = new wxComboBox(this, 504, wxEmptyString, wxDefaultPosition, wxDefaultSize, bitrates.size(), sBitrates, wxCB_DROPDOWN | wxCB_READONLY);
auto lblVbr = new wxStaticText(this, wxID_ANY, U(translate("Quality")) );
cbVbr = new wxComboBox(this, 502, wxEmptyString, wxDefaultPosition, wxDefaultSize, 5, sVbr, wxCB_DROPDOWN | wxCB_READONLY);

auto sizer = new wxFlexGridSizer(2, 0, 0);
sizer->Add(lblMode);
sizer->Add(cbMode);
sizer->Add(lblBitrate);
sizer->Add(cbBitrate);
sizer->Add(lblVbr);
sizer->Add(cbVbr);

auto dlgSizer = new wxBoxSizer(wxVERTICAL);
dlgSizer->Add(sizer, 1, wxEXPAND);
auto btnSizer = new wxStdDialogButtonSizer();
btnSizer->AddButton(new wxButton(this, wxID_OK));
btnSizer->AddButton(new wxButton(this, wxID_CANCEL));
btnSizer->Realize();
dlgSizer->Add(btnSizer, 0, wxEXPAND);
SetSizerAndFit(dlgSizer);

cbMode->Bind(wxEVT_COMBOBOX, &FormatOptionsDlgAAC::OnModeChange, this);
cbMode->SetFocus();
}

void fillTo (int& mode, int& bitrate, int& vbr) {
mode = cbMode->GetSelection();
vbr = cbVbr->GetSelection() +1;
bitrate = bitrates[cbBitrate->GetSelection()];
}

void fillFrom (int mode, int bitrate, int vbr) {
int bIndex = std::find(bitrates.begin(), bitrates.end(), bitrate) -bitrates.begin();
cbMode->SetSelection(mode);
cbVbr->SetSelection(vbr -1);
cbBitrate->SetSelection(bIndex);
wxCommandEvent* eptr = nullptr;
OnModeChange(*eptr);
}

};
vector<int> FormatOptionsDlgAAC::bitrates = { 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 192, 224, 256, 320  };


struct EncoderAAC: Encoder {
int mode, bitrate, vbr;

EncoderAAC (): 
Encoder("Advanced audio coding (AAC)", "aac", BASS_ENCODE_TYPE_AAC),
mode(CBR), bitrate(192), vbr(5)
{}

string getOptions () {
switch(mode){
case CBR: return format("--vbr 0 --bitrate {}", bitrate*1024);
case VBR: return format("--vbr {}", vbr);
default: return "";
}}

string getOptions (PlaylistItem& item) {
return getOptions();
}

virtual DWORD startEncoderFile (PlaylistItem& item, DWORD input, const std::string& file) final override {
string options = getOptions(item);
return BASS_Encode_AAC_StartFile(input, const_cast<char*>(options.c_str()), BASS_ENCODE_AUTOFREE, const_cast<char*>(file.c_str()) );
}

virtual DWORD startEncoderStream (DWORD input) final override {
string options = getOptions();
return BASS_Encode_AAC_Start(input, const_cast<char*>(options.c_str()), BASS_ENCODE_AUTOFREE, nullptr, nullptr);
}

virtual bool hasFormatDialog () final override { return true; }

virtual bool showFormatDialog (wxWindow* parent) final override {
FormatOptionsDlgAAC fod(wxGetApp(), parent);
fod.fillFrom(mode, bitrate, vbr);
if (wxID_OK==fod.ShowModal()) {
fod.fillTo(mode, bitrate, vbr);
return true;
}
return false;
}

};

void encAddAAC () {
Encoder::encoders.push_back(make_shared<EncoderAAC>());
}
