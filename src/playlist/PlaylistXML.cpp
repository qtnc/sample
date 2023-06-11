#include "Playlist.hpp"
#include "../common/stringUtils.hpp"
#include "../common/tinyxml2.h"
#include<fstream>
using namespace std;
using namespace tinyxml2;

std::string makeAbsoluteIfNeeded (const std::string& path, const std::string& basefile);

static inline XMLText* createTextNode (XMLDocument& doc, const char* val) { return doc.NewText(val); }
static inline XMLText* createTextNode (XMLDocument& doc, bool val) { return doc.NewText(val?"true":"false"); }
static inline XMLText* createTextNode (XMLDocument& doc, int i) { 
char val[50]={0};
snprintf(val, 49, "%d", i);
return doc.NewText(val); 
}
static inline XMLText* createTextNode (XMLDocument& doc, double i) { 
char val[50]={0};
snprintf(val, 49, "%.14g", i);
return doc.NewText(val); 
}

template <typename T> static XMLElement* addSimple (XMLElement* parent, const char* elname, T val) {
XMLDocument& doc = const_cast<XMLDocument&>(*parent->GetDocument());
XMLElement* e = doc.NewElement(elname);
e->InsertEndChild(createTextNode(doc, val));
parent->InsertEndChild(e);
return e;
}

static XMLElement* isXSPF (XMLDocument& doc) {
XMLElement* el = nullptr;
if ( (el=doc.FirstChildElement("playlist")) && (el=el->FirstChildElement("trackList")) && (el=el->FirstChildElement("track")) ) return el;
return nullptr;
}

static XMLElement* isASX (XMLDocument& doc) {
XMLElement* el = nullptr;
if ((el=doc.FirstChildElement("asx")) && (el=el->FirstChildElement("entry")) ) return el;
return nullptr;
}

static XMLElement* isWPL (XMLDocument& doc) {
XMLElement* el = nullptr;
if ((el=doc.FirstChildElement("smil")) && (el=el->FirstChildElement("body")) && (el=el->FirstChildElement("seq")) && (el=el->FirstChildElement("media")) ) return el;
return nullptr;
}

struct XSPFFormat: PlaylistFormat {
XSPFFormat (): PlaylistFormat("XML shareable playlist format", "*.xspf") {}
virtual bool checkRead (const string& file) final override {
XMLDocument doc;
return !doc.LoadFile(file.c_str()) && isXSPF(doc);
}
virtual bool checkWrite (const string& file) final override {
return true;
}
bool load (Playlist& list, const string& file) final override {
XMLDocument doc;
if (doc.LoadFile(file.c_str())) return false;
XMLElement* cur = isXSPF(doc);
if (!cur) return false;

do {
XMLElement* e = cur->FirstChildElement("location");
if (!e) break;
auto& it = list.add(makeAbsoluteIfNeeded(e->GetText(), file));
if (e=cur->FirstChildElement("title")) it.title = e->GetText();
if ((e=cur->FirstChildElement("duration")) && !e->QueryDoubleText(&it.length)) it.length/=1000.0;
cur = cur->NextSiblingElement("track");
} while(cur);
return true;
}
virtual bool save (Playlist& list, const string& file) final override {
XMLDocument doc;
doc.InsertEndChild(doc.NewDeclaration());
XMLElement* cur = (XMLElement*)doc.InsertEndChild(doc.NewElement("playlist"));
cur->SetAttribute("version", 1);
cur->SetAttribute("xmlns", "http://xspf.org/ns/0/");
cur = (XMLElement*)cur->InsertEndChild(doc.NewElement("trackList"));
for (int i=0, n=list.size(); i<n; i++) {
auto& it = list[i];
XMLElement *e = doc.NewElement("track");
addSimple(e, "location", it.file.c_str());
if (!it.title.empty()) addSimple(e, "title", it.title.c_str());
if (it.length>0) addSimple(e, "duration", (int)(1000 * it.length));
cur->InsertEndChild(e);
}
return !doc.SaveFile(file.c_str());
}
};

