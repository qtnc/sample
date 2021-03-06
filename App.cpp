#include "App.hpp"
#include "MainWindow.hpp"
#include "LevelsWindow.hpp"
#include "WorkerThread.hpp"
#include "Encoder.hpp"
#include "Caster.hpp"
#include "Loader.hpp"
#include "UniversalSpeech.h"
#include "bass.h"
#include "bass_fx.h"
#include "bassmidi.h"
#include "bassmix.h"
#include "bassenc.h"
#include "WXWidgets.hpp"
#include "stringUtils.hpp"
#include "cpprintf.hpp"
#include <wx/cmdline.h>
#include <wx/filename.h>
#include <wx/dir.h>
#include <wx/stdpaths.h>
#include <wx/textdlg.h>
#include <wx/filedlg.h>
#include <wx/dirdlg.h>
#include <wx/snglinst.h>
#include <wx/ipc.h>
#include <wx/datetime.h>
#include <wx/translation.h>
#include<string>
#include<vector>
#include<unordered_map>
using namespace std;

wxSingleInstanceChecker singleInstanceChecker;
wxIMPLEMENT_APP(App);

extern float eqBandwidths[], eqFreqs[];
extern bool BASS_SimpleInit (int device);
extern bool BASS_RecordSimpleInit (int device);
extern DWORD BASS_StreamCreateCopy (DWORD source, bool decode=true);
extern vector<pair<int,string>> BASS_GetDeviceList (bool includeLoopback = false);
extern vector<pair<int,string>> BASS_RecordGetDeviceList (bool includeLoopback = false);
extern void ldrAddAll ();

struct IPCConnection: wxConnection {
virtual bool OnExec (const wxString& topic, const wxString& data) final override {
App& app = wxGetApp();
vector<string> files = split(U(data), ";", true);
if (app.config.get("app.integration.openAction", "open")=="open") app.playlist.clear();
for (auto& file: files) app.openFileOrURL(file);
if (app.config.get("app.integration.openRefocus", false)) app.win->SetFocus();
return true;
}
};

struct IPCClient: wxClient {
virtual IPCConnection* OnMakeConnection () final override { return new IPCConnection(); }
inline IPCConnection* connect () { return static_cast<IPCConnection*>(MakeConnection("localhost", APP_NAME ".sock", APP_NAME " .ipc")); }
};

struct IPCServer: wxServer {
IPCServer (): wxServer() { Create(APP_NAME ".sock"); }
virtual IPCConnection* OnAcceptConnection (const wxString& topic) final override {
return new IPCConnection();
}};

struct CustomFileTranslationLoader: wxTranslationsLoader {
virtual wxMsgCatalog* LoadCatalog (const wxString& domain, const wxString& lang) final override {
string filename = "lang/" + U(domain) + "_" + U(lang) + ".mo";
println("Loading WXWidgets translations (domain=%s, lang=%s) in %s...", domain, lang, filename);
wxMsgCatalog* re = nullptr;
bool existing = false;
{ ifstream in(filename, ios::binary); if (in) existing=true; }
if (existing) re = wxMsgCatalog::CreateFromFile( U(filename), domain );
println(re? "Loaded WXWidgets translations (domain=%s, lang=%s) in %s" : "Couldn't load WXWidgets translations (domain=%s, lang=%s) in %s: not found", domain, lang, filename);
return re;
}
     virtual wxArrayString GetAvailableTranslations(const wxString& domain) const final override {
vector<string> langs = { "fr", "en", "it", "es", "pt", "de", "ru" };
wxArrayString v;
for (auto& s: langs) v.push_back(s);
return v;
}};

bool App::OnInit () {
initDirs();
initConfig();
initLocale();
initTranslations();
initSpeech();
initAudio();
initTags();

if (!wxApp::OnInit()) return false;

singleInstanceChecker.Create(GetAppName());
if (singleInstanceChecker.IsAnotherRunning()) {
IPCClient ipcClient;
unique_ptr<IPCConnection> con( ipcClient.connect());
if (con) {
ostringstream out;
for (int i=0, n=playlist.size(); i<n; i++) {
if (i>0) out << ';';
out << playlist[i].file;
}
con->Execute(U(out.str()));
}
return false;
}
ipcServer = new IPCServer();

if (!playlist.size()) {
string file = config.get("playlist.file", "");
playlist.load(file);
}
playNext(0);
if (playlist.curPosition>0 && curStream) BASS_ChannelSetPosition(curStream, BASS_ChannelSeconds2Bytes(curStream, playlist.curPosition / 1000.0), BASS_POS_BYTE);

win = new MainWindow(*this);
win->Show(true);
worker = new WorkerThread(*this);
worker->Run();
Bind(wxEVT_CHAR_HOOK, &App::OnGlobalCharHook, this);

return true;
}

