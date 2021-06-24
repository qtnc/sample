#ifndef _____APP_HPP_0
#define _____APP_HPP_0
#include "constants.h"
#include "PropertyMap.hpp"
#include "Playlist.hpp"
#include "Effect.hpp"
#include "WXWidgets.hpp"
#include <wx/thread.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include<string>
#include<vector>
#include<unordered_map>
#include<fstream>
#include<functional>
#include "cpprintf.hpp"

struct App;
wxDECLARE_APP(App);

struct App: wxApp {
struct MainWindow* win = nullptr;
struct WorkerThread* worker = nullptr;
wxLocale* wxlocale;
std::string locale;

PropertyMap config, lang;
wxPathList pathList;
wxString appDir, userDir, userLocalDir;
struct IPCServer* ipcServer;

bool loop = false, random=false, introMode=false;
float streamVol = 0.2f, previewVol = 0.2f, micFbVol1 = 0.2f, micFbVol2 = 0.2f, micVol1 = 1.0f, micVol2 = 1.0f, streamVolInMixer = 1.0f;
DWORD curStream = 0, curStreamEqFX = 0, curPreviewStream=0, mixHandle=0, encoderHandle=0, curStreamInMixer=0, curStreamType=0;
DWORD micHandle1 = 0, micHandle2 = 0, micFbHandle1 = 0, micFbHandle2 = 0;
int streamDevice = -1, previewDevice = -1, micFbDevice1 = -1, micFbDevice2 = -1, micDevice1 = -1, micDevice2 = -1;
int curStreamVoicesMax=0, castListeners=0, castListenersMax=0, castListenersTime=0;
bool explicitEncoderLaunch = false;
Playlist playlist;

std::vector<unsigned long> loadedPlugins;
std::vector<EffectParams> effects;

bool initDirs ();
bool initConfig ();
bool initLocale ();
bool initTranslations ();
bool initSpeech ();
bool initAudio ();
bool initAudioDevice (int& device, const std::string& configName, const std::vector<std::pair<int,std::string>>& deviceList, std::function<bool(int)> init, std::function<int()> getDefault);
bool initTags ();

bool saveConfig ();
std::string findWritablePath (const std::string& filename);
void changeLocale (const std::string& s);

DWORD loadFile (const std::string& file, bool loop=false, bool decode=false);
DWORD loadURL (const std::string& url, bool loop=false, bool decode=false);
DWORD loadFileOrURL (const std::string& s, bool loop=false, bool decode=false);

void playAt (int index);
void playNext (int step = 1) { if (playlist.size()>0) playAt((playlist.curIndex + step + playlist.size())%playlist.size()); }
void clearPlaylist () { playlist.clear(); }
void OnStreamEnd ();

void openFileOrURL (const std::string& s);

void saveEncode (struct PlaylistItem& item, const std::string& file, struct Encoder& enc);
void startMix ();
void stopMix ();
void tryStopMix ();
void startCaster (struct Caster& caster, struct Encoder& encoder, const std::string& server, const std::string& port, const std::string& user, const std::string& pass, const std::string& mount);
void stopCaster ();
bool startMic (int n);
void stopMic (int n);
void startStopMic (int n, bool b) { if (b) startMic(n); else stopMic(n); }
bool changeStreamDevice (int device);
bool changePreviewDevice (int device);
bool changeMicFeedbackDevice (int device, int n);
bool changeMicDevice (int device, int n);
void changePreviewVol (float vol, bool update=true);
void changeMicFeedbackVol (float vol, int n, bool update=true);
void changeMicMixVol (float vol, int n, bool update=true);
void changeStreamMixVol (float vol, bool update=true);
void applyEffect (EffectParams& effect);


virtual bool OnInit () override;
virtual void OnInitCmdLine (wxCmdLineParser& cmd) override;
virtual bool OnCmdLineParsed (wxCmdLineParser& cmd) override;
void OnQuit ();
};

template <class F> inline void RunEDT (const F& f) {
wxGetApp().CallAfter(f);
}


#endif

