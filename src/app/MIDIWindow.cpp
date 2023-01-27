#include "MIDIWindow.hpp"
#include "App.hpp"
#include "MainWindow.hpp"
#include "MIDIWindow.hpp"
#include "../common/bass.h"
#include "../common/bassmidi.h"
#include "../common/bass_fx.h"
#include "../common/cpprintf.hpp"
#include<string>
#include<vector>
using namespace std;

struct Preset {
int bank;
int program;
int index;
std::string name;
Preset (int b, int p, int i, const std::string& n): bank(b), program(p), index(i), name(n) {}
};
std::vector<Preset> presets;

void FillInstrumentList (wxComboBox* cb);

MIDIWindow::MIDIWindow (App& app):
wxDialog(app.win, -1, U(translate("MIDIWin")) ),
midi(0),
app(app)
{
auto sizer = new wxFlexGridSizer(4, 0, 0);

for (int i=0; i<16; i++) {
auto& ch = channels[i];
std::string chnumstr;
if (i<9) chnumstr = format("&%d", i+1);
else if (i==9) chnumstr = "1&0";
else chnumstr = std::to_string(i+1);
auto lblChannel = new wxStaticText(this, wxID_ANY, U(format(translate("MIDIChanLbl"), U(chnumstr))), wxDefaultPosition, wxDefaultSize);
ch.cbProgram = new wxComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_DROPDOWN | wxCB_READONLY);
ch.tbLockProgram = new wxToggleButton(this, wxID_ANY, U(translate("MIDILockChan")) );
ch.tbMute = new wxToggleButton(this, wxID_ANY, U(translate("MIDIMuteChan")) );
ch.program = 0;
ch.bankLSB = 0;
ch.bankMSB = 0;
ch.lock = false;
ch.mute = false;

sizer->Add(lblChannel, 0);
sizer->Add(ch.cbProgram, 0);
sizer->Add(ch.tbLockProgram, 0);
sizer->Add(ch.tbMute, 0);

ch.cbProgram->Bind(wxEVT_COMBOBOX, [=](auto&e){ OnSelectProgram(i); });
ch.tbLockProgram->Bind(wxEVT_TOGGLEBUTTON, [=](auto&e){ OnLockProgram(i); });
ch.tbMute->Bind(wxEVT_TOGGLEBUTTON, [=](auto&e){ OnMute(i); });

FillInstrumentList(ch.cbProgram);
}

btnClose = new wxButton(this, wxID_CANCEL, U(translate("Close")));

auto blank1 = new wxStaticText(this, wxID_ANY, wxEmptyString, wxPoint(-2, -2), wxSize(1, 1) );
sizer->Add(blank1, 0);
sizer->Add(btnClose, 0);

SetSizerAndFit(sizer);

btnClose->Bind(wxEVT_BUTTON, [&](auto& e){ this->OnCloseRequest(); });
channels[0].cbProgram->SetFocus();

if (app.curStreamType==BASS_CTYPE_STREAM_MIDI) {
DWORD midi = BASS_FX_TempoGetSource(app.curStream);
OnLoadMIDI(midi);
}

}

void MIDIWindow::OnCloseRequest () {
this->Hide(); 
app.win->GetMenuBar()->Check(IDM_SHOWMIDI, false);
app.win->btnPlay->SetFocus(); 
}

bool MIDIWindow::FilterEvent (DWORD handle, int track, void* e1) {
BASS_MIDI_EVENT& e = *(BASS_MIDI_EVENT*)e1;
if (e.chan>=16) return true;
auto& ch = channels[e.chan];
switch(e.event){
case MIDI_EVENT_NOTE:
return !ch.mute;
case MIDI_EVENT_BANK:
ch.bankMSB = e.param;
return !ch.lock;
case MIDI_EVENT_BANK_LSB:
ch.bankLSB = e.param;
return !ch.lock;
case MIDI_EVENT_PROGRAM:
ch.program = e.param;
return OnUpdateProgram(e.chan);
default: 
return true;
}}

