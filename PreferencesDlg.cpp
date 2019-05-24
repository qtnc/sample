#include "App.hpp"
#include "PreferencesDlg.hpp"
#include "MainWindow.hpp"
#include <wx/listctrl.h>
#include <wx/listbook.h>
#include <wx/spinctrl.h>
#include "cpprintf.hpp"
#include "stringUtils.hpp"
#include "UniversalSpeech.h"
#include "bass.h"
#include "bassmidi.h"
using namespace std;

void PreferencesDlg::ShowDlg (App& app, wxWindow* parent) {
PreferencesDlg prefs(app, parent);
if (prefs.ShowModal()==wxID_OK) {
}}

PreferencesDlg::PreferencesDlg (App& app, wxWindow* parent):
wxDialog(parent, -1, U(translate("PreferencesDlg")) ),
app(app)
{
book = new wxListbook(this, 4999, wxDefaultPosition, wxDefaultSize, wxLB_LEFT);

{ // Integration page
auto page = new wxPanel(book);
auto sizer = new wxBoxSizer(wxVERTICAL);
wxString openActions[] = { U(translate("PrefIntegrationOAOpen")), U(translate("PrefIntegrationOAAppend")) };
auto openAction = new wxRadioBox(page, 300, U(translate("PrefIntegrationOA")), wxDefaultPosition, wxDefaultSize, 2, openActions);
auto openFocus = new wxCheckBox(page, 301, U(translate("PrefIntegrationOAFocus")));
sizer->Add(openAction, 1, wxEXPAND);
sizer->Add(openFocus);
page->SetSizer(sizer);
book->AddPage(page, U(translate("PrefIntegrationPage")));
}

{ // Audio page
auto page = new wxPanel(book);
auto sizer = new wxBoxSizer(wxVERTICAL);
auto includeLoopback = new wxCheckBox(page, 325, U(translate("PrefAudioIncludeLoopback")));
sizer->Add(includeLoopback);
page->SetSizer(sizer);
book->AddPage(page, U(translate("PrefAudioPage")));
}

{ // Casting page
auto page = new wxPanel(book);
auto castAutoTitle = new wxCheckBox(page, 351, U(translate("PrefCastingAutoTitle")));
auto lblLRTB = new wxStaticText(page, wxID_ANY, U(translate("PrefCastingLRTB")) );
auto spLRT = new wxSpinCtrl(page, 350, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 300, 60);
auto lblLRTA = new wxStaticText(page, wxID_ANY, U(translate("PrefCastingLRTA")) );
auto sizer = new wxBoxSizer(wxVERTICAL);
auto subsizer = new wxBoxSizer(wxHORIZONTAL);
subsizer->Add(lblLRTB);
subsizer->Add(spLRT);
subsizer->Add(lblLRTA);
sizer->Add(castAutoTitle);
sizer->Add(subsizer);
page->SetSizer(sizer);
book->AddPage(page, U(translate("PrefCastingPage")));
}

//auto lblInfo = new wxStaticText(this, wxID_ANY, U(translate("ItInfoList")) );
//lcInfo = new wxListView(this, 501, wxDefaultPosition, wxDefaultSize, wxLC_NO_HEADER | wxLC_SINGLE_SEL | wxLC_REPORT);
//taComment = new wxTextCtrl(this, 500, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);

auto sizer = new wxBoxSizer(wxVERTICAL);
sizer->Add(book, 1, wxEXPAND);
auto btnSizer = new wxStdDialogButtonSizer();
btnSizer->AddButton(new wxButton(this, wxID_OK));
btnSizer->AddButton(new wxButton(this, wxID_CANCEL));
btnSizer->Realize();
sizer->Add(btnSizer, 0, wxEXPAND);
SetSizerAndFit(sizer);
book->SetFocus();
}

