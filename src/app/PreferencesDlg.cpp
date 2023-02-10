#include "App.hpp"
#include "PreferencesDlg.hpp"
#include "MainWindow.hpp"
#include <wx/listctrl.h>
#include <wx/listbook.h>
#include <wx/spinctrl.h>
#include <wx/gbsizer.h>
#include "../common/stringUtils.hpp"
#include "../common/UniversalSpeech.h"
#include "../common/bass.h"
#include "../common/bassmidi.h"
#include<fmt/format.h>
using namespace std;
using fmt::format;

wxString MIDIFontGetDisplayName (BassFontConfig& font);
wxString MIDIFontGetMapDesc (BassFontConfig& font);

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
virtual ~ConfigBind() {}
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

struct PluginListConfigBind: ConfigBind {
wxListView* lc;
PluginListConfigBind (wxListView* lc0): ConfigBind(""), lc(lc0) {}
void load (App& app) override {
lc->ClearAll();
lc->AppendColumn(wxEmptyString);
for (int i=0, n=app.loadedPlugins.size(); i<n; i++) {
auto& p = app.loadedPlugins[i];
auto info = BASS_PluginGetInfo(p.plugin);
wxString name = p.name;
auto k = name.rfind('.');
if (k!=std::string::npos) name = name.substr(0, k);
if (info) name += U(format(" {}.{}.{}.{}", (info->version>>24)&0xFF, (info->version>>16)&0xFF, (info->version>>8)&0xFF, info->version&0xFF));
lc->InsertItem(i, name);
lc->CheckItem(i, p.enabled);
lc->SetItemData(i, i);
}
}
void store (App& app) override {
for (int j=0, n=app.loadedPlugins.size(); j<n; j++) {
int i = lc->GetItemData(j);
auto& p = app.loadedPlugins[i];
p.enabled = lc->IsItemChecked(i);
p.priority = j;
app.config.set("plugin." + U(p.name) + ".priority", p.priority);
app.config.set("plugin." + U(p.name) + ".enabled", p.enabled);
BASS_PluginEnable(p.plugin, p.enabled);
}
}
};

struct MIDIFontsConfigBind: ConfigBind {
wxListView* lc;
MIDIFontsConfigBind (wxListView* lc0): ConfigBind(""), lc(lc0) {}
void load (App& app) override {
lc->ClearAll();
lc->AppendColumn(U(translate("Soundfont")));
lc->AppendColumn(U(translate("Mapping")));
for (int i=0; i<app.midiConfig.size(); i++) {
auto& font = app.midiConfig[i];
lc->InsertItem(i, MIDIFontGetDisplayName(font));
lc->SetItem(i, 1, MIDIFontGetMapDesc(font));
lc->SetItemPtrData(i, reinterpret_cast<uintptr_t>( new BassFontConfig(font)));
}
}
void store (App& app) override {
app.midiConfig.clear();
for (int i=0, n=lc->GetItemCount(); i<n; i++) {
auto font = reinterpret_cast<BassFontConfig*>( lc->GetItemData(i) );
if (font) app.midiConfig.push_back(*font);
}
app.applyMIDIConfig(app.midiConfig);
}
~MIDIFontsConfigBind () {
for (int i=0, n=lc->GetItemCount(); i<n; i++) {
auto font = reinterpret_cast<BassFontConfig*>( lc->GetItemData(i) );
if (font) delete font;
}
}
};

bool PreferencesDlg::ShowDlg (App& app, wxWindow* parent) {
PreferencesDlg prefs(app, parent);

ConfigBindList binds;
prefs.makeBinds(binds);
binds.load(app);
if (prefs.ShowModal()==wxID_OK) {
binds.store(app);
app.saveConfig();
app.saveMIDIConfig();
return true;
}
return false;
}

