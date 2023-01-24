#ifndef ____WINLIVEREGION___1
#define ____WINLIVEREGION___1
#define UNICODE
#include<winsock2.h>
#include<windows.h>
#include "objbase.h"
#include "oleacc.h"
#include "uiautomation.h"
#include "wxWidgets.hpp"

#define EVENT_OBJECT_LIVEREGIONCHANGED 0x8019

#define Assertive 2
#define Polite 1

bool lrSetLiveRegion (wxWindow* win, int value);
bool lrLiveRegionUpdated (wxWindow* win);

#endif

