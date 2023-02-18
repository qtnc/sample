#include "MidiFile.hpp"
#include<cmath>
#include<numeric>
#include<algorithm>
#include<sstream>
using namespace std;

#define MThd 0x4D546864
#define MTrk 0x4D54726B

template<class T> static inline void swapEndianess (T& x) {
unsigned char* b = (unsigned char*)(&x);
for (size_t i=0; i<sizeof(x)/2; i++) std::swap(b[i], b[sizeof(x) -i -1]);
}

template<class T> inline void read (istream& in, T& x, bool swap=true) {
in.read(reinterpret_cast<char*>(&x), sizeof(x));
if (swap && sizeof(x)>1) swapEndianess(x);
}

template<class T> inline void write (ostream& out, const T& x, bool swap=true) {
if (swap && sizeof(x)>1) {
T y = x;
swapEndianess(y);
out.write(reinterpret_cast<const char*>(&y), sizeof(y));
}
else out.write(reinterpret_cast<const char*>(&x), sizeof(x));
}

int readVLQ (istream& in) {
int val = 0;
unsigned char x;
do {
read(in, x);
val<<=7;
val|=(x&0x7F);
} while(x>=0x80);
return val;
}

void writeVLQ (ostream& out, int value) {
int buffer = value & 0x7F;
while((value >>= 7) > 0)  {
 buffer <<= 8;
 buffer |= 0x80;
 buffer |= (value &0x7F);
 } 
while(true){
write<unsigned char>(out, buffer&0xFF);
 if(buffer & 0x80) buffer >>= 8;
 else  break;
 }}

void MidiEvent::setData (const void* a, int l, int d2) {
data2=d2;
data1=l;
data = make_shared_array<unsigned char>(l);
memcpy(&data[0], a, l);
}

string MidiEvent::getDataAsString () {
return string( &data[0], &data[data1]);
}

void MidiEvent::setDataAsString (const string& s) {
setData(s.data(), s.size(), data2);
}

int MidiEvent::getDataAsInt24 () {
if (data1!=3) return -1;
return data[2] | (data[1]<<8L) | (data[0]<<16L);
}

void MidiEvent::setDataAsInt24 (int n) {
unsigned char a[3];
a[0] = (n>>16)&0xFF;
a[1] = (n>>8)&0xFF;
a[2] = n&0xFF;
setData(a, 3);
}

bool MidiFile::save (ostream& out) {
vector<vector<MidiEvent>> tracks;
sortEvents();
tracks.push_back(events);
write<int>(out, MThd);
write<int>(out, 6);
write<short>(out, 1);
write<short>(out, tracks.size());
write<short>(out, ppq);
for (vector<MidiEvent>& events: tracks) {
write<int>(out, MTrk);
int delta = 0, status = 0;
auto startpos = out.tellp();
write<int>(out, 0);
for (MidiEvent& e: events) {
writeVLQ(out, e.tick -delta);
delta = e.tick;
if (status!=e.status || e.status>=0xF0)  write<unsigned char>(out, e.status);
status = e.status;
switch(status){
case 0x80 ... 0xBF: 
write<unsigned char>(out, e.data1);
write<unsigned char>(out, e.data2);
break;
case 0xC0 ... 0xDF: 
write<unsigned char>(out, e.data1);
break;
case 0xE0 ... 0xEF: 
write<unsigned char>(out, e.data1&0x7F);
write<unsigned char>(out, e.data1>>7); 
break;
case 0xF0: case 0xF7: 
writeVLQ(out, e.data1); 
if (e.data1>0) out.write(reinterpret_cast<const char*>(&e.data[0]), e.data1); 
break;
case 0xFF: 
write<unsigned char>(out, e.data2);
writeVLQ(out, e.data1); 
if (e.data1>0) out.write(reinterpret_cast<const char*>(&e.data[0]), e.data1); 
break;
default: 
//println("Unknown status: %d. Skipping", status); 
break;
}}
auto endpos = out.tellp();
auto diff = endpos -startpos -4;
out.seekp(startpos);
write<int>(out, diff);
out.seekp(endpos);
}
return true;
}

void MidiFile::sortEvents () {
std::stable_sort(events.begin(), events.end(), [](const MidiEvent& a, const MidiEvent& b){ return a.tick<b.tick; });
}

bool MidiFile::open (istream& in) {
unsigned long magic, hdrlen;
unsigned short midiType, ppq, nTracks;
read(in, magic);
read(in, hdrlen);
read(in, midiType);
read(in, nTracks);
read(in, ppq);
if (magic!=MThd || midiType>=2 || hdrlen!=6) return false; // unsupported MIDI file
for (int t=0; t<nTracks; t++) {
read(in, magic);
read(in, hdrlen);
if (magic!=MTrk) return false;
int curpos = in.tellg(), endpos = curpos+hdrlen;
int delta=0;
unsigned char status=0, newstatus=0, d1=0, d2=0;
while((curpos=in.tellg())<endpos) {
delta += readVLQ(in);
read(in, newstatus);
if (newstatus<0x80) in.putback(newstatus);  // running status
else status = newstatus;
if (status<0x80) return false;
switch(status){
case 0x80 ... 0xBF :
read(in, d1); read(in, d2);
events.push_back(MidiEvent(delta, t, status, d1, d2));
break;
case 0xC0 ... 0xDF :
read(in, d1);
events.push_back(MidiEvent(delta, t, status, d1, 0));
break;
case 0xE0 ... 0xEF :
read(in, d1); read(in, d2);
events.push_back(MidiEvent(delta, t, status, (d2<<7) | (d1&0x7F), 0  ));
break;
case 0xF0: case 0xF7: {
int len = readVLQ(in);
auto a = make_shared_array<unsigned char>(len);
in.read(reinterpret_cast<char*>(&a[0]), len);
events.push_back(MidiEvent(delta, t, status, &a[0], len));
}break;
case 0xFF: {
read(in, d2);
int len = readVLQ(in);
auto a = make_shared_array<unsigned char>(len);
in.read(reinterpret_cast<char*>(&a[0]), len);
if (d2!=47) events.push_back(MidiEvent(delta, t, status, a, len, d2));
}break;
default:
//println("Unknown status: %d", status);
return false;
}}}
this->ppq = ppq;
this->nTracks = nTracks;
return true;
}

inline unsigned long long computePosition(unsigned long long lastFrameChange, unsigned long long lastTickChange, unsigned long long tick, unsigned long long  mpq, unsigned long long ppq, unsigned long long frameLen) {
return lastFrameChange
+ (tick - lastTickChange)
* (mpq * frameLen)
/ (ppq * 1000000);
}

void MidiFile::computeTimes (int frameLen) {
unsigned long long mpq = 500000, lastTickChange = 0, lastFrameChange = 0;
//sortEvents();
for (auto& e: events) {
if (e.status==255 && e.data2==81) {
lastFrameChange = computePosition(lastFrameChange, lastTickChange, e.tick, mpq, ppq, frameLen);
lastTickChange = e.tick;
mpq = e.getDataAsInt24();
}
e.time = computePosition(lastFrameChange, lastTickChange, e.tick, mpq, ppq, frameLen);
}
}
