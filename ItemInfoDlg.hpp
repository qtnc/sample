#ifndef ____PLITEMINFODLG____
#define ____PLITEMINFODLG____
#include "WXWidgets.hpp"

struct ItemInfoDlg: wxDialog {
struct PlaylistItem& item;
struct App& app;
wxListBox* lcInfo;
wxTextCtrl *taComment;
wxButton* btnSave;
int tagIndex;
static std::vector<std::string> taglist;

ItemInfoDlg (App& app, wxWindow* parent, PlaylistItem& item, unsigned long channel);

void OnListContextMenu (wxContextMenuEvent& e);
void OnListKeyDown (wxKeyEvent& e);
void OnListDoubleClick ();
void OnListCopySel ();
void OnSave ();
void fillList (unsigned long channel);
};

#endif
