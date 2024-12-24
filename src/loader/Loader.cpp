#include "Loader.hpp"
using namespace std;

vector<shared_ptr<Loader>> Loader::loaders;

void ldrAddArchive ();
void ldrAddZlib();
void ldrAddZip();
void ldrAddFfmpeg ();
void ldrAddYtdlp ();

void ldrAddAll () {
ldrAddArchive();
ldrAddZip();
ldrAddZlib();
ldrAddYtdlp();
ldrAddFfmpeg();
}

