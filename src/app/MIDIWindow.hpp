#ifndef ____MIDIWINDOW_1___
#define ____MIDIWINDOW_1___
#include "../common/WXWidgets.hpp"
#include <wx/tglbtn.h>

struct MIDIWindow: wxDialog {
struct App& app;
DWORD midi;

struct {
struct wxComboBox *cbProgram;
struct wxToggleButton *tbLockProgram, *tbMute;
DWORD originalProgram, selectedProgram;
bool lock, mute;
} channels[16];

struct wxButton *btnClose;
std::string input = "";
long lastInputTime = 0;

MIDIWindow (App& app);

void OnCloseRequest ();
void OnLoadMIDI (DWORD);
bool OnUpdateProgram (int channel);
void OnSelectProgram (int channel);
void OnMute (int channel);
void OnLockProgram (int channel);
void OnProgramListKeyChar (wxKeyEvent& e, int channel);

bool FilterEvent (DWORD handle, int track, void* event);
};

#endif