void PreferencesDlg::makeBinds (ConfigBindList& binds) {
binds
.bind("app.integration.openFocus", false, openFocus)
.bind("app.integration.openAction", openAction, 0, vector<string>({"open", "append"}))

.bind("app.levels.includeLoopback", false, includeLoopback)

.bind("midi.voices.max", 256, spMaxMidiVoices)

.bind("cast.autoTitle", false, castAutoTitle)
.bind("cast.listenersRefreshRate", 30, spLRT)
;

binds.binds.push_back(std::make_unique<PluginListConfigBind>(lcInputPlugins));
binds.binds.push_back(std::make_unique<MIDIFontsConfigBind>(lcMIDIFonts));
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

{ // Input Plugins page
auto page = new wxPanel(book);
auto sizer = new wxBoxSizer(wxVERTICAL);
auto subsizer1 = new wxBoxSizer(wxHORIZONTAL);
auto subsizer2 = new wxBoxSizer(wxVERTICAL);
auto lblInputPlugins = new wxStaticText(page, wxID_ANY, U(translate("PrefInputPluginsLbl")) );
lcInputPlugins = new wxListView(page, 324, wxDefaultPosition, wxDefaultSize, wxLC_NO_HEADER | wxLC_SINGLE_SEL | wxLC_REPORT);
lcInputPlugins->EnableCheckBoxes();
auto btnMoveUp = new wxButton(page, wxID_UP);
auto btnMoveDown = new wxButton(page, wxID_DOWN);
btnMoveUp->SetId(322); btnMoveDown->SetId(323);
sizer->Add(lblInputPlugins, 0);
subsizer1->Add(lcInputPlugins, 1, wxEXPAND);
subsizer2->Add(btnMoveUp, 0);
subsizer2->Add(btnMoveDown, 0);
subsizer1->Add(subsizer2, 0);
sizer->Add(subsizer1, 1, wxEXPAND);
page->SetSizer(sizer);
book->AddPage(page, U(translate("PrefInputPluginsPage")));
lcInputPlugins->Bind(wxEVT_CHAR_HOOK, &PreferencesDlg::OnPluginListKeyDown, this);
btnMoveUp->Bind(wxEVT_BUTTON, &PreferencesDlg::OnPluginListMoveUp, this);
btnMoveDown->Bind(wxEVT_BUTTON, &PreferencesDlg::OnPluginListMoveDown, this);
}

{ // MIDI page
auto page = new wxPanel(book);
auto lblMIDIFonts = new wxStaticText(page, wxID_ANY, U(translate("PrefMIDIFontsLbl")) );
lcMIDIFonts = new wxListView(page, 376, wxDefaultPosition, wxDefaultSize, wxLC_NO_HEADER | wxLC_SINGLE_SEL | wxLC_REPORT);
auto btnAdd = new wxButton(page, wxID_ADD);
auto btnModify = new wxButton(page, wxID_EDIT);
auto btnRemove = new wxButton(page, wxID_REMOVE);
auto btnMoveUp = new wxButton(page, wxID_UP);
auto btnMoveDown = new wxButton(page, wxID_DOWN);
btnAdd->SetId(377); btnModify->SetId(378); btnRemove->SetId(379); btnMoveUp->SetId(380); btnMoveDown->SetId(381);
auto lblMaxVoices = new wxStaticText(page, wxID_ANY, U(translate("PrefMIDIMaxVoices")) );
spMaxMidiVoices = new wxSpinCtrl(page, 375, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 4096, 256);
auto sizer = new wxBoxSizer(wxVERTICAL);
auto subLcSizer = new wxBoxSizer(wxHORIZONTAL);
auto subLcSizerBtns = new wxBoxSizer(wxVERTICAL);
auto grid = new wxFlexGridSizer(2, 0, 0);
sizer->Add(lblMIDIFonts, 0);
subLcSizer->Add(lcMIDIFonts, 1, wxEXPAND);
subLcSizerBtns->Add(btnAdd, 0);
subLcSizerBtns->Add(btnModify, 0);
subLcSizerBtns->Add(btnRemove, 0);
subLcSizerBtns->Add(btnMoveUp, 0);
subLcSizerBtns->Add(btnMoveDown, 0);
subLcSizer->Add(subLcSizerBtns, 0);
sizer->Add(subLcSizer, 1, wxEXPAND);
grid->Add(lblMaxVoices);
grid->Add(spMaxMidiVoices);
sizer->Add(grid, 1, wxEXPAND);
page->SetSizer(sizer);
book->AddPage(page, U(translate("PrefMIDIPage")));
lcMIDIFonts->Bind(wxEVT_CHAR_HOOK, &PreferencesDlg::OnMIDIFontsKeyDown, this);
btnAdd->Bind(wxEVT_BUTTON, &PreferencesDlg::OnMIDIFontAdd, this);
btnModify->Bind(wxEVT_BUTTON, &PreferencesDlg::OnMIDIFontModify, this);
btnRemove->Bind(wxEVT_BUTTON, &PreferencesDlg::OnMIDIFontRemove, this);
btnMoveUp->Bind(wxEVT_BUTTON, &PreferencesDlg::OnMIDIFontMoveUp, this);
btnMoveDown->Bind(wxEVT_BUTTON, &PreferencesDlg::OnMIDIFontMoveDown, this);
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

MIDIFontDlg::MIDIFontDlg (App& app, wxWindow* parent, BassFontConfig& font0):
wxDialog(parent, -1, U(translate("MIDIFontDlg")) ),
font(font0)
{
auto lblFile = new wxStaticText(this, wxID_ANY, U(translate("MIDIFontFile")));
file = new wxTextCtrl(this, wxID_ANY, U(font.file), wxDefaultPosition, wxDefaultSize, 0);
browseFile = new wxButton(this, wxID_ANY, U(translate("Browse")));
auto lblSPreset = new wxStaticText(this, wxID_ANY, U(translate("MIDIFontSPreset")));
sPreset = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -1, 127, font.spreset);
auto lblDPreset = new wxStaticText(this, wxID_ANY, U(translate("MIDIFontDPreset")));
dPreset = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -127, 128, font.dpreset);
auto lblSBank = new wxStaticText(this, wxID_ANY, U(translate("MIDIFontSBank")));
sBank = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -1, 128, font.sbank);
auto lblDBank = new wxStaticText(this, wxID_ANY, U(translate("MIDIFontDBank")));
dBank = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -127, 128, font.dbank);
auto lblDBankLSB = new wxStaticText(this, wxID_ANY, U(translate("MIDIFontDBankLSB")));
dBankLSB = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 127, font.dbanklsb);
auto lblVolume = new wxStaticText(this, wxID_ANY, U(translate("MIDIFontVolume")));
volume = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10000, round(100 * font.volume));
xgDrums = new wxCheckBox(this, wxID_ANY, U(translate("MIDIFontXGDrums")));
nolimits = new wxCheckBox(this, wxID_ANY, U(translate("MIDIFontNoLimits")));
linattmod = new wxCheckBox(this, wxID_ANY, U(translate("MIDIFontLinAttMod")));
lindecvol = new wxCheckBox(this, wxID_ANY, U(translate("MIDIFontLinDecVol")));
minfx = new wxCheckBox(this, wxID_ANY, U(translate("MIDIFontMinFX")));
nofx = new wxCheckBox(this, wxID_ANY, U(translate("MIDIFontNoFX")));
norampin = new wxCheckBox(this, wxID_ANY, U(translate("MIDIFontNoRampIn")));

