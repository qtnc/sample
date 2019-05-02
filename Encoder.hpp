#ifndef ___ENCODER0_1____
#define ___ENCODER0_1____
#include "Playlist.hpp"
#include<string>
#include<memory>
#include<vector>
#include "bass.h"
#include "bassenc.h"

void encAddAll ();

struct Encoder {
std::string name, extension, mimetype;
Encoder (const std::string& nam, const std::string& ext, const std::string& mt): name(nam), extension(ext), mimetype(mt)  {}
virtual DWORD startEncoderFile (PlaylistItem& item, DWORD input, const std::string& file) = 0;
virtual DWORD startEncoderStream (DWORD inputMix) = 0;
virtual bool hasFormatDialog () { return false; }
virtual bool showFormatDialog (struct wxWindow* parent) { return false; }

static std::vector<std::shared_ptr<Encoder>> encoders;
};

#endif
