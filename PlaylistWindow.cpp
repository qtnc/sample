#include<algorithm>
#include "PlaylistWindow.hpp"
#include "App.hpp"
#include "MainWindow.hpp"
#include "ItemInfoDlg.hpp"
#include "Playlist.hpp"
#include "cpprintf.hpp"
#include <wx/listctrl.h>
#include "bass.h"
#include<typeinfo>
using namespace std;

PlaylistWindow::PlaylistWindow (App& app):
wxDialog(app.win, -1, U(translate("PlaylistWin")) ),
app(app) 
{
auto lblFilter = new wxStaticText(this, wxID_ANY, U(translate("QuickFilter")), wxPoint(-2, -2), wxSize(1, 1) );
tfFilter = new wxTextCtrl(this, 500, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
auto lblPlaylist = new wxStaticText(this, wxID_ANY, U(translate("PlaylistLbl")), wxPoint(-2, -2), wxSize(1, 1) );
lcList = new wxListView(this, 501, wxDefaultPosition, wxDefaultSize, wxLC_REPORT);
status = new wxStatusBar(this, 505, 0, wxEmptyString);

auto sizer = new wxBoxSizer(wxVERTICAL);
sizer->Add(tfFilter, 0, wxEXPAND);
sizer->Add(lcList, 1, wxEXPAND);
sizer->Add(status, 0, wxEXPAND);
SetSizerAndFit(sizer);

tfFilter->Bind(wxEVT_TEXT, &PlaylistWindow::OnFilterTextChange, this);
tfFilter->Bind(wxEVT_TEXT_ENTER, &PlaylistWindow::OnFilterTextEnter, this);
lcList->Bind(wxEVT_LIST_ITEM_ACTIVATED, [&](auto& e){ onItemClick(); });
//lcList->Bind(wxEVT_CHAR_HOOK, &PlaylistWindow::OnListKeyDown, this);
lcList->Bind(wxEVT_CHAR, &PlaylistWindow::OnListKeyChar, this);
lcList->Bind(wxEVT_CONTEXT_MENU, &PlaylistWindow::OnContextMenu, this);
Bind(wxEVT_ACTIVATE, [&](auto& e){ updateList(); });
Bind(wxEVT_CLOSE_WINDOW, [&](auto& e){ Beep(1000,150); e.Skip(); });
lcList->SetFocus();
}

void PlaylistWindow::OnCloseRequest () {
this->Hide(); 
app.win->GetMenuBar()->Check(IDM_SHOWPLAYLIST, false);
app.win->btnPlay->SetFocus(); 
}

void PlaylistWindow::OnContextMenu (wxContextMenuEvent& e) {
vector<string> items = {
translate("Play"),
translate("FileProperties"),
translate("Close")
};
vector<function<void(PlaylistWindow&)>> actions = {
&PlaylistWindow::onItemClick,
&PlaylistWindow::OnFileProperties,
&PlaylistWindow::OnCloseRequest
};
int re = app.win->popupMenu(items);
if (re>=0 && re<actions.size()) actions[re](*this);
}

void PlaylistWindow::onItemClick () {
int selection = lcList->GetFirstSelected();
if (selection<0) return;
int index = lcList->GetItemData(selection);
app.playAt(index);
}

void PlaylistWindow::OnFileProperties () {
int selection = lcList->GetFirstSelected();
if (selection<0) return;
int index = lcList->GetItemData(selection);
auto& item = app.playlist[index];
DWORD stream = app.loadFileOrURL(item.file, false, true);
item.loadTagsFromBASS(stream);
ItemInfoDlg dlg(app, this, item, stream);
dlg.ShowModal();
updateList();
lcList->SetFocus();
BASS_StreamFree(stream);
BASS_MusicFree(stream);
}

void PlaylistWindow::OnListKeyDown (wxKeyEvent& e) {
int key = e.GetKeyCode(), mod = e.GetModifiers();
if (key==WXK_RETURN && mod==0) {
onItemClick();
return;
}
e.Skip();
}

void PlaylistWindow::OnListKeyChar (wxKeyEvent& e) {
wxChar ch[2] = {0,0};
ch[0] = e.GetUnicodeKey();
if (ch[0]<32) { e.Skip(); return; }
long time = e.GetTimestamp();
if (time-lastInputTime>600) input.clear();
lastInputTime=time;
input += U(ch);
int selection = lcList->GetFirstSelected(), len = lcList->GetItemCount();
for (int i=input.size()>1?0:1; i<len; i++) {
int indexInList = (selection + i)%len;
int index = lcList->GetItemData(indexInList);
if (app.playlist[index].match(input, index)) {
selection=indexInList;
break;
}}
lcList->Focus(selection);
}

void PlaylistWindow::OnFilterTextChange (wxCommandEvent& e) {
app.win->setTimeout(1000, [&](){ updateList(); });
}

void PlaylistWindow::OnFilterTextEnter (wxCommandEvent& e) {
app.win->setTimeout(100, [&](){ updateList(); lcList->SetFocus(); });
}

void PlaylistWindow::updateList () {
string filter = U(tfFilter->GetValue());
int selection = lcList->GetFirstSelected(), newSelection = wxNOT_FOUND;
if (selection>=0) selection = lcList->GetItemData(selection);
else selection = app.playlist.curIndex;
lcList->Freeze();
lcList->ClearAll();
lcList->AppendColumn(U(translate("tag_title")));
lcList->AppendColumn(U(translate("tag_length")), wxLIST_FORMAT_RIGHT);
for (int i=0, k=0, n=app.playlist.size(); i<n; i++) {
auto& item = app.playlist[i];
if (filter.size() && !item.match(filter)) continue;
auto lastSlash = item.file.find_last_of("/\\");
string sFile = item.file.substr(lastSlash==string::npos? 0 : lastSlash+1);
string sTitle = item.title.size()? item.title : sFile;
string sLength = item.length>0? format("%0$2d:%0$2d", item.length/60, item.length%60) : translate("Unknown");
lcList->InsertItem(k, U(sTitle));
lcList->SetItem(k, 1, U(sLength));
lcList->SetItemData(k, i);
if (selection==i) newSelection=k;
k++;
}
lcList->Thaw();
lcList->Focus(newSelection);
}

