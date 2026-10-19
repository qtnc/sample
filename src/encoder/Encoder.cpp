#include "Encoder.hpp"
using namespace std;

extern void encAddWAV ();
extern void encAddMP3 ();
extern void encAddOGG ();
extern void encAddAAC ();
extern void encAddFlac ();
extern void encAddOpus ();

vector<shared_ptr<Encoder>> Encoder::encoders;

void encAddAll () {
encAddWAV();
encAddMP3();
encAddOGG();
encAddAAC();
encAddOpus();
encAddFlac();
}

