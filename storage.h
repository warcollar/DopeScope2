#include "FS.h"
#include "SPIFFS.h"
#include "FFat.h"
#include "util.h"
#define FORMAT_SPIFFS_IF_FAILED true
#define FORMAT_FFAT_IF_FAILED true

void format();
bool FSinit();
File openFile(const char * path, const char * fmode);