void App::OnInitCmdLine (wxCmdLineParser& cmd) {
wxApp::OnInitCmdLine(cmd);
cmd.AddParam(wxEmptyString, wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_MULTIPLE | wxCMD_LINE_PARAM_OPTIONAL);
}

bool App::OnCmdLineParsed (wxCmdLineParser& cmd) {
for (int i=0, n=cmd.GetParamCount(); i<n; i++) openFileOrURL(UFN(cmd.GetParam(i)));
return true;
}


bool App::initDirs () {
cout << "Retrieving standard directories..." << endl;
SetAppName(APP_NAME);
SetClassName(APP_NAME);
SetVendorName(APP_VENDOR);

auto& stdPaths = wxStandardPaths::Get();
appDir = wxFileName(stdPaths.GetExecutablePath()).GetPath();
userDir = stdPaths.GetUserDataDir();
userLocalDir = stdPaths.GetUserLocalDataDir();

cout << "userDir = " << userDir << endl;
cout << "userLocalDir = " << userLocalDir << endl;
cout << "appDir = " << appDir << endl;

auto userDirFn = wxFileName::DirName(userDir);
auto userLocalDirFn = wxFileName::DirName(userLocalDir);

pathList.Add(userDir);
//pathList.Add(userLocalDir);
pathList.Add(appDir);

return 
userDirFn .Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)
&& userLocalDirFn .Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)
&& userDirFn.IsDirReadable()
&& userLocalDirFn.IsDirReadable();
}

bool App::initConfig () {
cout << "Loading user config..." << endl;
string configIniPath = UFN(pathList.FindAbsoluteValidPath(CONFIG_FILENAME));
if (configIniPath.empty()) cout << "No " << CONFIG_FILENAME << " found, fallback to defaults" << endl;
else {
cout << CONFIG_FILENAME << " found: " << configIniPath << endl;
config.setFlags(PM_BKESC);
config.load(configIniPath);
}
//todo: read config from map

string eflPath = UFN(pathList.FindAbsoluteValidPath("effects.ini"));
if (!eflPath.empty()) { ifstream in(eflPath); effects = EffectParams::read(in); }
return true;
}

string App::findWritablePath (const string& wantedPath) {
int lastSlash = wantedPath.find_last_of("/\\");
string path, file;
if (lastSlash==string::npos) { path = ""; file=wantedPath; }
else { path=wantedPath.substr(0, lastSlash); file=wantedPath.substr(lastSlash+1); }
for (int i=pathList.GetCount() -1; i>=0; i--) {
auto wxfn = wxFileName(pathList.Item(i), wxEmptyString);
if (wxfn.IsFileWritable() || wxfn.IsDirWritable()) {
string dirpath = UFN(wxfn.GetFullPath());
if (path.size()) {
if (!ends_with(dirpath, "/") && !ends_with(dirpath, "\\")) dirpath += "/";
dirpath += path;
}
if (wxFileName(dirpath, "").Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
wxfn = wxFileName(dirpath, file);
if (wxfn.IsFileWritable() || wxfn.IsDirWritable()) {
return UFN(wxfn.GetFullPath());
}}}}
return string();
}

bool App::saveConfig () {
string filename = findWritablePath(CONFIG_FILENAME);
if (filename.empty()) {
cout << "No valid writable path found to save configuration " << CONFIG_FILENAME << endl;
return false;
}
cout << "Saving configuration to " << filename << endl;
//todo: save config to map
return config.save(filename);
}