struct ASXFormat: PlaylistFormat {
ASXFormat (): PlaylistFormat("Advanced stream redirector", "*.asx") {}
virtual bool checkRead (const string& file) final override {
XMLDocument doc;
return !doc.LoadFile(file.c_str()) && isASX(doc);
}
virtual bool checkWrite (const string& file) final override {
return true;
}
bool load (Playlist& list, const string& file) final override {
XMLDocument doc;
if (doc.LoadFile(file.c_str())) return false;
XMLElement* cur = isASX(doc);
if (!cur) return false;

do {
XMLElement* e = cur->FirstChildElement("ref");
if (!e || !e->Attribute("href")) break;
auto& it = list.add(makeAbsoluteIfNeeded(e->Attribute("href"), file));
if (e=cur->FirstChildElement("title")) it.title = e->GetText();
if ((e=cur->FirstChildElement("duration")) || (e=cur->FirstChildElement("DURATION")) ) {
int h=0, m=0, s=0, r = sscanf(e->GetText(), "%02d:%02d:%02d", &h, &m, &s);
if (r==1) { s=h; h=m=0; }
else if (r==2) { s=m; m=h; h=0; }
it.length = h*3600 +m*60 +s;
}
cur = cur->NextSiblingElement("entry");
} while(cur);
return true;
}
virtual bool save (Playlist& list, const string& file) final override {
XMLDocument doc;
XMLElement* cur = (XMLElement*)doc.InsertEndChild(doc.NewElement("asx"));
cur->SetAttribute("version", "1.0");
for (int i=0, n=list.size(); i<n; i++) {
auto& it = list[i];
XMLElement *e = doc.NewElement("entry");
((XMLElement*)(e->InsertEndChild(doc.NewElement("ref")))) ->SetAttribute("href", it.file.c_str());
if (!it.title.empty()) addSimple(e, "title", it.title.c_str());
if (it.length>0) {
char buf[15];
snprintf(buf, 14, "%02d:%02d:%02d.00", (int)(it.length/3600), (int)(it.length/60)%60, (int)(it.length)%60);
addSimple(e, "duration", buf);
}
cur->InsertEndChild(e);
}
return !doc.SaveFile(file.c_str());
}
};

struct WPLFormat: PlaylistFormat {
WPLFormat (): PlaylistFormat("Windows media player playlist", "*.wpl") {}
virtual bool checkRead (const string& file) final override {
XMLDocument doc;
return !doc.LoadFile(file.c_str()) && isWPL(doc);
}
virtual bool checkWrite (const string& file) final override {
return true;
}
bool load (Playlist& list, const string& file) final override {
XMLDocument doc;
if (doc.LoadFile(file.c_str())) return false;
XMLElement* cur = isWPL(doc);
if (!cur) return false;
do {
if (!cur->Attribute("src")) break;
auto& it = list.add(makeAbsoluteIfNeeded(cur->Attribute("src"), file));
cur = cur->NextSiblingElement("media");
} while(cur);
return true;
}
virtual bool save (Playlist& list, const string& file) final override {
XMLDocument doc;
doc.InsertEndChild(doc.NewDeclaration("<?wpl version=\"1.0\"?>"));
XMLElement* root = (XMLElement*)doc.InsertEndChild(doc.NewElement("smil"));
XMLElement* cur = (XMLElement*)root->InsertEndChild(doc.NewElement("head"));
cur = (XMLElement*)cur->InsertEndChild(doc.NewElement("meta"));
cur->SetAttribute("name", "generator");
cur->SetAttribute("content", "6player 3.1 http://quentinc.net/");
cur = (XMLElement*)root->InsertEndChild(doc.NewElement("body"));
cur  = (XMLElement*)cur->InsertEndChild(doc.NewElement("seq"));
for (int i=0, n=list.size(); i<n; i++) {
auto& it = list[i];
XMLElement *e = doc.NewElement("media");
e->SetAttribute("src", it.file.c_str());
cur->InsertEndChild(e);
}
return !doc.SaveFile(file.c_str());
}
};


void plAddXML () {
Playlist::formats.push_back(make_shared<XSPFFormat>());
Playlist::formats.push_back(make_shared<ASXFormat>());
Playlist::formats.push_back(make_shared<WPLFormat>());
}
