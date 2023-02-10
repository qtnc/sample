#ifndef ____PREFERENCESDLG____
#define ____PREFERENCESDLG____
#include "../common/WXWidgets.hpp"

struct PreferencesDlg: wxDialog {
struct App& app;
struct wxListbook* book;

struct wxRadioBox *openAction;
struct wxCheckBox *openFocus, *includeLoopback, *castAutoTitle;
struct wxSpinCtrl *spLRT, *spMaxMidiVoices;
struct wxListView *lcInputPlugins, *lcMIDIFonts;

static bool ShowDlg (App& app, wxWindow* parent);
PreferencesDlg (App& app, wxWindow* parent);
void makeBinds (struct ConfigBindList& binds);

void OnPluginListKeyDown (wxKeyEvent& e);
void OnPluginListMoveUp ();
void OnPluginListMoveUp (wxCommandEvent& e) { OnPluginListMoveUp(); }
void OnPluginListMoveDown ();
void OnPluginListMoveDown (wxCommandEvent& e) { OnPluginListMoveDown(); }
void OnMIDIFontsKeyDown (wxKeyEvent& e);
void OnMIDIFontMoveUp ();
void OnMIDIFontMoveUp (wxCommandEvent& e) { OnMIDIFontMoveUp(); }
void OnMIDIFontMoveDown ();
void OnMIDIFontMoveDown (wxCommandEvent& e) { OnMIDIFontMoveDown(); }
void OnMIDIFontAdd ();
void OnMIDIFontAdd (wxCommandEvent& e) { OnMIDIFontAdd(); }
void OnMIDIFontModify ();
void OnMIDIFontModify (wxCommandEvent& e) { OnMIDIFontModify(); }
void OnMIDIFontRemove ();
void OnMIDIFontRemove (wxCommandEvent& e) { OnMIDIFontRemove(); }
};

struct MIDIFontDlg: wxDialog {
struct BassFontConfig& font;

struct wxTextCtrl *file;
struct wxSpinCtrl *sPreset, *sBank, *dPreset, *dBank, *dBankLSB, *volume;
struct wxCheckBox *xgDrums, *linattmod, *lindecvol, *nolimits, *norampin, *minfx, *nofx;
struct wxButton *browseFile;

MIDIFontDlg (App& app, wxWindow* parent, BassFontConfig& font);
void OnBrowseFont (wxCommandEvent& e);
static bool  ShowDlg (App& app, wxWindow* parent, BassFontConfig& font);
};

#endif
