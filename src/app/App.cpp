#include "App.hpp"
#include "MainWindow.hpp"
#include "LevelsWindow.hpp"
#include "MIDIWindow.hpp"
#include "WorkerThread.hpp"
#include "Tags.hpp"
#include "../encoder/Encoder.hpp"
#include "../caster/Caster.hpp"
#include "../loader/Loader.hpp"
#include "../common/UniversalSpeech.h"
#include "../common/bass.h"
#include "../common/bass_fx.h"
#include "../common/bassmidi.h"
#include "../common/bassmix.h"
#include "../common/bassenc.h"
#include "../common/WXWidgets.hpp"
#include "../common/stringUtils.hpp"
#include "../common/println.hpp"
#include <fmt/format.h>
#include <wx/cmdline.h>
#include <wx/filename.h>
#include <wx/dir.h>
#include <wx/wfstream.h>
#include <wx/stdstream.h>
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
using fmt::format;

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
if (toml::find_or(app.config, "app", "integration", "openAction", "open")=="open") app.playlist.clear();
for (auto& file: files) app.openFileOrURL(file);
if (toml::find_or(app.config, "app", "integration", "openRefocus", false)) app.win->SetFocus();
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
auto& stdPaths = wxStandardPaths::Get();
wxString basePath = wxFileName(stdPaths.GetExecutablePath()).GetPath();
wxString filename = basePath + "/lang/" + domain + "_" + lang + ".mo";
wxMsgCatalog* re = nullptr;
bool existing = wxFile::Exists(filename);
if (existing) re = wxMsgCatalog::CreateFromFile( U(filename), domain );
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
std::string file = toml::find_or(config, "playlist", "file", "");
playlist.load(file);
}
playNext(0);
if (playlist.curSubindex>0 && curStream) BASS_ChannelSetPosition(BASS_FX_TempoGetSource(curStream), playlist.curSubindex, BASS_POS_SUBSONG);
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
for (int i=0, n=cmd.GetParamCount(); i<n; i++) openFileOrURL(U(cmd.GetParam(i)));
return true;
}


bool App::initDirs () {
SetAppName(APP_NAME);
SetClassName(APP_NAME);
SetVendorName(APP_VENDOR);

auto& stdPaths = wxStandardPaths::Get();
appDir = wxFileName(stdPaths.GetExecutablePath()).GetPath();
userDir = stdPaths.GetUserDataDir();
userLocalDir = stdPaths.GetUserLocalDataDir();

println("userDir={}", userDir);
println("userLocalDir={}", userLocalDir);
println("appDir={}", appDir);

auto userDirFn = wxFileName::DirName(userDir);
auto userLocalDirFn = wxFileName::DirName(userLocalDir);

pathList.Add(userDir);
pathList.Add(userLocalDir);
pathList.Add(appDir);

return 
userDirFn .Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)
&& userLocalDirFn .Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)
&& userDirFn.IsDirReadable()
&& userLocalDirFn.IsDirReadable();
}

bool App::initConfig () {
config = toml::table();
wxString configIniPath = pathList.FindAbsoluteValidPath(CONFIG_FILENAME);
if (configIniPath.empty()) println("No {} found", CONFIG_FILENAME);
else {
println("Config file found in {}", configIniPath);
wxFileInputStream fIn(configIniPath);
wxStdInputStream in(fIn);
config = toml::parse(in, U(configIniPath));
}

// Reading config from map
for (int i=0; i<7; i++) {
eqFreqs[i] = toml::find_or(config, "equalizer", "band"+to_string(i+1), "frequency", eqFreqs[i]);
eqBandwidths[i] = toml::find_or(config, "equalizer", "band"+to_string(i+1), "bandwidth", eqBandwidths[i]);
}
// Other configs from map

wxString eflPath = pathList.FindAbsoluteValidPath("effects.ini");
if (!eflPath.empty()) { 
wxFileInputStream fIn(eflPath);
wxStdInputStream in(fIn);
effects = EffectParams::read(in); 
}
return true;
}

