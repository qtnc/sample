#ifndef ____PREFERENCESDLG____
#define ____PREFERENCESDLG____
#include "../common/WXWidgets.hpp"

struct PreferencesDlg: wxDialog {
struct App& app;
struct wxListbook* book;

struct wxTextCtrl *midiSfPath;
struct wxRadioBox *openAction;
struct wxCheckBox *openFocus, *includeLoopback, *castAutoTitle;
struct wxSpinCtrl *spLRT, *spMaxMidiVoices;
struct wxListView *lcInputPlugins;

static void ShowDlg (App& app, wxWindow* parent);
PreferencesDlg (App& app, wxWindow* parent);
void makeBinds (struct ConfigBindList& binds);


void OnPluginListKeyDown (wxKeyEvent& e);
void OnPluginListMoveDown ();
void OnPluginListMoveUp ();
};

#endif
