#ifndef ____LVWINDOW_1___
#define ____LVWINDOW_1___
#include "../common/WXWidgets.hpp"
#include <wx/tglbtn.h>

struct LevelsWindow: wxDialog {
struct App& app;
struct wxComboBox *cbStreamDevice, *cbPreviewDevice, *cbMicFbDevice1, *cbMicFbDevice2, *cbMicDevice1, *cbMicDevice2;
struct wxSlider *slStreamVol, *slPreviewVol, *slMicFbVol1, *slMicFbVol2, *slStreamVolInMixer, *slMicVol1, *slMicVol2;
struct wxToggleButton *tbMic1, *tbMic2;
struct wxButton *btnClose;

LevelsWindow (App& app);

void OnCloseRequest ();
void updateLists ();

void OnChangeStreamDevice (wxCommandEvent& e);
void OnChangePreviewDevice (wxCommandEvent& e);
void OnChangeMicDevice (wxCommandEvent& e, int n);
void OnChangeMicFeedbackDevice (wxCommandEvent& e, int n);
};

#endif