bool App::initSpeech () {
cout << "Initialing UniversalSpeech..." << endl;
int engine = config.get("speech.engine", -1);
speechSetValue(SP_ENABLE_NATIVE_SPEECH, engine>=0);
#define P(K,C) { int x = config.get("speech." #K, -1); if (x>=0) speechSetValue(C, x); }
P(engine, SP_ENGINE)
P(subengine, SP_SUBENGINE)
P(language, SP_LANGUAGE)
P(voice, SP_VOICE)
P(volume, SP_VOLUME)
P(pitch, SP_PITCH)
P(rate, SP_RATE)
P(inflexion, SP_INFLEXION)
#undef P
engine = speechGetValue(SP_ENGINE);
vector<string> engineNames = { "None", "Jaws", "WindowsEye", "NVDA", "SystemAccess", "Supernova/Dolphin", "ZoomText", "Cobra", "SAPI5" };
println("UniversalSpeech initialized: engineID=%d, engine=%s, subengine=%d, language=%d, voice=%d, volume=%d, pitch=%d, rate=%d, inflexion=%d",
engine, engineNames[engine+1], speechGetValue(SP_SUBENGINE), speechGetValue(SP_LANGUAGE), speechGetValue(SP_VOICE), speechGetValue(SP_VOLUME), speechGetValue(SP_PITCH), speechGetValue(SP_RATE), speechGetValue(SP_INFLEXION));
return true;
}

bool App::initAudio () {
cout << "Initializing BASS audio..." << endl;
BASS_SetConfig(BASS_CONFIG_UNICODE, true);
BASS_SetConfig(BASS_CONFIG_DEV_DEFAULT, true);
BASS_SetConfig(BASS_CONFIG_FLOATDSP, true);
BASS_SetConfig(BASS_CONFIG_NET_PLAYLIST, 1);
BASS_SetConfig(BASS_CONFIG_BUFFER, 150);
BASS_SetConfig(BASS_CONFIG_MIXER_BUFFER, 1);

streamVol = config.get("stream.volume", streamVol * 100.0f) / 100.0f;
previewVol = config.get("preview.volume", previewVol * 100.0f) / 100.0f;
micFbVol1 = config.get("mic1.feedback.volume", micFbVol1 * 100.0f) / 100.0f;
micFbVol2 = config.get("mic2.feedback.volume", micFbVol2 * 100.0f) / 100.0f;
streamVolInMixer = config.get("mixer.stream.volume", streamVolInMixer * 100.0f) / 100.0f;
micVol1 = config.get("mixer.mic1.volume", micVol1 * 100.0f) / 100.0f;
micVol2 = config.get("mixer.mic2.volume", micVol2 * 100.0f) / 100.0f;
loop = config.get("stream.loop", loop);
random = config.get("playlist.random", random);

cout << "Initializing BASS default device" << endl;
if (!BASS_SimpleInit(-1)) return false;

cout << "Loading BASS plugins..." << endl;
loadedPlugins.clear();
wxDir dir(appDir);
wxString dllFile;
if (dir.GetFirst(&dllFile, U("bass?*.dll"))) do {
string sFile = UFN(dllFile);
if (sFile=="bass.dll" || sFile=="bass_fx.dll" || sFile=="bassmix.dll" || starts_with(sFile, "bassenc")) continue;
auto plugin = BASS_PluginLoad(sFile.c_str(), 0);
println("Loading plugin: %s... %s", sFile, plugin?"OK":"failed");
if (plugin) loadedPlugins.push_back(plugin);
} while(dir.GetNext(&dllFile));

string defaultMidiSfPath = UFN(appDir) + "/ct8mgm.sf2";
BASS_SetConfig(BASS_CONFIG_MIDI_AUTOFONT, 2);
BASS_SetConfig(BASS_CONFIG_MIDI_VOICES, config.get("midi.voices.max", 256)); 
BASS_SetConfigPtr(BASS_CONFIG_MIDI_DEFFONT, defaultMidiSfPath.c_str());
//BASS_SetConfigPtr(BASS_CONFIG_MIDI_DEFFONT, "C:\\Temp\\soundbanks\\arachno-soundfont-10-sf2\\ArachnoSoundFont-Version1.0.sf2");
BASS_SetConfigPtr(BASS_CONFIG_MIDI_DEFFONT, config.get("midi.soundfont.path", defaultMidiSfPath).c_str());
config.set("midi.soundfont.path", reinterpret_cast<const char*>(BASS_GetConfigPtr(BASS_CONFIG_MIDI_DEFFONT)));
println("Default MIDI soundfont = %s", reinterpret_cast<const char*>(BASS_GetConfigPtr(BASS_CONFIG_MIDI_DEFFONT)));

auto deviceList = BASS_GetDeviceList(true);
initAudioDevice(streamDevice, "stream.device", deviceList, BASS_SimpleInit, BASS_GetDevice);
initAudioDevice(previewDevice, "preview.device", deviceList, BASS_SimpleInit, BASS_GetDevice);
initAudioDevice(micFbDevice1, "mic1.feedback.device", deviceList, BASS_SimpleInit, BASS_GetDevice);
initAudioDevice(micFbDevice2, "mic2.feedback.device", deviceList, BASS_SimpleInit, BASS_GetDevice);
deviceList = BASS_RecordGetDeviceList(true);
initAudioDevice(micDevice1, "mic1.device", deviceList, BASS_RecordSimpleInit, BASS_RecordGetDevice);
initAudioDevice(micDevice2, "mic2.device", deviceList, BASS_RecordSimpleInit, BASS_RecordGetDevice);

cout << "BASS audio initialized and configured" << endl;
return true;
}

