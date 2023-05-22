#ifndef ___LOADER_1___
#define ___LOADER_1___
#include<string>
#include<vector>
#include<memory>

#define LF_FILE 1
#define LF_URL 2
#define LF_WILDCARD 4

struct Loader {
std::string name, extension;
int flags;

Loader (const std::string& n, const std::string& e, int f): name(n), extension(e), flags(f) {}

virtual unsigned long load (const std::string& file, unsigned long flags) = 0;

static std::vector<std::shared_ptr<Loader>> loaders;
};

#endif