wxString App::findWritablePath (const wxString& wantedPath) {
auto lastSlash = wantedPath.find_last_of("/\\");
wxString path, file;
if (lastSlash==string::npos) { path = ""; file=wantedPath; }
else { path=wantedPath.substr(0, lastSlash); file=wantedPath.substr(lastSlash+1); }
for (int i=0, n=pathList.GetCount(); i<n; i++) {
auto wxfn = wxFileName(pathList.Item(i), wxEmptyString);
if (wxfn.IsFileWritable() || wxfn.IsDirWritable()) {
wxString dirpath = wxfn.GetFullPath();
if (path.size()) {
if (!ends_with(dirpath, "/") && !ends_with(dirpath, "\\")) dirpath += "/";
dirpath += path;
}
if (wxFileName(dirpath, "").Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
wxfn = wxFileName(dirpath, file);
if (wxfn.IsFileWritable() || wxfn.IsDirWritable()) {
return wxfn.GetFullPath();
}}}}
return wxEmptyString;
}

bool App::saveConfig () {
wxString filename = findWritablePath(CONFIG_FILENAME);
if (filename.empty()) {
println("No valid writable path found to save configuration {}", CONFIG_FILENAME);
return false;
}
println("Saving configuration to {}", filename);
wxFileOutputStream fOut(filename);
if (!fOut.IsOk()) return false;
wxStdOutputStream out(fOut);
if (!out) return false;
out << std::setw(0) << config << std::endl;
return true;
}

bool App::saveMIDIConfig () {
wxString filename = findWritablePath(MIDI_CONFIG_FILENAME);
if (filename.empty()) {
println("No valid writable path found to save MIDI configuration {}", MIDI_CONFIG_FILENAME);
return false;
}
println("Saving MIDI configuration to {}", filename);
return saveMIDIConfig(filename, midiConfig);
}

bool App::initSpeech () {
int engine = toml::find_or(config, "speech", "engine", -1);
speechSetValue(SP_ENABLE_NATIVE_SPEECH, engine>=0);
#define P(K,C) { int x = toml::find_or(config, "speech", #K, -1); if (x>=0) speechSetValue(C, x); }
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
println("UniversalSpeech initialized: engineID={}, engine={}, subengine={}, language={}, voice={}, volume={}, pitch={}, rate={}, inflexion={}",
engine, engineNames[engine+1], speechGetValue(SP_SUBENGINE), speechGetValue(SP_LANGUAGE), speechGetValue(SP_VOICE), speechGetValue(SP_VOLUME), speechGetValue(SP_PITCH), speechGetValue(SP_RATE), speechGetValue(SP_INFLEXION));
return true;
}

