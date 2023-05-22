#include "App.hpp"
#include "CastStreamDlg.hpp"
#include "../caster/Caster.hpp"
#include "../encoder/Encoder.hpp"
using namespace std;

CastStreamDlg::CastStreamDlg (App& app, wxWindow* parent):
wxDialog(parent, -1, U(translate("CastStreamDlg")) )
{
auto lblFormat = new wxStaticText(this, wxID_ANY, U(translate("CastFormat")) );
cbFormat = new wxComboBox(this, 505, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_DROPDOWN | wxCB_READONLY);
btnFormat = new wxButton(this, 506, U(translate("FormatOptionsBtn")));
auto lblServerType = new wxStaticText(this, wxID_ANY, U(translate("CastServerType")) );
cbServerType = new wxComboBox(this, 504, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_DROPDOWN | wxCB_READONLY);
auto lblServer = new wxStaticText(this, wxID_ANY, U(translate("CastServer")) );
tfServer = new wxTextCtrl(this, 500, wxEmptyString);
auto lblPort = new wxStaticText(this, wxID_ANY, U(translate("CastPort")) );
tfPort = new wxTextCtrl(this, 501, wxEmptyString);
auto lblMount = new wxStaticText(this, wxID_ANY, U(translate("CastMount")) );
tfMount = new wxTextCtrl(this, 502, wxEmptyString);
auto lblUser = new wxStaticText(this, wxID_ANY, U(translate("CastUser") ) );
tfUser = new wxTextCtrl(this, 507, wxEmptyString);
auto lblPass = new wxStaticText(this, wxID_ANY, U(translate("CastPass") ) );
tfPass = new wxTextCtrl(this, 503, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);

if (!Encoder::encoders.size()) encAddAll();
for (int i=0, n=Encoder::encoders.size(); i<n; i++) {
auto& encoder = *Encoder::encoders[i];
cbFormat->Append(U(encoder.name), reinterpret_cast<void*>(i));
}
for (auto& caster: Caster::casters) cbServerType->Append(U(caster->name));

auto fmtSizer = new wxBoxSizer(wxHORIZONTAL);
fmtSizer->Add(cbFormat, 1, wxEXPAND);
fmtSizer->Add(btnFormat, 0);
auto sizer = new wxFlexGridSizer(2, 0, 0);
sizer->Add(lblFormat);
sizer->Add(fmtSizer, 1, wxEXPAND);
sizer->Add(lblServerType);
sizer->Add(cbServerType, 1, wxEXPAND);
sizer->Add(lblServer);
sizer->Add(tfServer, 1, wxEXPAND);
sizer->Add(lblPort);
sizer->Add(tfPort);
sizer->Add(lblMount);
sizer->Add(tfMount, 1, wxEXPAND);
sizer->Add(lblUser);
sizer->Add(tfUser, 1, wxEXPAND);
sizer->Add(lblPass);
sizer->Add(tfPass, 1, wxEXPAND);
auto dlgSizer = new wxBoxSizer(wxVERTICAL);
dlgSizer->Add(sizer, 1, wxEXPAND);
auto btnSizer = new wxStdDialogButtonSizer();
btnSizer->AddButton(new wxButton(this, wxID_OK));
btnSizer->AddButton(new wxButton(this, wxID_CANCEL));
btnSizer->Realize();
dlgSizer->Add(btnSizer, 0, wxEXPAND);
SetSizerAndFit(dlgSizer);
cbFormat->SetFocus();

btnFormat->Bind(wxEVT_BUTTON, &CastStreamDlg::OnFormatOptions, this);
cbFormat->Bind(wxEVT_COMBOBOX, &CastStreamDlg::OnFormatChange, this);
cbServerType->Bind(wxEVT_COMBOBOX, &CastStreamDlg::OnServerTypeChange, this);

wxCommandEvent* eptr = nullptr;
cbFormat->SetSelection(0);
cbServerType->SetSelection(0);
OnFormatChange(*eptr);
OnServerTypeChange(*eptr);
}

void CastStreamDlg::OnServerTypeChange (wxCommandEvent& e) {
int selection = cbServerType->GetSelection();
int flags = Caster::casters[selection]->flags;
tfServer->Enable(flags&CF_SERVER);
tfPort->Enable(flags&CF_PORT);
tfUser->Enable(flags&CF_USERNAME);
tfPass->Enable(flags&CF_PASSWORD);
tfMount->Enable(flags&CF_MOUNT);
}

void CastStreamDlg::OnFormatChange (wxCommandEvent& e) {
int selection = cbFormat->GetSelection();
if (selection<0) return;
size_t index = reinterpret_cast<size_t>(cbFormat->GetClientData(selection));
if (index>=Encoder::encoders.size()) return;
auto& encoder = *Encoder::encoders[index];
btnFormat->Enable(encoder.hasFormatDialog());
}

void CastStreamDlg::OnFormatOptions (wxCommandEvent& e) {
int selection = cbFormat->GetSelection();
if (selection<0) return;
size_t index = reinterpret_cast<size_t>(cbFormat->GetClientData(selection));
if (index>=Encoder::encoders.size()) return;
auto& encoder = *Encoder::encoders[index];
if (encoder.hasFormatDialog()) encoder.showFormatDialog(this);
}
