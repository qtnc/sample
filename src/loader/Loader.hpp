#ifndef ___LOADER_1___
#define ___LOADER_1___
#include<string>
#include<vector>
#include<memory>

struct Loader {
std::string name, extension;

Loader (const std::string& n, const std::string& e): name(n), extension(e) {}

virtual unsigned long load (const std::string& file, unsigned long flags) = 0;

static std::vector<std::shared_ptr<Loader>> loaders;
};

#endif
