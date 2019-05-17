#include "App.hpp"
#include "PreferencesDlg.hpp"
#include "MainWindow.hpp"
#include <wx/listctrl.h>
#include <wx/listbook.h>
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

