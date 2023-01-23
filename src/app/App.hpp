#ifndef _____APP_HPP_0
#define _____APP_HPP_0
#include "../common/constants.h"
#include "../common/PropertyMap.hpp"
#include "../playlist/Playlist.hpp"
#include "../effect/Effect.hpp"
#include "../common/WXWidgets.hpp"
#include "../common/BassPlugin.hpp"
#include <wx/thread.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include<string>
#include<vector>
#include<unordered_map>
#include<fstream>
#include<functional>

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

bool loop = false, random=false, introMode=false, seekable=true, previewLoop=false;
float streamVol = 0.2f, previewVol = 0.2f, micFbVol1 = 0.2f, micFbVol2 = 0.2f, micVol1 = 1.0f, micVol2 = 1.0f, streamVolInMixer = 1.0f;
DWORD curStream = 0, curStreamEqFX = 0, curPreviewStream=0, mixHandle=0, encoderHandle=0, curStreamInMixer=0, curStreamType=0;
DWORD micHandle1 = 0, micHandle2 = 0, micFbHandle1 = 0, micFbHandle2 = 0;
int streamDevice = -1, previewDevice = -1, micFbDevice1 = -1, micFbDevice2 = -1, micDevice1 = -1, micDevice2 = -1;
int streamPitch = 0;
double streamRateRatio = 1;
Playlist playlist;

std::vector<BassPlugin> loadedPlugins;
std::vector<EffectParams> effects;

bool explicitEncoderLaunch = false;
int curStreamVoicesMax=0, castListeners=0, castListenersMax=0, castListenersTime=0, curStreamBPM=0, curStreamRowMax=0;
std::string curPreviewFile;

bool initDirs ();
bool initConfig ();
bool initLocale ();
bool initTranslations ();
bool initSpeech ();
bool initAudio ();
bool initAudioDevice (int& device, const std::string& configName, const std::vector<std::pair<int,std::string>>& deviceList, std::function<bool(int)> init, std::function<int()> getDefault);
bool initTags ();

bool saveConfig ();
wxString findWritablePath (const wxString& filename);
void changeLocale (const std::string& s);

DWORD loadFile (const std::string& file, bool loop=false, bool decode=false);
DWORD loadURL (const std::string& url, bool loop=false, bool decode=false);
DWORD loadFileOrURL (const std::string& s, bool loop=false, bool decode=false);

void playAt (int index);
void playNext (int step = 1) { if (playlist.size()>0) playAt((playlist.curIndex + step + playlist.size())%playlist.size()); }
void clearPlaylist () { playlist.clear(); }
void OnStreamEnd ();
void OnGlobalCharHook (struct wxKeyEvent& e);

void openFileOrURL (const std::string& s);

void saveEncode (struct PlaylistItem& item, const std::string& file, struct Encoder& enc);
void startMix ();
void stopMix ();
void tryStopMix ();
void startCaster (struct Caster& caster, struct Encoder& encoder, const std::string& server, const std::string& port, const std::string& user, const std::string& pass, const std::string& mount);
void stopCaster ();
bool startMic (int n, bool updateMenu, bool updateLevelWindow);
void stopMic (int n, bool updateMenu, bool updateLevelWindow);
void startStopMic (int n, bool b, bool um, bool ul) { if (b) startMic(n, um, ul); else stopMic(n, um, ul); }
bool changeStreamDevice (int device);
bool changePreviewDevice (int device);
bool changeMicFeedbackDevice (int device, int n);
bool changeMicDevice (int device, int n);
void changePreviewVol (float vol, bool updateLevel=true, bool updatePreview=true);
void changeMicFeedbackVol (float vol, int n, bool update=true);
void changeMicMixVol (float vol, int n, bool update=true);
void changeStreamMixVol (float vol, bool update=true);
void applyEffect (EffectParams& effect);
void playPreview (const std::string& file);
void pausePreview ();
void stopPreview ();
void seekPreview (int pos, bool abs, bool updatePreview=true);
void changeLoopPreview (bool b);

virtual bool OnInit () override;
virtual void OnInitCmdLine (wxCmdLineParser& cmd) override;
virtual bool OnCmdLineParsed (wxCmdLineParser& cmd) override;
void OnQuit ();
};

template <class F> inline void RunEDT (const F& f) {
wxGetApp().CallAfter(f);
}


#endif

