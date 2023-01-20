#ifndef ____BASSPLUGINSTRUCT_1
#define ____BASSPLUGINSTRUCT_1
#include "WXWidgets.hpp"
#include "bass.h"

struct BassPlugin {
HPLUGIN plugin;
wxString name;
bool enabled;

BassPlugin (HPLUGIN p, const wxString& n, bool e): plugin(p), name(n), enabled(e) {}
};

#endif