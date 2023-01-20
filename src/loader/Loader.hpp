#ifndef ___LOADER_1___
#define ___LOADER_1___
#include<string>
#include<vector>
#include<memory>

struct Loader {
virtual unsigned long load (const std::string& file, unsigned long flags) = 0;

static std::vector<std::shared_ptr<Loader>> loaders;
};

#endif