BOOL CALLBACK MIDIFilterProc (HSTREAM handle, int track, BASS_MIDI_EVENT* event, BOOL seeking, void* user) {
return reinterpret_cast<MIDIWindow*>(user) ->FilterEvent(handle, track, event);
}

void MIDIWindow::OnLoadMIDI (DWORD handle) {
midi = handle;
for (int i=0; i<16; i++) {
auto& ch = channels[i];
ch.program = BASS_MIDI_StreamGetEvent(midi, i, MIDI_EVENT_PROGRAM);
ch.bankMSB = BASS_MIDI_StreamGetEvent(midi, i, MIDI_EVENT_BANK);
ch.bankLSB = BASS_MIDI_StreamGetEvent(midi, i, MIDI_EVENT_BANK_LSB);
ch.mute = false;
ch.lock = false;
ch.tbMute->SetValue(false);
ch.tbLockProgram->SetValue(false);
OnUpdateProgram(i);
}
BASS_MIDI_StreamSetFilter(midi, true, &MIDIFilterProc, this);
}

bool MIDIWindow::OnUpdateProgram (int channel) {
auto& ch = channels[channel];
if (ch.lock) return false;
RunEDT([=](){
int index = -1;
for (auto& preset: presets) {
if (preset.bank==ch.bankMSB && preset.program==ch.program) {
index = preset.index;
break;
}}
ch.cbProgram->SetSelection(index);
});
return true;
}

void MIDIWindow::OnSelectProgram (int channel) {
auto& ch = channels[channel];
auto cb = ch.cbProgram;
DWORD pb = reinterpret_cast<DWORD>( cb->GetClientData(cb->GetSelection()));
int bank = HIWORD(pb), program = LOWORD(pb);

BASS_MIDI_StreamEvent(midi, channel, MIDI_EVENT_BANK, bank);
BASS_MIDI_StreamEvent(midi, channel, MIDI_EVENT_PROGRAM, program);

ch.lock = true;
ch.tbLockProgram->SetValue(true);
}

void MIDIWindow::OnMute (int channel) {
bool mute = channels[channel].tbMute->GetValue();
if (mute) BASS_MIDI_StreamEvent(midi, channel, MIDI_EVENT_SOUNDOFF, 0);
channels[channel].mute = mute;
}

void MIDIWindow::OnLockProgram (int channel) {
auto& ch = channels[channel];
bool lock = ch.tbLockProgram->GetValue();
ch.lock = lock;
if (!lock) {
BASS_MIDI_StreamEvent(midi, channel, MIDI_EVENT_BANK, ch.bankMSB);
BASS_MIDI_StreamEvent(midi, channel, MIDI_EVENT_PROGRAM, ch.program);
OnUpdateProgram(channel);
}
}

void InitInstrumentList () {
presets.clear();
auto& app = wxGetApp();
auto fontPath = reinterpret_cast<const char*>(BASS_GetConfigPtr(BASS_CONFIG_MIDI_DEFFONT));
auto font = BASS_MIDI_FontInit(fontPath, 0);
if (!font) return;
BASS_MIDI_FONTINFO info;
if (!BASS_MIDI_FontGetInfo(font, &info)) return;
DWORD pnums[info.presets];
if (!BASS_MIDI_FontGetPresets(font, pnums)) return;
for (int i=0; i<info.presets; i++) {
int bank = HIWORD(pnums[i]);
int program = LOWORD(pnums[i]);
const char* name = BASS_MIDI_FontGetPreset(font, program, bank);
presets.emplace_back(bank, program, i, name);
println("%d, %d, %s", bank, program, name);
}
}

void FillInstrumentList (wxComboBox* cb) {
if (presets.empty()) InitInstrumentList();
cb->Freeze();
cb->Clear();
for (auto& p: presets) {
cb->Append( U(p.name), (void*)MAKELONG(p.program, p.bank));
}
cb->Thaw();
}