xgDrums->SetValue( font.flags & BASS_MIDI_FONT_XGDRUMS );
nolimits->SetValue( font.flags & BASS_MIDI_FONT_NOLIMITS );
norampin->SetValue( font.flags & BASS_MIDI_FONT_NORAMPIN );
linattmod->SetValue( font.flags & BASS_MIDI_FONT_LINATTMOD );
lindecvol->SetValue( font.flags & BASS_MIDI_FONT_LINDECVOL );
minfx->SetValue( font.flags & BASS_MIDI_FONT_MINFX );
nofx->SetValue( font.flags & BASS_MIDI_FONT_NOFX );

auto sizer = new wxBoxSizer(wxVERTICAL);
auto grid = new wxGridBagSizer(0, 0);

grid->Add(lblFile, wxGBPosition(0, 0), wxGBSpan(1, 1));
grid->Add(file, wxGBPosition(0, 1), wxGBSpan(1, 2));
grid->Add(browseFile, wxGBPosition(0, 3), wxGBSpan(1, 1));
grid->Add(lblSPreset, wxGBPosition(1, 0), wxGBSpan(1, 1));
grid->Add(sPreset, wxGBPosition(1, 1), wxGBSpan(1, 1));
grid->Add(lblDPreset, wxGBPosition(1, 2), wxGBSpan(1, 1));
grid->Add(dPreset, wxGBPosition(1, 3), wxGBSpan(1, 1));
grid->Add(lblSBank, wxGBPosition(2, 0), wxGBSpan(1, 1));
grid->Add(sBank, wxGBPosition(2, 1), wxGBSpan(1, 1));
grid->Add(lblDBank, wxGBPosition(2, 2), wxGBSpan(1, 1));
grid->Add(dBank, wxGBPosition(2, 3), wxGBSpan(1, 1));
grid->Add(lblDBankLSB, wxGBPosition(3, 2), wxGBSpan(1, 1));
grid->Add(dBankLSB, wxGBPosition(3, 3), wxGBSpan(1, 1));
grid->Add(xgDrums, wxGBPosition(4, 0), wxGBSpan(1, 2));
grid->Add(nolimits, wxGBPosition(4, 2), wxGBSpan(1, 2));
grid->Add(linattmod, wxGBPosition(5, 0), wxGBSpan(1, 2));
grid->Add(lindecvol, wxGBPosition(5, 2), wxGBSpan(1, 2));
grid->Add(minfx, wxGBPosition(6, 0), wxGBSpan(1, 2));
grid->Add(nofx, wxGBPosition(6, 2), wxGBSpan(1, 2));
grid->Add(norampin, wxGBPosition(7, 0), wxGBSpan(1, 2));