bool App::initAudioDevice (int& device, const string& configName, const vector<pair<int,string>>& deviceList, function<bool(int)> init, function<int()> getDefault) {
string sConf = config.get(configName, "default");
int iFound = find_if(deviceList.begin(), deviceList.end(), [&](auto& p){ return iequals(p.second, sConf); }) -deviceList.begin();
if (iFound>=0 && iFound<deviceList.size() && init(deviceList[iFound].first)) device = deviceList[iFound].first;
if (device<0) device = getDefault();
if (device>=0) println("%s set to %s (%d)", configName, deviceList[iFound].second, device);
else println("%s set to default/undefined (%d)", configName, device);
return true;
}

bool App::initLocale () {
cout << "Initializing locale..." << endl;
locale = config.get("locale", "default");
wxlocale = new wxLocale();
if (locale=="default") {
cout << "No locale set in the configuration, retrieving system default" << endl;
wxlocale->Init();
}
else {
auto info = wxLocale::FindLanguageInfo(U(locale));
if (info) wxlocale->Init(info->Language);
else cout << "Couldn't find locale information for " << locale << endl;
}
this->locale = U(wxlocale->GetCanonicalName());
auto& translations = *wxTranslations::Get();
translations.SetLoader(new CustomFileTranslationLoader());
translations.AddStdCatalog();
cout << "Locale configured to " << locale << endl;
return true;
}

bool App::initTranslations () {
cout << "Loading translations..." << endl;
vector<string> locales = {
config.get("locale", locale),
config.get("locale", locale).substr(0, 5),
config.get("locale", locale).substr(0, 2),
locale,
locale.substr(0, 5),
locale.substr(0, 2),
"en"
};
for (string& l: locales) {
string transPath = UFN(pathList.FindAbsoluteValidPath(format("lang/sample_%s.properties", l)));
if (!transPath.empty()) {
cout << "Translations found for locale " << l << " in " << transPath << endl;
lang.setFlags(PM_BKESC);
lang.load(transPath);
break;
}}
return true;
}

void App::changeLocale (const string& s) {
auto info = wxLocale::FindLanguageInfo(U(s));
if (!info) {
cout << "Couldn't change locale to " << s << ", no locale information found" << endl;
return;
}
println("Changing language to %s...", U(info->CanonicalName));
locale = U(info->CanonicalName);
config.set("locale", locale);
initLocale();
initTranslations();
//todo: other consequences of changing locale
}

void App::openFileOrURL (const std::string& s) {
auto& entry = playlist.add(s);
if (playlist.size()==1) playNext();
}

DWORD App::loadFileOrURL (const std::string& s, bool loop, bool decode) {
if (string::npos!=s.find(':', 2)) return loadURL(s, loop, decode);
else return loadFile(s, loop, decode);
}

DWORD loadUsingLoaders (const string& file, DWORD flags) {
if (!Loader::loaders.size()) ldrAddAll();
for (auto& loader: Loader::loaders) {
DWORD stream = loader->load(file, flags);
if (stream) return stream;
}
return 0;
}

DWORD App::loadFile (const std::string& file, bool loop, bool decode) {
DWORD flags = (loop? BASS_SAMPLE_LOOP : 0) | (decode? BASS_STREAM_DECODE : BASS_STREAM_AUTOFREE);
DWORD stream = BASS_StreamCreateFile(false, file.c_str(), 0, 0, flags | BASS_STREAM_PRESCAN);
//if (!stream) stream = BASS_MusicLoad(false, file.c_str(), 0, 0, flags | BASS_MUSIC_SINCINTER | BASS_MUSIC_POSRESET | BASS_MUSIC_POSRESETEX | BASS_MUSIC_PRESCAN, 48000);
if (!stream) stream = loadUsingLoaders(file, flags);
return stream;
}

