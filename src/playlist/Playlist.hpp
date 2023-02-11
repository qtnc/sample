#ifndef ____PLITEM_0____
#define ____PLITEM_0____
#include<string>
#include<memory>
#include<vector>
#include<functional>

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
double length = -1, replayGain = 0;

PlaylistItem (const std::string& file_): file(file_), title(), length(-1), replayGain(0)  {}
bool match (const std::string& s, int index=-1);
void loadMetaData  (unsigned long handle);
void loadMetaData  (unsigned long handle, struct PropertyMap& tags);
};

struct Playlist {
std::vector<std::shared_ptr<PlaylistItem>> items;
std::shared_ptr<PlaylistFormat> format = nullptr;
std::string file;
int curIndex=-1, curPosition=0, curSubindex=-1;

inline int size () { return items.size(); }
inline void clear () { items.clear(); }
inline PlaylistItem& operator[] (int n) { return *items[n]; }
inline PlaylistItem& current () { return (*this)[curIndex]; }
inline void erase (int i) { items.erase(items.begin()+i); }
inline void erase () { erase(curIndex); }
PlaylistItem& add (const std::string& file, int n = -1);
void sort (const std::function<bool(const std::shared_ptr<PlaylistItem>&, const std::shared_ptr<PlaylistItem>&)>& func);
void shuffle (int fromIndex=0, int toIndex=-1);
bool load (const std::string& file);
bool save (const std::string& file = std::string());


static void initFormats ();

static std::vector<std::shared_ptr<PlaylistFormat>> formats;
};

#endif
