#ifndef ___CASTER0_1____
#define ___CASTER0_1____
#include "../playlist/Playlist.hpp"
#include<string>
#include<memory>
#include<vector>
#include "../common/bass.h"

#define CF_SERVER 1
#define CF_PORT 2
#define CF_PASSWORD 4
#define CF_USERNAME 8
#define CF_MOUNT 16

struct Caster {
std::string name;
int flags;
Caster (const std::string& n, int f): name(n), flags(f) {}
virtual DWORD startCaster (DWORD inputMix, struct Encoder& encoder, const std::string& server, const std::string& port, const std::string& username, const std::string& password, const std::string& mount) = 0;

static std::vector<std::shared_ptr<Caster>> casters;
};

#endif