DWORD App::loadURL (const std::string& url, bool loop, bool decode) {
DWORD flags = (loop? BASS_SAMPLE_LOOP : 0) | (decode? BASS_STREAM_DECODE : BASS_STREAM_AUTOFREE);
DWORD stream = BASS_StreamCreateURL(url.c_str(), 0, flags, nullptr, nullptr);
if (!stream) stream = loadUsingLoaders(url, flags);
return stream;
}

static void CALLBACK streamSyncEnd (HSYNC sync, DWORD chan, DWORD data, void* udata) {
App& app = *reinterpret_cast<App*>(udata);
app.OnStreamEnd();
}

static void plugCurStreamToMix (App& app) {
app.curStreamInMixer = BASS_StreamCreateCopy(app.curStream, true);
BASS_ChannelSetAttribute(app.curStreamInMixer, BASS_ATTRIB_VOL, app.streamVolInMixer);
BASS_Mixer_StreamAddChannel(app.mixHandle, app.curStreamInMixer, BASS_MIXER_DOWNMIX | BASS_MIXER_LIMIT | BASS_STREAM_AUTOFREE);
//if (app.micHandle1) BASS_Mixer_StreamAddChannel(app.mixHandle, app.micHandle1, BASS_MIXER_DOWNMIX | BASS_STREAM_AUTOFREE);
//if (app.micHandle2) BASS_Mixer_StreamAddChannel(app.mixHandle, app.micHandle2, BASS_MIXER_DOWNMIX | BASS_STREAM_AUTOFREE);
}

static void CALLBACK BPMUpdateProc (DWORD handle, float bpm, void* udata) {
App& app = *reinterpret_cast<App*>(udata);
app.curStreamBPM = bpm;
}

void App::playAt (int index) {
if (curStream) BASS_ChannelStop(curStream);
playlist.curIndex=index;
DWORD loopFlag = loop? BASS_SAMPLE_LOOP : 0;
BASS_SetDevice(streamDevice);
DWORD stream = loadFileOrURL(playlist[index].file, false, true);
if (!stream) {
string file = playlist.current().file;
playlist.erase();
bool re = playlist.load(file);
playNext(0);
return;
}
BASS_CHANNELINFO ci;
BASS_ChannelGetInfo(stream, &ci);
curStreamVoicesMax = 0;
curStreamRowMax = 0;
curStreamBPM = 0;
curStreamType = ci.ctype;
curStream = BASS_FX_TempoCreate(stream, loopFlag | BASS_FX_FREESOURCE | BASS_STREAM_AUTOFREE);
curStreamEqFX = BASS_ChannelSetFX(curStream, BASS_FX_BFX_PEAKEQ, 0);
BASS_FX_BPM_CallbackSet(curStream, &BPMUpdateProc, 8, 0x1000030, 0, this);
for (int i=0; i<7; i++) { BASS_BFX_PEAKEQ p = { i, eqBandwidths[i], 0, eqFreqs[i], 0, -1 }; BASS_FXSetParameters(curStreamEqFX, &p); }
for (auto& effect: effects) { effect.handle=0; applyEffect(effect); }
BASS_ChannelSetSync(curStream, BASS_SYNC_END, 0, streamSyncEnd, this);
BASS_ChannelSetAttribute(curStream, BASS_ATTRIB_VOL, streamVol);
BASS_ChannelPlay(curStream, false);
playlist.current().length = BASS_ChannelBytes2Seconds(curStream, BASS_ChannelGetLength(curStream, BASS_POS_BYTE));
playlist.current().loadTagsFromBASS(stream);
if (mixHandle) plugCurStreamToMix(*this);
if (win) win->OnTrackChanged();
}

void App::saveEncode (PlaylistItem& item, const std::string& file, Encoder& encoder) {
auto& app = *this;
worker->submit([=, &encoder, &item, &app](){
println("Saving %s...", file);
DWORD source = loadFileOrURL(item.file, false, true);
println("source=%p", source);
if (!source) return;
DWORD encoderHandle = encoder.startEncoderFile(item, source, file);
println("encoderHandle=%p", encoderHandle);
auto lastSlash = file.find_last_of("/\\");
int read=0, length = BASS_ChannelGetLength(source, BASS_POS_BYTE);
string sFile = file.substr(lastSlash==string::npos? 0 : lastSlash+1);
println("sFile=%s, length=%d", sFile, length);
unique_ptr<char[]> buffer = make_unique<char[]>(65536);
win->openProgress(format(translate("Saving"), sFile), format(translate("Saving"), sFile), length);
for (int count=0; count<length; ) {
read = BASS_ChannelGetData(source, &buffer[0], 65536);
if (read<0 || win->isProgressCancelled()) break;
count += read;
win->setProgressTitle(format(translate("SavingWP"), sFile, static_cast<int>(100.0 * count / length)));
win->updateProgress(count);
}
win->closeProgress();
BASS_StreamFree(source);
});}

