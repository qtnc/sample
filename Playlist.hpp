#ifndef ____PLITEM_0____
#define ____PLITEM_0____
#include<string>
#include<memory>
#include<vector>
#include<boost/container/flat_map.hpp>

struct PlaylistFormat {
std::string name, extension;
PlaylistFormat (const std::string& n, const std::string& e): name(n), extension(e) {}
virtual bool checkRead (const std::string& file) = 0;
virtual bool checkWrite (const std::string& file) = 0;
virtual bool load (struct Playlist& playlist, const std::string& file) = 0;
virtual bool save (struct Playlist& playlist, const std::string& file) = 0;
};

struct PlaylistItem {
std::string file, title;
int length = -1;
boost::container::flat_map<std::string, std::string> tags;

PlaylistItem (const std::string& file_): file(file_) {}
void loadTagsFromBASS (unsigned long handle);
bool match (const std::string& s, int index=-1);
};

struct Playlist {
std::vector<std::shared_ptr<PlaylistItem>> items;
std::shared_ptr<PlaylistFormat> format = nullptr;
std::string file;
int curIndex=-1, curPosition=0;

inline int size () { return items.size(); }
inline void clear () { items.clear(); }
inline PlaylistItem& operator[] (int n) { return *items[n]; }
inline PlaylistItem& current () { return (*this)[curIndex]; }
inline void erase (int i) { items.erase(items.begin()+i); }
inline void erase () { erase(curIndex); }
PlaylistItem& add (const std::string& file, int n = -1);
bool load (const std::string& file);
bool save (const std::string& file = std::string());

static std::vector<std::shared_ptr<PlaylistFormat>> formats;
};

#endif