bool App::initAudio () {
BASS_SetConfig(BASS_CONFIG_UNICODE, true);
BASS_SetConfig(BASS_CONFIG_DEV_DEFAULT, true);
BASS_SetConfig(BASS_CONFIG_FLOATDSP, true);
BASS_SetConfig(BASS_CONFIG_NET_PLAYLIST, 1);
BASS_SetConfig(BASS_CONFIG_BUFFER, 150);
BASS_SetConfig(BASS_CONFIG_MIXER_BUFFER, 1);

streamVol = toml::find_or(config, "stream", "volume", (int)(100.0f * streamVol)) /100.0f;
previewVol = toml::find_or(config, "preview", "volume", (int)(100.0f * previewVol)) / 100.0f;
previewLoop = toml::find_or(config, "preview", "loop", previewLoop);
micFbVol1 = toml::find_or(config, "mic1", "feedback", "volume", (int)(100.0f * micFbVol1)) / 100.0f;
micFbVol2 = toml::find_or(config, "mic2", "feedback", "volume", (int)(100.0f * micFbVol2)) / 100.0f;
streamVolInMixer = toml::find_or(config, "mixer", "stream", "volume", (int)(100.0f * streamVolInMixer)) / 100.0f;
micVol1 = toml::find_or(config, "mixer", "mic1", "volume", (int)(100.0f * micVol1)) / 100.0f;
micVol2 = toml::find_or(config, "mixer", "mic2", "volume", (int)(100.0f * micVol2)) / 100.0f;
loop = toml::find_or(config, "stream", "loop", loop);
random = toml::find_or(config, "playlist", "random", random);

println("Initializing BASS...");
if (!BASS_SimpleInit(-1)) return false;

loadedPlugins.clear();
wxDir dir(appDir);
wxString dllFile;
if (dir.GetFirst(&dllFile, U("bass?*.dll"))) do {
if (dllFile=="bass.dll" || dllFile=="bass_fx.dll" || dllFile=="bassmix.dll" || dllFile=="bass_vst.dll" || dllFile=="bass_ssl.dll" || starts_with(dllFile, "bassenc")) continue;
std::string dllKey = U(dllFile) .substr(0, dllFile.size() -4);
bool enabled = toml::find_or(config, "plugin", dllKey, "enabled", true);
int priority = toml::find_or(config, "plugin", dllKey, "priority", 1<<30);
loadedPlugins.emplace_back(0, dllFile, priority, enabled);
} while(dir.GetNext(&dllFile));

bool useMidiVsti = toml::find_or(config, "midi", "vsti", "enabled", false) && !toml::find_or(config, "midi", "vsti", "path", "").empty();
auto pMidi = std::find_if(loadedPlugins.begin(), loadedPlugins.end(), [&](auto&p){ return p.name=="bassmidi.dll"; });
auto pVsti = std::find_if(loadedPlugins.begin(), loadedPlugins.end(), [&](auto&p){ return p.name=="bassvstimidi.dll"; });
if (pMidi!=loadedPlugins.end()) pMidi->enabled = !useMidiVsti;
if (pVsti!=loadedPlugins.end()) pVsti->enabled = useMidiVsti;

std::stable_sort(loadedPlugins.begin(), loadedPlugins.end(), [&](auto& p1, auto& p2){ return p1.priority<p2.priority; });
for (int i=0, n=loadedPlugins.size(); i<n; i++) {
auto& plugin = loadedPlugins[i];
plugin.plugin = BASS_PluginLoad(U(plugin.name).c_str(), 0);
println("Plugin {}, loaded={}, enabled={}, priority={}, error={}", plugin.name, !!plugin.plugin, plugin.enabled, plugin.priority, BASS_ErrorGetCode());
if (!plugin.plugin) continue;
BASS_PluginEnable(plugin.plugin, plugin.enabled);
plugin.priority = i;
}

auto deviceList = BASS_GetDeviceList(true);
initAudioDevice(streamDevice, "stream.device", toml::find_or(config, "stream", "device", "default"), deviceList, BASS_SimpleInit, BASS_GetDevice);
initAudioDevice(previewDevice, "preview.device", toml::find_or(config, "preview", "device", "default"), deviceList, BASS_SimpleInit, BASS_GetDevice);
initAudioDevice(micFbDevice1, "mic1.feedback.device", toml::find_or(config, "mic1", "feedback", "device", "default"), deviceList, BASS_SimpleInit, BASS_GetDevice);
initAudioDevice(micFbDevice2, "mic2.feedback.device", toml::find_or(config, "mic2", "feedback", "device", "default"), deviceList, BASS_SimpleInit, BASS_GetDevice);
deviceList = BASS_RecordGetDeviceList(true);
initAudioDevice(micDevice1, "mic1.device", toml::find_or(config, "mic1", "device", "default"), deviceList, BASS_RecordSimpleInit, BASS_RecordGetDevice);
initAudioDevice(micDevice2, "mic2.device", toml::find_or(config, "mic2", "device", "default"), deviceList, BASS_RecordSimpleInit, BASS_RecordGetDevice);

wxString midiConfigPath = pathList.FindAbsoluteValidPath(MIDI_CONFIG_FILENAME);
if (midiConfigPath.empty()) loadDefaultMIDIConfig(midiConfig);
else loadMIDIConfig(midiConfigPath, midiConfig);
BASS_SetConfigPtr(BASS_CONFIG_MIDI_DEFFONT, (const char*)nullptr);
BASS_SetConfig(BASS_CONFIG_MIDI_AUTOFONT, 2);
BASS_SetConfig(BASS_CONFIG_MIDI_VOICES, toml::find_or(config, "midi", "maxVoices", 256)); 
applyMIDIConfig(midiConfig);

BASS_SetConfigPtr(0x10800, toml::find_or(config, "midi", "vsti", "path", "").c_str());

println("BASS audio initialized and configured successfully");
return true;
}