void App::startMix () {
if (mixHandle) return;
BASS_SimpleInit(0);
BASS_SetDevice(0);
mixHandle = BASS_Mixer_StreamCreate(44100, 2, BASS_SAMPLE_FLOAT | BASS_STREAM_AUTOFREE | BASS_MIXER_NONSTOP);
BASS_ChannelSetAttribute(mixHandle, BASS_ATTRIB_VOL, 0);
BASS_ChannelPlay(mixHandle, false);
plugCurStreamToMix(*this);
}

void App::stopMix () {
BASS_ChannelStop(mixHandle);
mixHandle = 0;
}

void App::tryStopMix () {
if (!encoderHandle && !micHandle1 && !micHandle2) stopMix();
}

void App::stopCaster () {
tryStopMix();
BASS_Encode_Stop(encoderHandle);
if (encoderHandle) {
App& app = *this;
speechSay(U(translate("CastStopped")).wc_str(), true);
}
encoderHandle = 0;
}

static void CALLBACK encoderNotification (HENCODE encoderHandle, DWORD status, void* udata) {
if (status==BASS_ENCODE_NOTIFY_CAST || status==BASS_ENCODE_NOTIFY_ENCODER || status==BASS_ENCODE_NOTIFY_FREE) {
App& app = wxGetApp();
app .stopCaster();
speechSay(U(translate("CastStopped")).wc_str(), true);
}}

void App::startCaster (Caster& caster, Encoder& encoder, const string& server, const string& port, const string& user, const string& pass, const string& mount) {
stopCaster();
startMix();
encoderHandle = caster.startCaster(mixHandle, encoder, server, port, user, pass, mount);
if (encoderHandle) {
BASS_Encode_SetNotify(encoderHandle, &encoderNotification, nullptr);
App& app = *this;
speechSay(U(translate("CastStarted")).wc_str(), true);
}
else stopCaster();
}

void App::stopMic (int n) {
DWORD* mhs[] = { &micHandle1, &micHandle2 };
DWORD* mfhs[] = { &micFbHandle1, &micFbHandle2 };
DWORD &mic = *mhs[n -1], &micFb = *mfhs[n -1];
println("Stop mic ! mic=%p, fb=%p", mic, micFb);
if (micFb) BASS_ChannelStop(micFb);
if (mic) BASS_ChannelStop(mic);
if (mixHandle && mic) BASS_Mixer_ChannelRemove(mic);
mic = micFb = 0;
}

bool App::startMic (int n) {
DWORD* mhs[] = { &micHandle1, &micHandle2 };
DWORD* mfhs[] = { &micFbHandle1, &micFbHandle2 };
int* mdevs[] = { &micDevice1, &micDevice2 };
int* mfdevs[] = { &micFbDevice1, &micFbDevice2 };
float* mvols[] = { &micVol1, &micVol2 };
float* mfvols[] = { &micFbVol1, &micFbVol2 };
DWORD &mic = *mhs[n -1], &micFb = *mfhs[n -1];
int &device = *mdevs[n -1], &fbDevice = *mfdevs[n -1];
float &vol = *mvols[n -1], &fbVol = *mfvols[n -1];
if (mic || micFb) stopMic(n);
println("Starting mic ! ");
if (!BASS_RecordSimpleInit(device) || !BASS_SimpleInit(fbDevice)) return false;
println("Start recording");
BASS_RecordSetDevice(device);
mic = BASS_RecordStart(0, 0, BASS_SAMPLE_FLOAT, nullptr, nullptr);
println("mic=%p, vol=%g", mic, vol);
if (!mic) return false;
BASS_SetDevice(fbDevice);
micFb = BASS_StreamCreateCopy(mic, false);
println("micFb=%p, vol=%g", micFb, fbVol);
auto re2 = BASS_ChannelSetAttribute(micFb, BASS_ATTRIB_VOL, fbVol);
println("re2=%d, error=%d", re2, BASS_ErrorGetCode());
auto re3 = BASS_ChannelSetAttribute(mic, BASS_ATTRIB_VOL, vol);
println("re3=%d, error=%d", re3, BASS_ErrorGetCode());
auto re4 = BASS_ChannelPlay(micFb, false);
println("re4=%d, error=%d", re4, BASS_ErrorGetCode());
startMix();
auto re = BASS_Mixer_StreamAddChannel(mixHandle, mic, BASS_MIXER_DOWNMIX | BASS_STREAM_AUTOFREE);
println("Add mic to mix, mix=%p, re=%d, error=%d", mixHandle, re, BASS_ErrorGetCode());
return true;
}

