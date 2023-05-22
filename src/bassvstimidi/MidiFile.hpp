#include<string>
#include<vector>
#include<iosfwd>
#include<stdexcept>
#include<boost/shared_array.hpp>
using boost::shared_array;

struct MidiEvent {
int tick, track, status, data1, data2;
shared_array<unsigned char> data;
unsigned long long time;

std::string getDataAsString ();
void setDataAsString (const std::string&);
int getDataAsInt24 ();
void setDataAsInt24 (int);
void setData (const void*  a, int l, int d2=0);
inline MidiEvent (int t, int r, int s, int d1=0, int d2=0): tick(t), track(r), status(s), data1(d1), data2(d2), data(nullptr), time(0)  {}
inline MidiEvent (int t, int r, int s, const shared_array<unsigned char>& a, int l, int d2=0): tick(t), track(r), status(s), data1(l), data2(d2), data(a), time(0)  {}
inline MidiEvent (int t, int r, int s, const void* d, int l, int d2=0): tick(t), track(r), status(s), data1(l), data2(d2), data(nullptr), time(0)  { setData(d,l); }
inline MidiEvent (int t, int r, int s, const std::string& str, int d2=0): tick(t), track(r), status(s), data1(0), data2(d2), data(nullptr), time(0) { setDataAsString(str); }
};

struct MidiFile {
std::vector<MidiEvent> events;
int ppq=480, nTracks=1, bpm=120;


bool open (std::istream& in);
bool save (std::ostream& out);
void sortEvents ();
void computeTimes (int frameLen);
};//MidiFile

template<class T> inline shared_array<T> make_shared_array (size_t n) {
return shared_array<T>(new T[n]);
}