bool App::initAudioDevice (int& device, const string& configName, const string& sConf, const vector<pair<int,string>>& deviceList, function<bool(int)> init, function<int()> getDefault) {
size_t iFound = find_if(deviceList.begin(), deviceList.end(), [&](auto& p){ return iequals(p.second, sConf); }) -deviceList.begin();
if (iFound<deviceList.size() && init(deviceList[iFound].first)) device = deviceList[iFound].first;
if (device<0) device = getDefault();
if (device>=0 && device<(int)deviceList.size()) println("{} set to {} ({})", configName, deviceList[iFound].second, device);
else println("{} set to default/undefined ({})", configName, device);
return true;
}

bool App::loadMIDIConfig (const wxString& fn, std::vector<BassFontConfig>& midiConfig) {
wxFileInputStream fIn(fn);
if (!fIn.IsOk()) return false;
wxStdInputStream in(fIn);
if (!in) return false;
auto conf = toml::parse(in, U(fn));
for (int sfi=0, sfn=conf.count("soundfont"); sfi<sfn; sfi++) {
const auto& sf = toml::find(conf, "soundfont", sfi);
std::string file = toml::find_or(sf, "path", "");
if (file.empty()) break;
DWORD flags =
(toml::find_or(sf, "xgdrums", false)? BASS_MIDI_FONT_XGDRUMS : 0)
| (toml::find_or(sf, "linattmod", false)? BASS_MIDI_FONT_LINATTMOD : 0)
| (toml::find_or(sf, "lindecvol", false)? BASS_MIDI_FONT_LINDECVOL  : 0)
| (toml::find_or(sf, "minfx", false)? BASS_MIDI_FONT_MINFX : 0)
| (toml::find_or(sf, "nofx", false)? BASS_MIDI_FONT_NOFX : 0)
| (toml::find_or(sf, "nolimits", false)? BASS_MIDI_FONT_NOLIMITS : 0)
| (toml::find_or(sf, "norampin", false)? BASS_MIDI_FONT_NORAMPIN : 0)
| (toml::find_or(sf, "mmap", false)? BASS_MIDI_FONT_MMAP : 0);
auto font = BASS_MIDI_FontInit(file.c_str(), flags);
if (!font) continue;

int spreset = toml::find_or(sf, "spreset", -1);
int sbank = toml::find_or(sf, "sbank", -1);
int dpreset = toml::find_or(sf, "dpreset", -1);
int dbank = toml::find_or(sf, "dbank", 0);
int dbanklsb = toml::find_or(sf, "dbanklsb", 0);
double volume = toml::find_or(sf, "volume", 100) / 100.0;
if (volume!=1) BASS_MIDI_FontSetVolume(font, volume);
midiConfig.emplace_back(file, font, spreset, sbank, dpreset, dbank, dbanklsb, volume, flags);
}
return true;
}

bool App::loadDefaultMIDIConfig (std::vector<BassFontConfig>& midiConfig) {
midiConfig.clear();
wxDir dir(appDir);
wxString sf2file;
if (dir.GetFirst(&sf2file, U("*.sf2"))) do {
auto font = BASS_MIDI_FontInit(sf2file.wc_str(), BASS_UNICODE | 0);
if (!font) continue;
midiConfig.emplace_back( U(sf2file), font, -1, -1, -1, 0, 0, 1, 0);
} while(dir.GetNext(&sf2file));
return true;
}

bool App::saveMIDIConfig (const wxString& fn, const std::vector<BassFontConfig>& midiConfig) {
toml::value sfshdr;
auto& sfs = (sfshdr["soundfont"] = toml::array());
for (auto& c: midiConfig) {
toml::value sf;
sf["path"] = c.file;
if (c.spreset!=-1) sf["spreset"] = c.spreset;
if (c.sbank!=-1) sf["sbank"]= c.sbank;
if (c.dpreset!=-1) sf["dpreset"] = c.dpreset;
if (c.dbank!=0) sf["dbank"] = c.dbank;
if (c.dbanklsb!=0) sf["dbanklsb"] = c.dbanklsb;
if (c.volume!=1) sf["volume"] = (int)(100.0f * c.volume);
if (c.flags & BASS_MIDI_FONT_XGDRUMS) sf["xgdrums"] = true;
if (c.flags & BASS_MIDI_FONT_LINATTMOD) sf["linattmod"] = true;
if (c.flags & BASS_MIDI_FONT_LINDECVOL) sf["lindecvol"] = true;
if (c.flags & BASS_MIDI_FONT_MINFX) sf["minfx"] = true;
if (c.flags & BASS_MIDI_FONT_NOFX) sf["nofx"] = true;
if (c.flags & BASS_MIDI_FONT_NOLIMITS) sf["nolimits"] = true;
if (c.flags & BASS_MIDI_FONT_NORAMPIN) sf["norampin"] = true;
if (c.flags & BASS_MIDI_FONT_MMAP) sf["mmap"] = true;
sfs.emplace_back(sf);
}
wxFileOutputStream fOut(fn);
if (!fOut.IsOk()) return false;
wxStdOutputStream out(fOut);
if (!out) return false;
out << std::setw(0) << sfshdr << std::endl;
return true;
}

