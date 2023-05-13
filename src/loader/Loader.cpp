#include "Loader.hpp"
using namespace std;

vector<shared_ptr<Loader>> Loader::loaders;

void ldrAddArchive ();
void ldrAddZlib();
void ldrAddFfmpeg ();
void ldrAddYtdlp ();

void ldrAddAll () {
ldrAddArchive();
ldrAddZlib();
ldrAddYtdlp();
ldrAddFfmpeg();
}

