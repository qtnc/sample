#include "MIDIWindow.hpp"
#include "App.hpp"
#include "MainWindow.hpp"
#include "MIDIWindow.hpp"
#include "../common/bass.h"
#include "../common/bassmidi.h"
#include "../common/bass_fx.h"
#include<fmt/format.h>
#include<string>
#include<vector>
#include<map>
#include "../common/println.hpp"
using namespace std;
using fmt::format;

struct Preset {
DWORD program;
int index;
std::string name;
Preset (DWORD p=0, int i=-1, const std::string& n=""): program(p), index(i), name(n) {}
};
std::map<DWORD, Preset> presets;

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
if (i<9) chnumstr = format("&{}", i+1);
else if (i==9) chnumstr = "1&0";
else chnumstr = std::to_string(i+1);
auto lblChannel = new wxStaticText(this, wxID_ANY, U(format(translate("MIDIChanLbl"), chnumstr)), wxDefaultPosition, wxDefaultSize);
ch.cbProgram = new wxComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_DROPDOWN | wxCB_READONLY);
ch.tbLockProgram = new wxToggleButton(this, wxID_ANY, U(translate("MIDILockChan")) );
ch.tbMute = new wxToggleButton(this, wxID_ANY, U(translate("MIDIMuteChan")) );
ch.program = 0;
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
ch.program = (ch.program&0x00FFFF) | ((e.param&0xFF)<<16);
return !ch.lock;
case MIDI_EVENT_BANK_LSB:
ch.program = (ch.program&0xFF00FF) | ((e.param&0xFF)<<8);
return !ch.lock;
case MIDI_EVENT_DRUMS:
ch.program = (ch.program&0x7FFFFF) | (e.param? 0x800000 : 0);
return !ch.lock;
case MIDI_EVENT_PROGRAM:
ch.program = (ch.program&0xFFFF00) | (e.param&0xFF);
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
DWORD program = BASS_MIDI_StreamGetEvent(midi, i, MIDI_EVENT_PROGRAM);
DWORD bankMSB = BASS_MIDI_StreamGetEvent(midi, i, MIDI_EVENT_BANK);
DWORD bankLSB = BASS_MIDI_StreamGetEvent(midi, i, MIDI_EVENT_BANK_LSB);
DWORD drums = BASS_MIDI_StreamGetEvent(midi, i, MIDI_EVENT_DRUMS);
ch.program = (program&0xFF) | ((bankLSB&0xFF)<<8) | ((bankMSB&0xFF)<<16) | (drums? 0x800000 : 0);
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
auto it = presets.find(ch.program);
if (it==presets.end()) it = presets.find(ch.program&0xFF00FF);
if (it==presets.end()) it = presets.find(ch.program&0x00FFFF);
if (it==presets.end()) it = presets.find(ch.program&0x0000FF);
int index = it==presets.end()? -1 : it->second.index;
ch.cbProgram->SetSelection(index);
});
return true;
}

void MIDIWindow::OnSelectProgram (int channel) {
auto& ch = channels[channel];
auto cb = ch.cbProgram;
DWORD pb = reinterpret_cast<uintptr_t>( cb->GetClientData(cb->GetSelection()));
int program = pb&0xFF, bankLSB = (pb>>8)&0xFF, bankMSB = (pb>>16)&0xFF, drums = (pb>>23)&1;

BASS_MIDI_StreamEvent(midi, channel, MIDI_EVENT_DRUMS, drums);
BASS_MIDI_StreamEvent(midi, channel, MIDI_EVENT_BANK, bankMSB);
BASS_MIDI_StreamEvent(midi, channel, MIDI_EVENT_BANK_LSB, bankLSB);
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
int program = ch.program&0xFF, bankLSB = (ch.program>>8)&0xFF, bankMSB = (ch.program>>16)&0xFF, drums = (ch.program>>23)&1;
BASS_MIDI_StreamEvent(midi, channel, MIDI_EVENT_DRUMS, drums);
BASS_MIDI_StreamEvent(midi, channel, MIDI_EVENT_BANK, bankMSB);
BASS_MIDI_StreamEvent(midi, channel, MIDI_EVENT_BANK_LSB, bankLSB);
BASS_MIDI_StreamEvent(midi, channel, MIDI_EVENT_PROGRAM, program);
OnUpdateProgram(channel);
}
}

void InitInstrumentList () {
presets.clear();

int nFonts = BASS_MIDI_StreamGetFonts(0, (BASS_MIDI_FONTEX*)nullptr, 0);
println("MIDI config {} fonts", nFonts);
BASS_MIDI_FONTEX fonts[nFonts];
if (!BASS_MIDI_StreamGetFonts(0, fonts, nFonts | BASS_MIDI_FONT_EX)) return;
for (int i=nFonts -1; i>=0; i--) {
auto& font = fonts[i];
if (font.spreset!=-1) {
DWORD program = (font.dpreset&0xFF) | ((font.dbanklsb&0xFF)<<8) | ((font.dbank&0xFF)<<16);
const char* name = BASS_MIDI_FontGetPreset(font.font, font.spreset, font.sbank);
if (!name) continue;
presets[program] = Preset(program, -1, name);
}
else {
BASS_MIDI_FONTINFO info;
if (!BASS_MIDI_FontGetInfo(font.font, &info)) continue;
DWORD pnums[info.presets];
if (!BASS_MIDI_FontGetPresets(font.font, pnums)) continue;
for (size_t j=0; j<info.presets; j++) {
int bank = HIWORD(pnums[j]), program = LOWORD(pnums[j]);
if (font.sbank!=-1 && font.sbank!=bank) continue;
const char* name = BASS_MIDI_FontGetPreset(font.font, program, bank);
if (!name) continue;
DWORD dprogram = (program&0xFF) | ((font.dbanklsb&0xFF)<<8) | (((bank+font.dbank)&0xFF)<<16);
presets[dprogram] = Preset(dprogram, -1, name);
}
}}
}

void FillInstrumentList (wxComboBox* cb) {
if (presets.empty()) InitInstrumentList();
int index = 0;
cb->Freeze();
cb->Clear();
for (auto& e: presets) {
auto& p = e.second;
p.index = index++;
std::string name = format("{} ({},{},{})", p.name, p.program>>16, (p.program>>8)&0xFF, p.program&0xFF);
cb->Append( U(name), reinterpret_cast<void*>(static_cast<intptr_t>(e.first)));
}
cb->Thaw();
}