auto btnSizer = new wxStdDialogButtonSizer();
btnSizer->AddButton(new wxButton(this, wxID_OK));
btnSizer->AddButton(new wxButton(this, wxID_CANCEL));
btnSizer->Realize();
sizer->Add(grid, 1, wxEXPAND);
sizer->Add(btnSizer, 0, wxEXPAND);
SetSizerAndFit(sizer);

browseFile->Bind(wxEVT_BUTTON, &MIDIFontDlg::OnBrowseFont, this);

file->SetFocus();
}

bool MIDIFontDlg::ShowDlg (App& app, wxWindow* parent, BassFontConfig& font) {
MIDIFontDlg dlg(app, parent, font);
if (dlg.ShowModal()==wxID_OK) {
font.file = U(dlg.file->GetValue());
font.volume = dlg.volume->GetValue() /100.0;
font.spreset = dlg.sPreset->GetValue();
font.sbank = dlg.sBank->GetValue();
font.dpreset = dlg.dPreset->GetValue();
font.dbank = dlg.dBank->GetValue();
font.dbanklsb = dlg.dBankLSB->GetValue();
font.flags =
(dlg.xgDrums->GetValue()? BASS_MIDI_FONT_XGDRUMS :0)
| (dlg.nolimits->GetValue()? BASS_MIDI_FONT_NOLIMITS :0)
| (dlg.norampin->GetValue()? BASS_MIDI_FONT_NORAMPIN :0)
| (dlg.linattmod->GetValue()? BASS_MIDI_FONT_LINATTMOD :0)
| (dlg.lindecvol->GetValue()? BASS_MIDI_FONT_LINDECVOL :0)
| (dlg.minfx->GetValue()? BASS_MIDI_FONT_MINFX :0)
| (dlg.nofx->GetValue()? BASS_MIDI_FONT_NOFX:0);
font.font = BASS_MIDI_FontInit( font.file.c_str(), font.flags);
BASS_MIDI_FontSetVolume(font.font, font.volume);
return true;
}
return false;
}

