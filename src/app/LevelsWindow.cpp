#include "LevelsWindow.hpp"
#include "App.hpp"
#include "MainWindow.hpp"
using namespace std;

extern vector<pair<int,string>> BASS_GetDeviceList(bool includeLoopback = false);
extern vector<pair<int,string>> BASS_RecordGetDeviceList(bool includeLoopback = false);

LevelsWindow::LevelsWindow (App& app):
wxDialog(app.win, -1, U(translate("LevelsWin")) ),
app(app) 
{
auto lblStreamDevice = new wxStaticText(this, wxID_ANY, U(translate("LvStreamDevice")), wxPoint(-2, -2), wxSize(1, 1) );
cbStreamDevice = new wxComboBox(this, 500, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_DROPDOWN | wxCB_READONLY);
auto lblStreamVol = new wxStaticText(this, wxID_ANY, U(translate("LvStreamVol")), wxPoint(-2, -2), wxSize(1, 1) );
slStreamVol = new wxSlider(this, 600, app.streamVol * 100, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
auto lblPreviewDevice = new wxStaticText(this, wxID_ANY, U(translate("LvPreviewDevice")), wxPoint(-2, -2), wxSize(1, 1) );
cbPreviewDevice = new wxComboBox(this, 501, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_DROPDOWN | wxCB_READONLY);
auto lblPreviewVol = new wxStaticText(this, wxID_ANY, U(translate("LvPreviewVol")), wxPoint(-2, -2), wxSize(1, 1) );
slPreviewVol = new wxSlider(this, 601, app.previewVol * 100, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
auto lblMicDevice1 = new wxStaticText(this, wxID_ANY, U(translate("LvMicDevice1")), wxPoint(-2, -2), wxSize(1, 1) );
cbMicDevice1 = new wxComboBox(this, 502, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_DROPDOWN | wxCB_READONLY);
tbMic1 = new wxToggleButton(this, 602, U(translate("LvMic1Btn")) );
auto lblMicDevice2 = new wxStaticText(this, wxID_ANY, U(translate("LvMicDevice2")), wxPoint(-2, -2), wxSize(1, 1) );
cbMicDevice2 = new wxComboBox(this, 503, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_DROPDOWN | wxCB_READONLY);
tbMic2 = new wxToggleButton(this, 603, U(translate("LvMic2Btn")) );
auto lblMicFbDevice1 = new wxStaticText(this, wxID_ANY, U(translate("LvMicFbDevice1")), wxPoint(-2, -2), wxSize(1, 1) );
cbMicFbDevice1 = new wxComboBox(this, 504, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_DROPDOWN | wxCB_READONLY);
auto lblMicFbVol1 = new wxStaticText(this, wxID_ANY, U(translate("LvMicFbVol1")), wxPoint(-2, -2), wxSize(1, 1) );
slMicFbVol1 = new wxSlider(this, 604, app.micFbVol1 * 100, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
auto lblMicFbDevice2 = new wxStaticText(this, wxID_ANY, U(translate("LvMicFbDevice2")), wxPoint(-2, -2), wxSize(1, 1) );
cbMicFbDevice2 = new wxComboBox(this, 505, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_DROPDOWN | wxCB_READONLY);
auto lblMicFbVol2 = new wxStaticText(this, wxID_ANY, U(translate("LvMicFbVol2")), wxPoint(-2, -2), wxSize(1, 1) );
slMicFbVol2 = new wxSlider(this, 606, app.micFbVol2 * 100, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
auto lblStreamVolInMixer = new wxStaticText(this, wxID_ANY, U(translate("LvStreamVolInMixer")), wxPoint(-2, -2), wxSize(1, 1) );
slStreamVolInMixer = new wxSlider(this, 700, app.streamVolInMixer * 100, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
auto lblMicVol1 = new wxStaticText(this, wxID_ANY, U(translate("LvMicVol1")), wxPoint(-2, -2), wxSize(1, 1) );
slMicVol1 = new wxSlider(this, 701, app.micVol1 * 100, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
auto lblMicVol2 = new wxStaticText(this, wxID_ANY, U(translate("LvMicVol2")), wxPoint(-2, -2), wxSize(1, 1) );
slMicVol2 = new wxSlider(this, 702, app.micVol2 * 100, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL);
btnClose = new wxButton(this, wxID_CANCEL, U(translate("Close")));

auto blank1 = new wxStaticText(this, wxID_ANY, wxEmptyString, wxPoint(-2, -2), wxSize(1, 1) );
auto blank2 = new wxStaticText(this, wxID_ANY, wxEmptyString, wxPoint(-2, -2), wxSize(1, 1) );
auto blank3 = new wxStaticText(this, wxID_ANY, wxEmptyString, wxPoint(-2, -2), wxSize(1, 1) );
auto blank4 = new wxStaticText(this, wxID_ANY, wxEmptyString, wxPoint(-2, -2), wxSize(1, 1) );

updateLists();

auto sizer = new wxFlexGridSizer(3, 0, 0);
sizer->Add(lblStreamDevice);
sizer->Add(cbStreamDevice, 1);
sizer->Add(slStreamVol, 1);
sizer->Add(lblPreviewDevice);
sizer->Add(cbPreviewDevice, 1);
sizer->Add(slPreviewVol, 1);
sizer->Add(lblMicDevice1);
sizer->Add(cbMicDevice1, 1);
sizer->Add(tbMic1);
sizer->Add(lblMicDevice2);
sizer->Add(cbMicDevice2, 1);
sizer->Add(tbMic2);
sizer->Add(lblMicFbDevice1);
sizer->Add(cbMicFbDevice1, 1);
sizer->Add(slMicFbVol1, 1);
sizer->Add(lblMicFbDevice2);
sizer->Add(cbMicFbDevice2, 1);
sizer->Add(slMicFbVol2, 1);
sizer->Add(lblStreamVolInMixer);
sizer->Add(blank3);
sizer->Add(slStreamVolInMixer);
sizer->Add(lblMicVol1);
sizer->Add(blank4);
sizer->Add(slMicVol1, 1);
sizer->Add(lblMicVol2);
sizer->Add(blank1);
sizer->Add(slMicVol2, 1);
sizer->Add(blank2);
sizer->Add(btnClose);
SetSizerAndFit(sizer);

cbStreamDevice->Bind(wxEVT_COMBOBOX, &LevelsWindow::OnChangeStreamDevice, this);
cbPreviewDevice->Bind(wxEVT_COMBOBOX, &LevelsWindow::OnChangePreviewDevice, this);
cbMicDevice1->Bind(wxEVT_COMBOBOX, [&](auto& e){ OnChangeMicDevice(e, 1); });
cbMicDevice2->Bind(wxEVT_COMBOBOX, [&](auto& e){ OnChangeMicDevice(e, 2); });
cbMicFbDevice1->Bind(wxEVT_COMBOBOX, [&](auto& e){ OnChangeMicFeedbackDevice(e, 1); });
cbMicFbDevice2->Bind(wxEVT_COMBOBOX, [&](auto& e){ OnChangeMicFeedbackDevice(e, 2); });
slStreamVol->Bind(wxEVT_SCROLL_CHANGED, [&](auto& e){ app.win->changeVol( slStreamVol->GetValue()/100.0, true, false); });
slPreviewVol->Bind(wxEVT_SCROLL_CHANGED, [&](auto& e){ app.changePreviewVol( slPreviewVol->GetValue()/100.0, false, true); });
slMicFbVol1->Bind(wxEVT_SCROLL_CHANGED, [&](auto& e){ app.changeMicFeedbackVol( slMicFbVol1->GetValue()/100.0, 1, false); });
slMicFbVol2->Bind(wxEVT_SCROLL_CHANGED, [&](auto& e){ app.changeMicFeedbackVol( slMicFbVol2->GetValue()/100.0, 2, false); });
slMicVol1->Bind(wxEVT_SCROLL_CHANGED, [&](auto& e){ app.changeMicMixVol( slMicVol1->GetValue()/100.0, 1, false); });
slMicVol2->Bind(wxEVT_SCROLL_CHANGED, [&](auto& e){ app.changeMicMixVol( slMicVol1->GetValue()/100.0, 2, false); });
slStreamVolInMixer->Bind(wxEVT_SCROLL_CHANGED, [&](auto& e){ app.changeStreamMixVol( slStreamVolInMixer->GetValue()/100.0, false); });
tbMic1->Bind(wxEVT_TOGGLEBUTTON, [&](auto& e){ app.startStopMic(1, tbMic1->GetValue(), true, false);  });
tbMic2->Bind(wxEVT_TOGGLEBUTTON, [&](auto& e){ app.startStopMic(2, tbMic2->GetValue(), true, false);  });
btnClose->Bind(wxEVT_BUTTON, [&](auto& e){ this->OnCloseRequest(); });
cbStreamDevice->SetFocus();
}

void LevelsWindow::OnCloseRequest () {
this->Hide(); 
app.win->GetMenuBar()->Check(IDM_SHOWLEVELS, false);
app.win->btnPlay->SetFocus(); 
}

void LevelsWindow::OnChangeStreamDevice (wxCommandEvent& e) {
app.win->setTimeout(2500, [=]()mutable{ app.changeStreamDevice( reinterpret_cast<int>(cbStreamDevice->GetClientData(cbStreamDevice->GetSelection())) ); });
}

void LevelsWindow::OnChangePreviewDevice (wxCommandEvent& e) {
app.win->setTimeout(2500, [=]()mutable{ app.changePreviewDevice( reinterpret_cast<int>(cbPreviewDevice->GetClientData(cbPreviewDevice->GetSelection())) ); });
}

void LevelsWindow::OnChangeMicDevice (wxCommandEvent& e, int n) {
wxComboBox* cbs[] = { cbMicDevice1, cbMicDevice2 };
wxComboBox* cb = cbs[n -1];
app.win->setTimeout(2500, [=]()mutable{ app.changeMicDevice( reinterpret_cast<int>(cb->GetClientData(cb->GetSelection())), n ); });
}

void LevelsWindow::OnChangeMicFeedbackDevice (wxCommandEvent& e, int n) {
wxComboBox* cbs[] = { cbMicFbDevice1, cbMicFbDevice2 };
wxComboBox* cb = cbs[n -1];
app.win->setTimeout(2500, [=]()mutable{ app.changeMicFeedbackDevice( reinterpret_cast<int>(cb->GetClientData(cb->GetSelection())), n ); });
}

static void updateList (wxComboBox* cb, int sel, const vector<pair<int,string>>& list) {
if (sel<0 && list.size()>0) sel=0;
int i=0, selidx = -1;
cb->Clear();
for (auto& p: list) {
cb->Append(U(p.second), reinterpret_cast<void*>(p.first));
if (sel==p.first) selidx=i;
i++;
}
cb->SetSelection(selidx);
}

void LevelsWindow::updateLists () {
bool includeLoopback = app.config.get("app.levels.includeLoopback", false);
auto list = BASS_GetDeviceList(includeLoopback);
updateList(cbStreamDevice, app.streamDevice, list);
updateList(cbPreviewDevice, app.previewDevice, list);
updateList(cbMicFbDevice1, app.micFbDevice1, list);
updateList(cbMicFbDevice2, app.micFbDevice2, list);
list = BASS_RecordGetDeviceList(includeLoopback );
updateList(cbMicDevice1, app.micDevice1, list);
updateList(cbMicDevice2, app.micDevice2, list);
}

