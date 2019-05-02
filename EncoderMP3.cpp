#ifdef __WIN32
#define UNICODE
#include<winsock2.h>
#endif
#include "Encoder.hpp"
#include "cpprintf.hpp"
#include "bass.h"
#include "bassenc.h"
#include "bassenc_mp3.h"
#include "wxWidgets.hpp"
#include "App.hpp"
using namespace std;

#define CBR 0
#define ABR 1
#define VBR 2

struct FormatOptionsDlgMP3: wxDialog {
static vector<int> bitrates;

wxComboBox *cbMode, *cbBitrate, *cbVbr, *cbComplexity;


void OnModeChange (wxCommandEvent& e) {
int mode = cbMode->GetSelection();
cbVbr->Enable(mode==2);
cbBitrate->Enable(mode!=2);
}

FormatOptionsDlgMP3 (App& app, wxWindow* parent):
wxDialog(parent, -1, U(translate("FormatOptionsDlg"))) 
{
int vbrkbps[] = { 320, 256, 224, 192, 160, 128, 112, 96, 80, 64 };
wxString sBitrates[bitrates.size()], sVbr[10], sComplexities[10], sModes[3] = { U(translate("MP3CBR")), U(translate("MP3ABR")), U(translate("MP3VBR")) };
for (int i=0; i<10; i++) sComplexities[i] = U(translate("mp3q" + to_string(i)));
for (int i=0; i<10; i++) sVbr[i] = U(format("V%d (~%d kbps)", i, vbrkbps[i]));
for (int i=0, n=bitrates.size(); i<n; i++) sBitrates[i] = U(format("%d kbps", bitrates[i]));
auto lblMode = new wxStaticText(this, wxID_ANY, U(translate("MP3Mode")) );
cbMode = new wxComboBox(this, 501, wxEmptyString, wxDefaultPosition, wxDefaultSize, 3, sModes, wxCB_DROPDOWN | wxCB_READONLY);
auto lblBitrate = new wxStaticText(this, wxID_ANY, U(translate("lblBitrate")) );
cbBitrate = new wxComboBox(this, 504, wxEmptyString, wxDefaultPosition, wxDefaultSize, bitrates.size(), sBitrates, wxCB_DROPDOWN | wxCB_READONLY);
auto lblVbr = new wxStaticText(this, wxID_ANY, U(translate("Quality")) );
cbVbr = new wxComboBox(this, 502, wxEmptyString, wxDefaultPosition, wxDefaultSize, 10, sVbr, wxCB_DROPDOWN | wxCB_READONLY);
auto lblComplexity = new wxStaticText(this, wxID_ANY, U(translate("Complexity")) );
cbComplexity = new wxComboBox(this, 503, wxEmptyString, wxDefaultPosition, wxDefaultSize, 10, sComplexities, wxCB_DROPDOWN | wxCB_READONLY);

auto sizer = new wxFlexGridSizer(2, 0, 0);
sizer->Add(lblMode);
sizer->Add(cbMode);
sizer->Add(lblBitrate);
sizer->Add(cbBitrate);
sizer->Add(lblVbr);
sizer->Add(cbVbr);
sizer->Add(lblComplexity);
sizer->Add(cbComplexity);

auto dlgSizer = new wxBoxSizer(wxVERTICAL);
dlgSizer->Add(sizer, 1, wxEXPAND);
auto btnSizer = new wxStdDialogButtonSizer();
btnSizer->AddButton(new wxButton(this, wxID_OK));
btnSizer->AddButton(new wxButton(this, wxID_CANCEL));
btnSizer->Realize();
dlgSizer->Add(btnSizer, 0, wxEXPAND);
SetSizerAndFit(dlgSizer);

cbMode->Bind(wxEVT_COMBOBOX, &FormatOptionsDlgMP3::OnModeChange, this);
cbMode->SetFocus();
}

void fillTo (int& mode, int& bitrate, int& vbr, int& quality) {
mode = cbMode->GetSelection();
vbr = cbVbr->GetSelection();
bitrate = bitrates[cbBitrate->GetSelection()];
quality = cbComplexity->GetSelection();
}

void fillFrom (int mode, int bitrate, int vbr, int quality) {
int bIndex = std::find(bitrates.begin(), bitrates.end(), bitrate) -bitrates.begin();
cbMode->SetSelection(mode);
cbVbr->SetSelection(vbr);
cbComplexity->SetSelection(quality);
cbBitrate->SetSelection(bIndex);
wxCommandEvent* eptr = nullptr;
OnModeChange(*eptr);
}

};
vector<int> FormatOptionsDlgMP3::bitrates = { 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320  };


struct EncoderMP3: Encoder {
int mode, bitrate, vbr, quality;

EncoderMP3 (): 
Encoder("MPEG1 Layer3 (MP3)", "mp3", BASS_ENCODE_TYPE_MP3),
mode(ABR), bitrate(192), vbr(3), quality(3)
{}

string getOptions () {
switch(mode){
case CBR: return format("-q%d -b%d", quality, bitrate);
case ABR: return format("-q%d --abr %d", quality, bitrate);
case VBR: return format("-q%d -V%d", quality, vbr);
default: return "";
}}

string getOptions (PlaylistItem& item) {
return getOptions();
}

virtual DWORD startEncoderFile (PlaylistItem& item, DWORD input, const std::string& file) final override {
string options = getOptions(item);
return BASS_Encode_MP3_StartFile(input, const_cast<char*>(options.c_str()), BASS_ENCODE_AUTOFREE, const_cast<char*>(file.c_str()) );
}

virtual DWORD startEncoderStream (DWORD input) final override {
string options = getOptions();
return BASS_Encode_MP3_Start(input, const_cast<char*>(options.c_str()), BASS_ENCODE_AUTOFREE, nullptr, nullptr);
}

virtual bool hasFormatDialog () final override { return true; }

virtual bool showFormatDialog (wxWindow* parent) final override {
FormatOptionsDlgMP3 fod(wxGetApp(), parent);
fod.fillFrom(mode, bitrate, vbr, quality);
if (wxID_OK==fod.ShowModal()) {
fod.fillTo(mode, bitrate, vbr, quality);
return true;
}
return false;
}

};

void encAddMP3 () {
Encoder::encoders.push_back(make_shared<EncoderMP3>());
}