void MIDIFontDlg::OnBrowseFont (wxCommandEvent& e) {
App& app = wxGetApp();
wxFileDialog fd(this, U(translate("MIDIFontOpen")), wxEmptyString, wxEmptyString, "SF2 Soundfont (*.sf2)|*.sf2", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
fd.SetPath(file->GetValue());
if (wxID_OK==fd.ShowModal()) {
file->SetValue(fd.GetPath());
}
}

void PreferencesDlg::OnMIDIFontAdd () {
BassFontConfig font("", 0, -1, -1, -1, 0, 0, 1, 0);
if (MIDIFontDlg::ShowDlg(app, this, font)) {
int i = lcMIDIFonts->GetItemCount();
lcMIDIFonts->InsertItem(i, MIDIFontGetDisplayName(font));
lcMIDIFonts->SetItem(i, 1, MIDIFontGetMapDesc(font));
lcMIDIFonts->SetItemPtrData(i, reinterpret_cast<uintptr_t>( new BassFontConfig(font) ));
lcMIDIFonts->Select(--i);
}
lcMIDIFonts->SetFocus();
}

void PreferencesDlg::OnMIDIFontModify () {
int selection = lcMIDIFonts->GetFirstSelected();
if (selection<0) return;
auto font = reinterpret_cast<BassFontConfig*>( lcMIDIFonts->GetItemData(selection) );
if (!font) return;
if (MIDIFontDlg::ShowDlg(app, this, *font)) {
lcMIDIFonts->SetItem(selection, 0, MIDIFontGetDisplayName(*font));
lcMIDIFonts->SetItem(selection, 1, MIDIFontGetMapDesc(*font));
}
lcMIDIFonts->SetFocus();
}

void PreferencesDlg::OnMIDIFontRemove () {
int selection = lcMIDIFonts->GetFirstSelected();
if (selection<0) return;
auto font = reinterpret_cast<BassFontConfig*>( lcMIDIFonts->GetItemData(selection) );
if (font) delete font;
lcMIDIFonts->DeleteItem(selection);
lcMIDIFonts->SetFocus();
}

void PreferencesDlg::OnMIDIFontMoveUp () {
int selection = lcMIDIFonts->GetFirstSelected();
int count = lcMIDIFonts->GetItemCount();
if (selection<1) return;
wxString text = lcMIDIFonts->GetItemText(selection -1);
wxString text2 = lcMIDIFonts->GetItemText(selection -1, 1);
uintptr_t data = lcMIDIFonts->GetItemData(selection -1);
lcMIDIFonts->InsertItem(selection+1, text);
lcMIDIFonts->SetItem(selection+1, 1, text2);
lcMIDIFonts->SetItemData(selection+1, data);
lcMIDIFonts->DeleteItem(selection -1);
lcMIDIFonts->Select(--selection);
lcMIDIFonts->Focus(selection);
}

void PreferencesDlg::OnMIDIFontMoveDown () {
int selection = lcMIDIFonts->GetFirstSelected();
int count = lcMIDIFonts->GetItemCount();
if (selection<0 || selection >= count -1) return;
wxString text = lcMIDIFonts->GetItemText(selection+1);
wxString text2 = lcMIDIFonts->GetItemText(selection+1, 1);
uintptr_t data = lcMIDIFonts->GetItemData(selection+1);
lcMIDIFonts->DeleteItem(selection +1);
lcMIDIFonts->InsertItem(selection , text);
lcMIDIFonts->SetItem(selection, 1, text2);
lcMIDIFonts->SetItemData(selection , data);
lcMIDIFonts->Select(--selection+2);
lcMIDIFonts->Focus(selection+2);
}

void PreferencesDlg::OnMIDIFontsKeyDown (wxKeyEvent& e) {
int key = e.GetKeyCode(), mod = e.GetModifiers();
if (key==WXK_UP && (mod==wxMOD_CONTROL || mod==wxMOD_SHIFT)) OnMIDIFontMoveUp();
else if (key==WXK_DOWN && (mod==wxMOD_CONTROL || mod==wxMOD_SHIFT)) OnMIDIFontMoveDown();
else e.Skip();
}

void PreferencesDlg::OnPluginListMoveUp () {
int selection = lcInputPlugins->GetFirstSelected();
int count = lcInputPlugins->GetItemCount();
if (selection<1) return;
wxString text = lcInputPlugins->GetItemText(selection -1);
int index = lcInputPlugins->GetItemData(selection -1);
bool enabled = lcInputPlugins->IsItemChecked(selection -1);
lcInputPlugins->InsertItem(selection+1, text);
lcInputPlugins->SetItemData(selection+1, index);
lcInputPlugins->CheckItem(selection+1, enabled);
lcInputPlugins->DeleteItem(selection -1);
lcInputPlugins->Select(--selection);
lcInputPlugins->Focus(selection);
}

void PreferencesDlg::OnPluginListMoveDown () {
int selection = lcInputPlugins->GetFirstSelected();
int count = lcInputPlugins->GetItemCount();
if (selection<0 || selection >= count -1) return;
wxString text = lcInputPlugins->GetItemText(selection+1);
int index = lcInputPlugins->GetItemData(selection+1);
bool enabled = lcInputPlugins->IsItemChecked(selection+1);
lcInputPlugins->DeleteItem(selection +1);
lcInputPlugins->InsertItem(selection , text);
lcInputPlugins->SetItemData(selection , index);
lcInputPlugins->CheckItem(selection , enabled);
lcInputPlugins->Select(--selection+2);
lcInputPlugins->Focus(selection+2);
}


void PreferencesDlg::OnPluginListKeyDown (wxKeyEvent& e) {
int key = e.GetKeyCode(), mod = e.GetModifiers();
if (key==WXK_UP && (mod==wxMOD_CONTROL || mod==wxMOD_SHIFT)) OnPluginListMoveUp();
else if (key==WXK_DOWN && (mod==wxMOD_CONTROL || mod==wxMOD_SHIFT)) OnPluginListMoveDown();
else e.Skip();
}

wxString MIDIFontGetDisplayName (BassFontConfig& font) {
wxString name;
BASS_MIDI_FONTINFO info;
if (BASS_MIDI_FontGetInfo(font.font, &info) && info.name && *info.name) name = UI(info.name);
else wxFileName::SplitPath(U(font.file), nullptr, &name, nullptr);
return name;
}

wxString MIDIFontGetMapDesc (BassFontConfig& font) {
string desc;
if (font.spreset<0 && font.sbank<0 && font.dpreset<0) {
desc = format("* -> {}, {:+d}", font.dbanklsb, font.dbank);
}
else if (font.spreset<0 && font.dpreset<0) {
desc = format("{}, * -> {}, {}, *", font.sbank, font.dbanklsb, font.dbank);
}
else {
desc = format("{}, {} -> {}, {}, {}", font.sbank, font.spreset, font.dbank, font.dbanklsb, font.dpreset);
}
return U(desc);
}

