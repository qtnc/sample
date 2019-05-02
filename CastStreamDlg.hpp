#ifndef ___CASTSTREAMDLG___
#define ___CASTSTREAMDLG___
#include "WXWidgets.hpp"

struct CastStreamDlg: wxDialog {
wxTextCtrl *tfServer, *tfPort, *tfMount, *tfUser, *tfPass;
wxComboBox *cbServerType, *cbFormat;
wxButton *btnFormat;

CastStreamDlg (App& app, wxWindow* parent);

void OnFormatOptions (wxCommandEvent& e);
void OnFormatChange (wxCommandEvent& e);
void OnServerTypeChange (wxCommandEvent& e);
};

#endif