bool App::changeStreamDevice (int device) {
if (!BASS_SimpleInit(device)) return false;
if (curStream && !BASS_ChannelSetDevice(curStream, device)) return false;
streamDevice = device;
return true;
}

bool App::changePreviewDevice (int device) {
if (!BASS_SimpleInit(device)) return false;
if (curPreviewStream && !BASS_ChannelSetDevice(curPreviewStream, device)) return false;
previewDevice = device;
return true;
}

bool App::changeMicFeedbackDevice (int device, int n) {
int* mfds[] = { &micFbDevice1, &micFbDevice2 };
DWORD* mfhs[] = { &micFbHandle1, &micFbHandle2 };
DWORD &micFb = *mfhs[n -1];
int& micFbDevice = *mfds[n -1];
if (!BASS_SimpleInit(device)) return false;
if (micFb && !BASS_ChannelSetDevice(micFb, device)) return false;
micFbDevice = device;
return true;
}

bool App::changeMicDevice (int device, int n) {
DWORD* mhs[] = { &micHandle1, &micHandle2 };
DWORD* mfhs[] = { &micFbHandle1, &micFbHandle2 };
int* mfds[] = { &micDevice1, &micDevice2 };
int& micDevice = *mfds[n -1];
DWORD &mic = *mhs[n -1], &micFb = *mfhs[n -1];
if (!BASS_RecordSimpleInit(device)) return false;
micDevice = device;
if (mic || micFb) startMic(n);
return true;
}

void App::changePreviewVol (float vol, bool updateLevel, bool updatePreview) {
previewVol = vol;
if (curPreviewStream) BASS_ChannelSetAttribute(curPreviewStream, BASS_ATTRIB_VOL, vol);
if (updateLevel && win && win->levelsWindow) win->levelsWindow->slPreviewVol->SetValue(100 * vol);
if (updatePreview && win && win->slPreviewVolume) win->slPreviewVolume->SetValue(100 * vol);
}

void App::changeMicFeedbackVol (float vol, int n, bool update) {
DWORD* mics[] = { &micFbHandle1, &micFbHandle2 };
DWORD& mic = *mics[n -1];
if (mic) BASS_ChannelSetAttribute(mic, BASS_ATTRIB_VOL, vol);
float* vols[] = { &micFbVol1, &micFbVol2 };
*vols[n -1] = vol;
if (update && win && win->levelsWindow) {
wxSlider* sls[] = { win->levelsWindow->slMicFbVol1, win->levelsWindow->slMicFbVol2 };
sls[n -1]->SetValue(100*vol);
}}

void App::changeMicMixVol (float vol, int n, bool update) {
DWORD* mics[] = { &micHandle1, &micHandle2 };
DWORD& mic = *mics[n -1];
if (mic) BASS_ChannelSetAttribute(mic, BASS_ATTRIB_VOL, vol);
float* vols[] = { &micVol1, &micVol2 };
*vols[n -1] = vol;
if (update && win && win->levelsWindow) {
wxSlider* sls[] = { win->levelsWindow->slMicVol1, win->levelsWindow->slMicVol2 };
sls[n -1]->SetValue(100*vol);
}}

void App::changeStreamMixVol (float vol, bool update) {
streamVolInMixer = vol;
if (curStreamInMixer) BASS_ChannelSetAttribute(curStreamInMixer, BASS_ATTRIB_VOL, vol);
if (update && win && win->levelsWindow) win->levelsWindow->slStreamVolInMixer->SetValue(100 * vol);
}

void App::applyEffect (EffectParams& effect) {
if (effect.on) {
if (effect.handle = BASS_ChannelSetFX(curStream, effect.fxType, 0))
BASS_FXSetParameters(effect.handle, effect.data());
}
else if (effect.handle) {
BASS_ChannelRemoveFX(curStream, effect.handle);
effect.handle=0;
}}

void App::OnStreamEnd () {
if (!loop) playNext();
}

