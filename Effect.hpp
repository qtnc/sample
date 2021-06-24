#ifndef ___EFFECT0_1____
#define ___EFFECT0_1____
#include<vector>
#include<string>
#include<iosfwd>

struct EffectDesc {
int fxID;
std::string fxName;
std::string paramTypes;
std::vector<std::string> paramNames;
};

struct EffectParams {
int fxType;
unsigned long handle;
bool on;
std::string name;
char buf[256];
EffectParams ();
void setFloat (int index, float value);
void setInt (int index, int value);
static std::vector<EffectParams> read (std::istream& in);
const char* data () const { return buf; }
};

#endif
