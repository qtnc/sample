#ifdef __WIN32
#define UNICODE
#include<winsock2.h>
#endif
#include "Encoder.hpp"
#include "cpprintf.hpp"
#include "bass.h"
#include "bassenc.h"
#include "bassenc_flac.h"
using namespace std;

struct EncoderFlac: Encoder {

EncoderFlac (): 
Encoder("Free lossless audio codec (flac)", "flac", "audio/flac")
{}

string getOptions () {
return "--best";
}

string getOptions (PlaylistItem& item) {
return getOptions();
}

virtual DWORD startEncoderFile (PlaylistItem& item, DWORD input, const std::string& file) final override {
string options = getOptions(item);
return BASS_Encode_FLAC_StartFile(input, const_cast<char*>(options.c_str()), BASS_ENCODE_AUTOFREE, const_cast<char*>(file.c_str()) );
}

virtual DWORD startEncoderStream (DWORD input) final override {
string options = getOptions();
return BASS_Encode_FLAC_Start(input, const_cast<char*>(options.c_str()), BASS_ENCODE_AUTOFREE, nullptr, nullptr);
}

};

void encAddFlac () {
Encoder::encoders.push_back(make_shared<EncoderFlac>());
}