bool App::applyMIDIConfig (const std::vector<BassFontConfig>& config) {
BASS_MIDI_FONTEX fonts[config.size()];
std::copy(config.begin(), config.end(), fonts);
return BASS_MIDI_StreamSetFonts(0, fonts, config.size() | BASS_MIDI_FONT_EX);
}

bool App::initLocale () {
locale = toml::find_or(config, "app", "locale", "default");
wxlocale = new wxLocale();
if (locale=="default") {
println("No locale set in the configuration, retrieving system default");
wxlocale->Init();
}
else {
auto info = wxLocale::FindLanguageInfo(U(locale));
if (info) wxlocale->Init(info->Language);
else println("Couldn't find locale information for {}", locale);
}
this->locale = U(wxlocale->GetCanonicalName());
auto& translations = *wxTranslations::Get();
translations.SetLoader(new CustomFileTranslationLoader());
translations.AddStdCatalog();
println("Locale configured to {}", locale);
return true;
}

bool App::initTranslations () {
std::string conflocale = toml::find_or(config, "app", "locale", locale);
vector<string> locales = {
conflocale,
conflocale.substr(0, 5),
conflocale.substr(0, 2),
locale,
locale.substr(0, 5),
locale.substr(0, 2),
"en"
};
for (string& l: locales) {
wxString transPath = pathList.FindAbsoluteValidPath(format("lang/sample_{}.properties", l));
if (!transPath.empty()) {
println("Translations found for locale {} in {}", l, transPath);
lang.setFlags(PM_BKESC);
lang.load(U(transPath));
break;
}}
return true;
}

void App::changeLocale (const string& s) {
auto info = wxLocale::FindLanguageInfo(U(s));
if (!info) {
println("Couldn't change locale to {}, no locale information found", s);
return;
}
println("Changing language to {}...", info->CanonicalName);
locale = U(info->CanonicalName);
config["app"]["locale"] = locale;
initLocale();
initTranslations();
//todo: other consequences of changing locale
}

bool App::OnExceptionInMainLoop () {
try { throw; } catch (std::exception& e) {
std::string tp = boost::core::demangle(typeid(e).name()), what = e.what(), msg = tp + ": " + what;
println("FATAL ERROR! {}", msg);
wxMessageBox(msg, "FATAL ERROR!", wxICON_ERROR | wxOK);
}
return true;
}

void App::OnUnhandledException () {
OnExceptionInMainLoop();
}

void App::openFileOrURL (const std::string& s) {
auto& entry = playlist.add(s);
if (playlist.size()==1) playNext(0);
}

DWORD App::loadFileOrURL (const std::string& s, bool loop, bool decode) {
if (string::npos!=s.find(':', 2)) return loadURL(s, loop, decode);
else return loadFile(s, loop, decode);
}

DWORD loadUsingLoaders (const string& file, DWORD flags, int ldrFlags) {
if (!Loader::loaders.size()) ldrAddAll();
for (auto& loader: Loader::loaders) {
if (!(loader->flags&ldrFlags)) continue;
DWORD stream = loader->load(file, flags);
if (stream) return stream;
}
return 0;
}

DWORD App::loadFile (const std::string& file, bool loop, bool decode) {
DWORD flags = (loop? BASS_SAMPLE_LOOP : 0) | (decode? BASS_STREAM_DECODE : BASS_STREAM_AUTOFREE);
DWORD stream = BASS_StreamCreateFile(false, file.c_str(), 0, 0, flags | BASS_STREAM_PRESCAN);
if (!stream) stream = loadUsingLoaders(file, flags, LF_FILE);
return stream;
}

