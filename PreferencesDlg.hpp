#ifndef ____PREFERENCESDLG____
#define ____PREFERENCESDLG____
#include "WXWidgets.hpp"

struct PreferencesDlg: wxDialog {
struct App& app;
struct wxListbook* book;

static void ShowDlg (App& app, wxWindow* parent);
PreferencesDlg (App& app, wxWindow* parent);
};

#endif
