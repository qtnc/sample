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

template<> wxString PropertyMap::get (const string& key, const wxString& def) {
return U(get(key, U(def)));
}

struct ConfigBind {
string key;
ConfigBind (const string& k): key(k) {}
template <class T> inline void storeValue (App& app, const T& value) { app.config.set(key, value); }
template<class T> T inline loadValue (App& app, const T& def) { return app.config.get(key, def); }
virtual void load (App& app) = 0;
virtual void store (App& app) = 0;
};

template<class T, class C> struct ConfigValueBind: ConfigBind  {
C* component;
T def;
ConfigValueBind (const string& k, const T& d, C* c): ConfigBind(k), def(d), component(c) {}
virtual void load (App& app) override { component->SetValue( loadValue(app, def)); }
virtual void store (App& app) override { storeValue(app, component->GetValue()); }
};

template<class T, class C> struct ConfigSelectionBind: ConfigBind  {
C* component;
int def;
vector<T> values;
ConfigSelectionBind (const string& k, int d, const vector<T>& v, C* c): ConfigBind(k), def(d), component(c), values(v)  {}
ConfigSelectionBind (const string& k, int d, C* c): ConfigBind(k), def(d), component(c) {}
virtual void load (App& app) override { 
if (values.empty()) component->SetSelection( loadValue(app, def)); 
else {
T value = loadValue(app, values[def]);
auto it = find(values.begin(), values.end(), value);
int selection = it==values.end()? -1 : it-values.begin();
component->SetSelection(selection);
}}
virtual void store (App& app) override { 
if (values.empty()) storeValue(app, component->GetSelection()); 
else {
int selection = component->GetSelection();
if (selection>=0 && selection<values.size()) storeValue(app, values[selection]);
}}
};

struct ConfigBindList {
vector<unique_ptr<ConfigBind>> binds;
void load (App& app) { for (auto& b: binds) b->load(app); }
void store (App& app) { for (auto& b: binds) b->store(app); }
template<class T, class C> ConfigBindList& bind (const string& key, const T& def, C* c) {
binds.push_back(make_unique<ConfigValueBind<T,C>>(key, def, c));
return *this;
}
template<class T, class C> ConfigBindList& bind (const string& key, C* c, int def) {
binds.push_back(make_unique<ConfigSelectionBind<T,C>>(key, def, c));
return *this;
}
template<class T, class C> ConfigBindList& bind (const string& key, C* c, int def, const vector<T>& values) {
binds.push_back(make_unique<ConfigSelectionBind<T,C>>(key, def, values, c));
return *this;
}
};

void PreferencesDlg::ShowDlg (App& app, wxWindow* parent) {
PreferencesDlg prefs(app, parent);

ConfigBindList binds;
prefs.makeBinds(binds);
binds.load(app);
if (prefs.ShowModal()==wxID_OK) {
binds.store(app);
}}

void PreferencesDlg::makeBinds (ConfigBindList& binds) {
binds
.bind("app.integration.openFocus", false, openFocus)
.bind("app.integration.openAction", openAction, 0, vector<string>({"open", "append"}))

.bind("app.levels.includeLoopback", false, includeLoopback)

.bind("midi.soundfont.path", app.appDir + U("/ct8mgm.sf2"), midiSfPath)
.bind("midi.voices.max", 256, spMaxMidiVoices)

.bind("cast.autoTitle", false, castAutoTitle)
.bind("cast.listenersRefreshRate", 30, spLRT)
;
}

PreferencesDlg::PreferencesDlg (App& app, wxWindow* parent):
wxDialog(parent, -1, U(translate("PreferencesDlg")) ),
app(app)
{
book = new wxListbook(this, 4999, wxDefaultPosition, wxDefaultSize, wxLB_LEFT);

{ // Integration page
auto page = new wxPanel(book);
auto sizer = new wxBoxSizer(wxVERTICAL);
wxString openActions[] = { U(translate("PrefIntegrationOAOpen")), U(translate("PrefIntegrationOAAppend")) };
openAction = new wxRadioBox(page, 300, U(translate("PrefIntegrationOA")), wxDefaultPosition, wxDefaultSize, 2, openActions);
openFocus = new wxCheckBox(page, 301, U(translate("PrefIntegrationOAFocus")));
sizer->Add(openAction, 1, wxEXPAND);
sizer->Add(openFocus);
page->SetSizer(sizer);
book->AddPage(page, U(translate("PrefIntegrationPage")));
}

{ // Devices and levels page
auto page = new wxPanel(book);
auto sizer = new wxBoxSizer(wxVERTICAL);
includeLoopback = new wxCheckBox(page, 325, U(translate("PrefLvIncludeLoopback")));
sizer->Add(includeLoopback);
page->SetSizer(sizer);
book->AddPage(page, U(translate("PrefLvPage")));
}

{ // MIDI page
auto page = new wxPanel(book);
auto lblSfPath = new wxStaticText(page, wxID_ANY, U(translate("PrefMIDISfPath")) );
midiSfPath = new wxTextCtrl(page, 376, wxEmptyString);
auto lblMaxVoices = new wxStaticText(page, wxID_ANY, U(translate("PrefMIDIMaxVoices")) );
spMaxMidiVoices = new wxSpinCtrl(page, 375, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 4096, 256);
auto sizer = new wxBoxSizer(wxVERTICAL);
auto grid = new wxFlexGridSizer(2, 0, 0);
grid->Add(lblSfPath);
grid->Add(midiSfPath, 1, wxEXPAND);
grid->Add(lblMaxVoices);
grid->Add(spMaxMidiVoices);
sizer->Add(grid, 1, wxEXPAND);
page->SetSizer(sizer);
book->AddPage(page, U(translate("PrefMIDIPage")));
}

{ // Casting page
auto page = new wxPanel(book);
castAutoTitle = new wxCheckBox(page, 351, U(translate("PrefCastingAutoTitle")));
auto lblLRTB = new wxStaticText(page, wxID_ANY, U(translate("PrefCastingLRTB")) );
spLRT = new wxSpinCtrl(page, 350, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 300, 60);
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

