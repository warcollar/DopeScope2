#include "FS.h"
#include "SPIFFS.h"
#include "FFat.h"
#include "util.h"
#include "storage.h"

void format(){
  switch(FSTYPE){
    case 0:
      SPIFFS.format();
      break;
    case 1:
      FFat.format();
      break;
  }
}

bool FSinit(){
  if (DEBUG) Serial.print("Initializing FS:");
  if (DEBUG) Serial.print(FSTYPE);
  switch(FSTYPE){
    case 0:
      return SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED);
      break;
    case 1:
      return FFat.begin(FORMAT_FFAT_IF_FAILED);
      break;
  }
}

File openFile(const char * path, const char * fmode){
  switch(FSTYPE){
    case 0:
      return SPIFFS.open(path, fmode);
    case 1:
      return FFat.open(path, fmode);
  }
}