DWORD App::loadURL (const std::string& url, bool loop, bool decode) {
DWORD flags = (loop? BASS_SAMPLE_LOOP : 0) | (decode? BASS_STREAM_DECODE : BASS_STREAM_AUTOFREE);
DWORD stream = BASS_StreamCreateURL(url.c_str(), 0, flags, nullptr, nullptr);
if (!stream) stream = loadUsingLoaders(url, flags, LF_URL);
return stream;
}

static void CALLBACK streamSyncEnd (HSYNC sync, DWORD chan, DWORD data, void* udata) {
App& app = *reinterpret_cast<App*>(udata);
app.OnStreamEnd();
}

static void CALLBACK customLoop (HSYNC sync, DWORD chan, DWORD data, void* udata) {
if (wxGetApp().loop) BASS_ChannelSetPosition(chan, (DWORD)udata, BASS_POS_BYTE);
}

static void CALLBACK streamMidiMark (HSYNC sync, DWORD chan, DWORD data, void* udata) {
BASS_MIDI_MARK mark;
if (!BASS_MIDI_StreamGetMark(chan, (DWORD)udata, data, &mark)) return;
if (!mark.text) return;
if (!*mark.text || *mark.text=='@') return;
wxString text = UI(mark.text);
if (text.empty()) return;
bool append = true;
if (text[0]=='/' || text[0]=='\\') {
text.erase(text.begin());
append=false;
}
RunEDT([=](){
App& app = wxGetApp();
if (app.win) app.win->OnSubtitle(text, append);
});
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
if (bpm>160) bpm/=2;
app.curStreamBPM = bpm;
}

void App::shufflePlaylist () {
auto& pl = playlist;
pl.shuffle(pl.curIndex+1);
if (win) win->OnTrackChanged();
}

void App::playNext (int step) { 
DWORD srcStream = curStream && step!=0? BASS_FX_TempoGetSource(curStream) :0;
int nSubsongs = srcStream? BASS_ChannelGetLength(srcStream, BASS_POS_SUBSONG) :0;
int curSubsong = nSubsongs>0? BASS_ChannelGetPosition(srcStream, BASS_POS_SUBSONG) :-1;
if (curSubsong>=0 && curSubsong+step>=0 && curSubsong+step<nSubsongs) {
curSubsong += step;
playlist.curSubindex = curSubsong;
BASS_ChannelSetPosition(srcStream, curSubsong, BASS_POS_SUBSONG);
if (win) win->OnTrackChanged();
}
else if (playlist.size()>0) {
if (step!=0) playlist.curSubindex = 0;
playAt((playlist.curIndex + step + playlist.size())%playlist.size()); 
}
}

