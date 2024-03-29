#include "stringUtils.hpp"
#include<fmt/format.h>
using namespace std;
using fmt::format;

string formatTime (int n) {
if (n<3600) return format("{:02d}:{:02d}", n/60, n%60);
else return format("{:d}:{:02d}:{:02d}", n/3600, (n/60)%60, n%60);
}

string formatSize (long long size) {
static const char suffixes[] = "oKMGT";
const char* suffix = suffixes;
double divider = 1;
while(size/divider>1000) {
suffix++;
divider*=1024;
}
return format("{:.3g}{:c}", size/divider, *suffix);
}