void App::OnGlobalCharHook (wxKeyEvent& e) {
auto k = e.GetKeyCode(), mods = e.GetModifiers();
e.Skip();
if (curPreviewStream && win && win->slPreviewVolume && win->slPreviewPosition) {
if (mods==wxMOD_NONE && k==WXK_F8) pausePreview();
else if (mods==wxMOD_NONE && k==WXK_F6) seekPreview(-5, false, true);
else if (mods==wxMOD_NONE && k==WXK_F7) seekPreview(5, false, true);
else if (mods==wxMOD_NONE && k==WXK_F11) changePreviewVol(std::max(0.0f, previewVol -0.025f), true);
else if (mods==wxMOD_NONE && k==WXK_F12 ) changePreviewVol(std::min(1.0f, previewVol +0.025f), true);
}}

void App::pausePreview () {
if (!curPreviewStream && !curPreviewFile.empty()) { playPreview(curPreviewFile); return; }
else if (!curPreviewStream) return;
if (BASS_ChannelIsActive(curPreviewStream)==BASS_ACTIVE_PLAYING) BASS_ChannelPause(curPreviewStream);
else BASS_ChannelPlay(curPreviewStream, false);
}

void App::stopPreview () {
if (curPreviewStream) BASS_ChannelStop(curPreviewStream);
curPreviewStream = 0;
curPreviewFile.clear();
}

void App::playPreview (const std::string& file) {
stopPreview();
if (file.empty()) return;
curPreviewFile = file;
BASS_SetDevice(previewDevice);
curPreviewStream = loadFileOrURL(curPreviewFile);
if (!curPreviewStream) return;
BASS_ChannelSetAttribute(curPreviewStream, BASS_ATTRIB_VOL, previewVol);
BASS_ChannelPlay(curPreviewStream, false);
if (win && win->slPreviewPosition) {
auto byteLen = BASS_ChannelGetLength(curPreviewStream, BASS_POS_BYTE);
int secLen = BASS_ChannelBytes2Seconds(curPreviewStream, byteLen);
win->slPreviewPosition->SetRange(0, secLen);
win->slPreviewPosition->SetLineSize(5);
win->slPreviewPosition->SetPageSize(30);
}}

void App::seekPreview (int pos, bool abs, bool updateOpen) {
if (!curPreviewStream) return;
if (!abs) pos += BASS_ChannelBytes2Seconds(curPreviewStream, BASS_ChannelGetPosition(curPreviewStream, BASS_POS_BYTE));
BASS_ChannelSetPosition(curPreviewStream, BASS_ChannelSeconds2Bytes(curPreviewStream, pos), BASS_POS_BYTE);
if (updateOpen && win && win->slPreviewPosition) win->slPreviewPosition->SetValue(pos);
}

void App::OnQuit  () {
if (curStream) {
playlist.curPosition = 1000 * BASS_ChannelBytes2Seconds(curStream, BASS_ChannelGetPosition(curStream, BASS_POS_BYTE));
BASS_ChannelStop(curStream);
}
string file = playlist.file.size()? playlist.file : findWritablePath(APP_NAME "_auto_save.pls");
if (!playlist.save(file)) file = findWritablePath(APP_NAME "_auto_save.pls");
if (!playlist.save(file)) file.clear();
if (file.empty()) config.erase("playlist.file");
else config.set("playlist.file", playlist.file);
config.set("stream.volume", 100.0f * streamVol);
config.set("preview.volume", 100.0f * previewVol);
config.set("mic1.feedback.volume", 100.0f * micFbVol1);
config.set("mic2.feedback.volume", 100.0f * micFbVol2);
config.set("mixer.stream.volume", 100.0f * streamVolInMixer);
config.set("mixer.mic1.volume", 100.0f * micVol1);
config.set("mixer.mic2.volume", 100.0f * micVol2);

auto list = BASS_GetDeviceList(true);
auto find = [&](int x){ for (auto& p: list) if (p.first==x) return p.second; return string(); };
if (streamDevice>=0) config.set("stream.device", find(streamDevice));
if (previewDevice>=0) config.set("preview.device", find(previewDevice));
if (micFbDevice1>=0) config.set("mic1.feedback.device", find(micFbDevice1));
if (micFbDevice2>=0) config.set("mic2.feedback.device", find(micFbDevice2));
list = BASS_RecordGetDeviceList(true);
if (micDevice1>=0)  config.set("mic1.device", find(micDevice1));
if (micDevice2>=0)  config.set("mic2.device", find(micDevice2));

config.set("stream.loop", loop);
config.set("playlist.random", random);
saveConfig();
delete ipcServer;
}


