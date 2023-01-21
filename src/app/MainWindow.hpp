#ifndef _____MAIN_WINDOW_0
#define _____MAIN_WINDOW_0
#include "../common/WXWidgets.hpp"

struct MainWindow: wxFrame {
struct App& app;

struct wxStatusBar* status;
struct wxSlider *slPosition, *slVolume, *slPitch, *slRate, *slEqualizer[7], *slPreviewVolume=nullptr, *slPreviewPosition=nullptr;
struct wxButton* btnPlay, *btnNext, *btnPrev, *btnOptions;

struct wxTimer *refreshTimer = nullptr, *otherTimer = nullptr;
struct PlaylistWindow* playlistWindow = nullptr;
struct LevelsWindow* levelsWindow = nullptr;
struct wxProgressDialog* progressDialog = nullptr;
bool progressCancelled=false;
std::function<void()> timerFunc = nullptr;
int statusDisplayModes[5] = {0};

MainWindow (App& app);
void showAboutBox (wxWindow* parent);

void openProgress (const std::string& title, const std::string& message, int maxValue);
void closeProgress ();
void updateProgress (int value);
void setProgressText (const std::string& msg);
void setProgressTitle (const std::string& msg);
void setProgressMax (int max);
bool isProgressCancelled ();
void OnProgress (struct wxThreadEvent& e);
void setTimeout (int ms, const std::function<void()>& func);

int popupMenu (const std::vector<std::string>& items, int selection=-1);

void OnClose (wxCloseEvent& e);

void OnTrackChanged ();
void OnTrackUpdate (struct wxTimerEvent& e);
void restart ();
bool seekPosition (double secs, bool update=true);
void OnSeekPosition (wxScrollEvent& e);
void OnJumpDlg (wxCommandEvent& e);
void changeVol (float vol, bool update=true, bool updateInLevels=true);
void OnVolChange (wxScrollEvent& e);
void changeRate (double ratio, bool update=true);
void OnRateChange (wxScrollEvent& e);
void changePitch (int pitch, bool update=true);
void OnPitchChange (wxScrollEvent& e);
void changeEqualizer (int index, float gain, bool update=true);
void OnEqualizerChange (wxScrollEvent& e, int index);
void OnLoopChange ();
void OnLoopChange (wxCommandEvent& e) { OnLoopChange(); }
void OnMicChange (int n);
void OnMic1Change (wxCommandEvent& e) { OnMicChange(0); }
void OnMic2Change (wxCommandEvent& e) { OnMicChange(1); }
void OnToggleEffect (int id);
void OnToggleEffect (wxCommandEvent& e) { OnToggleEffect(e.GetId()); }
void OnStatusBarContextMenu (int fieldIndex);
void OnStatusBarContextMenu (wxContextMenuEvent& e);
void OnStatusBarClick (int fieldIndex);
void OnStatusBarClick (wxMouseEvent& e);

void OnPlayPause ();
void OnPlayPause (wxCommandEvent& e) { OnPlayPause(); }
void OnPlayPauseHK (wxKeyEvent& e) { OnPlayPause(); }
void OnNextTrack ();
void OnNextTrack (wxCommandEvent& e) { OnNextTrack(); }
void OnPrevNextTrackHK (wxKeyEvent& e, bool next);
void OnNextTrackHK (wxKeyEvent& e) { OnPrevNextTrackHK(e, true); }
void OnPrevTrack ();
void OnPrevTrack (wxCommandEvent& e) { OnPrevTrack(); }
void OnPrevTrackHK (wxKeyEvent& e) { OnPrevNextTrackHK(e, false); }
void OnCharHook (wxKeyEvent& e);

void OnShowPlaylist (wxCommandEvent& e);
void OnShowLevels (wxCommandEvent& e);
void OnShowItemInfo (wxCommandEvent& e);
void OnPreferencesDlg (wxCommandEvent& e);
void OnSaveDlg (wxCommandEvent& e);
void OnSavePlaylistDlg (wxCommandEvent& e);

void OnOpenFile (wxCommandEvent& e);
void OnAppendFile (wxCommandEvent& e);
void OnOpenFileDlg (bool append);
void OnOpenDir (wxCommandEvent& e);
void OnAppendDir (wxCommandEvent& e);
void OnOpenDirDlg (bool append);
void OnOpenURL (wxCommandEvent& e);
void OnAppendURL (wxCommandEvent& e);
void OnOpenURLDlg (bool append);
void OnCastStreamDlg (wxCommandEvent& e);
};

#endif
