#ifndef _____PLWINDOW0_____
#define _____PLWINDOW0_____
#include "WXWidgets.hpp"

struct PlaylistWindow: wxDialog {
struct App& app;
struct wxTextCtrl* tfFilter;
struct wxListView* lcList;
struct wxStatusBar* status;

std::string input = "";
long lastInputTime=0;

PlaylistWindow (App& app);
void onItemClick ();
void OnItemSelect ();
void OnListKeyDown (wxKeyEvent& e);
void OnListKeyChar (wxKeyEvent& e);
void OnContextMenu (wxContextMenuEvent& e);
void OnCloseRequest ();
void OnFileProperties ();
void OnIndexAll();
void OnShuffle ();
void OnMoveUp ();
void OnMoveDown ();
void OnFilterTextChange (wxCommandEvent& e);
void OnFilterTextEnter (wxCommandEvent& e);
void updateList ();
void startPreview ();
};

#endif
