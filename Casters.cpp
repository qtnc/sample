#include "cpprintf.hpp"
#include "Caster.hpp"
#include "Encoder.hpp"
#include "bass.h"
#include "bassenc.h"
using namespace std;

struct CasterHTTP: Caster {
CasterHTTP (): Caster("HTTP Direct", CF_PORT) {}
virtual DWORD startCaster (DWORD inputMix, Encoder& encoder, const std::string& server, const std::string& port, const std::string& username, const std::string& password, const std::string& mount) final override {
DWORD encoderHandle = encoder.startEncoderStream(inputMix);
DWORD finalPort = 0;
finalPort = BASS_Encode_ServerInit(encoderHandle, port.c_str(), 16384, 16384, BASS_ENCODE_SERVER_META, nullptr, nullptr);
if (!finalPort) {
BASS_Encode_Stop(encoderHandle);
encoderHandle = 0;
}
println("Cast started on port %d", finalPort);
return encoderHandle;
}};

struct CasterIcecast: Caster {
CasterIcecast (): Caster("Icecast", CF_SERVER | CF_PORT | CF_USERNAME | CF_PASSWORD | CF_MOUNT) {}
virtual DWORD startCaster (DWORD inputMix, Encoder& encoder, const std::string& server, const std::string& port, const std::string& username, const std::string& password, const std::string& mount) final override {
DWORD encoderHandle = encoder.startEncoderStream(inputMix);
string address = format("%s:%s/%s", server, port, mount);
string userpass = username.size()? format("%s:%s", username, password) : password;
if (!BASS_Encode_CastInit(encoderHandle, address.c_str(), userpass.c_str(), encoder.mimetype.c_str(), nullptr, nullptr, nullptr, nullptr, nullptr, 0, false)) {
BASS_Encode_Stop(encoderHandle);
encoderHandle = 0;
}
return encoderHandle;
}};

struct CasterShoutcast2: Caster {
CasterShoutcast2 (): Caster("Shoutcast 2", CF_SERVER | CF_PORT | CF_USERNAME | CF_PASSWORD | CF_MOUNT) {}
virtual DWORD startCaster (DWORD inputMix, Encoder& encoder, const std::string& server, const std::string& port, const std::string& username, const std::string& password, const std::string& mount) final override {
DWORD encoderHandle = encoder.startEncoderStream(inputMix);
string address = format("%s:%s,%s", server, port, mount);
string userpass = username.size()? format("%s:%s", username, password) : password;
if (!BASS_Encode_CastInit(encoderHandle, address.c_str(), userpass.c_str(), encoder.mimetype.c_str(), nullptr, nullptr, nullptr, nullptr, nullptr, 0, false)) {
BASS_Encode_Stop(encoderHandle);
encoderHandle = 0;
}
return encoderHandle;
}};

struct CasterShoutcast1: Caster {
CasterShoutcast1 (): Caster("Shoutcast 1.x", CF_SERVER | CF_PORT | CF_PASSWORD) {}
virtual DWORD startCaster (DWORD inputMix, Encoder& encoder, const std::string& server, const std::string& port, const std::string& username, const std::string& password, const std::string& mount) final override {
DWORD encoderHandle = encoder.startEncoderStream(inputMix);
string address = format("%s:%s", server, port);
if (!BASS_Encode_CastInit(encoderHandle, address.c_str(), password.c_str(), encoder.mimetype.c_str(), nullptr, nullptr, nullptr, nullptr, nullptr, 0, false)) {
BASS_Encode_Stop(encoderHandle);
encoderHandle = 0;
}
return encoderHandle;
}};

std::vector<std::shared_ptr<Caster>> Caster::casters = {
make_shared<CasterHTTP>(),
make_shared<CasterIcecast>(),
make_shared<CasterShoutcast1>(),
make_shared<CasterShoutcast2>()
};