void App::playAt (int index) {
if (curStream) BASS_ChannelStop(curStream);
playlist.curIndex=index;
auto& item = playlist[index];
DWORD loopFlag = loop? BASS_SAMPLE_LOOP : 0;
BASS_SetDevice(streamDevice);
DWORD stream = loadFileOrURL(item.file, loop, true);
if (!stream) {
string file = item.file;
playlist.erase();
bool  loaded = playlist.load(file);
if (loaded && random) shufflePlaylist();
playNext(0);
return;
}

BASS_CHANNELINFO ci;
BASS_ChannelGetInfo(stream, &ci);
curStreamVoicesMax = 0;
curStreamRowMax = 0;
curStreamBPM = 0;
curStreamType = ci.ctype;
seekable = !(ci.flags & ( BASS_STREAM_BLOCK | BASS_STREAM_RESTRATE));
if (playlist.curSubindex>0) BASS_ChannelSetPosition(stream, playlist.curSubindex, BASS_POS_SUBSONG);

if (ci.ctype==BASS_CTYPE_STREAM_MIDI) {
BASS_ChannelSetSync(stream, BASS_SYNC_MIDI_MARK, BASS_MIDI_MARK_TEXT, streamMidiMark, (void*)BASS_MIDI_MARK_TEXT);
BASS_ChannelSetSync(stream, BASS_SYNC_MIDI_MARK, BASS_MIDI_MARK_LYRIC, streamMidiMark, (void*)BASS_MIDI_MARK_LYRIC);
if (win && win->midiWindow) win->midiWindow->OnLoadMIDI(stream);
}
PropertyMap tags(PM_LCKEYS);
item.loadMetaData(stream, tags);

DWORD loopStart = tags.get("loopstart", 0), loopEnd = tags.get("loopend", 0);
if (loopStart && loopEnd) {
loopEnd = std::min<DWORD>(loopEnd, BASS_ChannelGetLength(stream, BASS_POS_BYTE) );
BASS_ChannelSetSync(stream, BASS_SYNC_POS | BASS_SYNC_MIXTIME, loopEnd, customLoop, (void*)loopStart);
}

if (item.replayGain) {
double linear = pow(10, item.replayGain/20.0);
bool re = BASS_ChannelSetAttribute(stream, BASS_ATTRIB_VOLDSP, linear);
}

curStream = BASS_FX_TempoCreate(stream, loopFlag | BASS_FX_FREESOURCE | BASS_STREAM_AUTOFREE);
BASS_FX_BPM_CallbackSet(curStream, &BPMUpdateProc, 5, 0, 0, this);
curStreamEqFX = BASS_ChannelSetFX(curStream, BASS_FX_BFX_PEAKEQ, 0);
BASS_ChannelSetAttribute(curStream, BASS_ATTRIB_TEMPO_PITCH, streamPitch);
BASS_ChannelSetAttribute(curStream, BASS_ATTRIB_TEMPO, (streamRateRatio * 100) -100);
for (int i=0; i<7; i++) { BASS_BFX_PEAKEQ p = { i, eqBandwidths[i], 0, eqFreqs[i], 0, -1 }; BASS_FXSetParameters(curStreamEqFX, &p); }
for (auto& effect: effects) { effect.handle=0; applyEffect(effect); }
BASS_ChannelSetSync(curStream, BASS_SYNC_END, 0, streamSyncEnd, this);
BASS_ChannelSetAttribute(curStream, BASS_ATTRIB_VOL, streamVol);
BASS_ChannelPlay(curStream, false);

if (mixHandle) plugCurStreamToMix(*this);

if (win) win->OnTrackChanged();
}

