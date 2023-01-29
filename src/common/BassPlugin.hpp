#ifndef ____BASSPLUGINSTRUCT_1
#define ____BASSPLUGINSTRUCT_1
#include "WXWidgets.hpp"
#include "bass.h"

struct BassPlugin {
HPLUGIN plugin;
wxString name;
int priority;
bool enabled;

BassPlugin (HPLUGIN p, const wxString& n, int pr, bool e): plugin(p), name(n), priority(pr), enabled(e) {}
};

#endif