#include "Encoder.hpp"
#include "cpprintf.hpp"
using namespace std;

extern void encAddMP3 ();
extern void encAddOGG ();
extern void encAddFlac ();
extern void encAddOpus ();

vector<shared_ptr<Encoder>> Encoder::encoders;

void encAddAll () {
encAddMP3();
encAddOGG();
encAddOpus();
encAddFlac();
}