void App::saveEncode (PlaylistItem& item, const std::string& file, Encoder& encoder) {
auto& app = *this;
worker->submit([=, &encoder, &item, &app](){
println("Saving {}...", file);
DWORD source = loadFileOrURL(item.file, false, true);
if (!source) return;
DWORD encoderHandle = encoder.startEncoderFile(item, source, file);
auto lastSlash = file.find_last_of("/\\");
int read=0, length = BASS_ChannelGetLength(source, BASS_POS_BYTE);
string sFile = file.substr(lastSlash==string::npos? 0 : lastSlash+1);
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

void App::stopMic (int n, bool updateMenu, bool updateLevelWindow) {
DWORD* mhs[] = { &micHandle1, &micHandle2 };
DWORD* mfhs[] = { &micFbHandle1, &micFbHandle2 };
DWORD &mic = *mhs[n], &micFb = *mfhs[n ];
if (micFb) BASS_ChannelStop(micFb);
if (mic) BASS_ChannelStop(mic);
if (mixHandle && mic) BASS_Mixer_ChannelRemove(mic);
mic = micFb = 0;
if (updateMenu && win) win->GetMenuBar() ->Check(IDM_MIC1+n, false);
if (updateLevelWindow && win && win->levelsWindow) win->levelsWindow->tbMic1->SetValue(false);
}

bool App::startMic (int n, bool updateMenu, bool updateLevelWindow) {
DWORD* mhs[] = { &micHandle1, &micHandle2 };
DWORD* mfhs[] = { &micFbHandle1, &micFbHandle2 };
int* mdevs[] = { &micDevice1, &micDevice2 };
int* mfdevs[] = { &micFbDevice1, &micFbDevice2 };
float* mvols[] = { &micVol1, &micVol2 };
float* mfvols[] = { &micFbVol1, &micFbVol2 };
DWORD &mic = *mhs[n], &micFb = *mfhs[n];
int &device = *mdevs[n], &fbDevice = *mfdevs[n];
float &vol = *mvols[n], &fbVol = *mfvols[n];
if (mic || micFb) stopMic(n, updateMenu, updateLevelWindow);
if (!BASS_RecordSimpleInit(device) || !BASS_SimpleInit(fbDevice)) return false;
BASS_RecordSetDevice(device);
mic = BASS_RecordStart(0, 0, BASS_SAMPLE_FLOAT, nullptr, nullptr);
if (!mic) return false;
BASS_SetDevice(fbDevice);
micFb = BASS_StreamCreateCopy(mic, false);
auto re2 = BASS_ChannelSetAttribute(micFb, BASS_ATTRIB_VOL, fbVol);
auto re3 = BASS_ChannelSetAttribute(mic, BASS_ATTRIB_VOL, vol);
auto re4 = BASS_ChannelPlay(micFb, false);
startMix();
auto re = BASS_Mixer_StreamAddChannel(mixHandle, mic, BASS_MIXER_DOWNMIX | BASS_STREAM_AUTOFREE);
if (updateMenu && win) win->GetMenuBar() ->Check(IDM_MIC1+n, true);
if (updateLevelWindow && win && win->levelsWindow) win->levelsWindow->tbMic1->SetValue(true);
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
if (mic || micFb) startMic(n, false, false);
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

void App::changeLoopPreview (bool b) {
previewLoop=b;
if (curPreviewStream) {
DWORD flags = previewLoop? BASS_SAMPLE_LOOP : 0;
BASS_ChannelFlags(curPreviewStream, flags, BASS_SAMPLE_LOOP);
}
}

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
else if (!seekable) playNext(0);
}

void App::OnGlobalCharHook (wxKeyEvent& e) {
auto k = e.GetKeyCode(), mods = e.GetModifiers();
e.Skip();
if (curPreviewStream && win && win->slPreviewVolume && win->slPreviewPosition) {
if (mods==wxMOD_NONE && k==WXK_F8) pausePreview();
else if (mods==wxMOD_NONE && k==WXK_F6) { seekPreview(-5, false, true); e.Skip(false); }
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
curPreviewStream = loadFileOrURL(curPreviewFile, previewLoop);
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
std::string file = toml::find_or(config, "playlist", "file", "");
if (file.empty()) file = U(findWritablePath(APP_NAME "_auto_save.pls"));
if (playlist.save(file)) config["playlist"]["file"] = playlist.file;
else config["playlist"]["file"] = "";

config["stream"]["volume"] = (int)(100.0f * streamVol);
config["preview"]["volume"] = (int)(100.0f * previewVol);
config["mic1"]["feedback"]["volume"] = (int)(100.0f * micFbVol1);
config["mic2"]["feedback"]["volume"] = (int)(100.0f * micFbVol2);
config["mixer"]["stream"]["volume"] = (int)(100.0f * streamVolInMixer);
config["mixer"]["mic1"]["volume"] = (int)(100.0f * micVol1);
config["mixer"]["mic2"]["volume"] = (int)(100.0f * micVol2);

auto list = BASS_GetDeviceList(true);
auto find = [&](int x){ for (auto& p: list) if (p.first==x) return p.second; return string(); };
if (streamDevice>=0) config["stream"]["device"] = find(streamDevice);
if (previewDevice>=0) config["preview"]["device"] = find(previewDevice);
if (micFbDevice1>=0) config["mic1"]["feedback"]["device"] = find(micFbDevice1);
if (micFbDevice2>=0) config["mic2"]["feedback"]["device"] = find(micFbDevice2);
list = BASS_RecordGetDeviceList(true);
if (micDevice1>=0)  config["mic1"]["device"] = find(micDevice1);
if (micDevice2>=0)  config["mic2"]["device"] = find(micDevice2);

config["stream"]["loop"] = loop;
config["preview"]["loop"] = previewLoop;
config["playlist"]["random"] = random;
//###!
for (int i=0, n=loadedPlugins.size(); i<n; i++) {
auto& plugin = loadedPlugins[i];
std::string key = U(plugin.name.substr(0, plugin.name.size() -4));
config["plugin"][key]["enabled"] = plugin.enabled;
config["plugin"][key]["priority"] = plugin.priority;
}
saveConfig();
delete ipcServer;
}


