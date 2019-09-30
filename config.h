#ifndef _CONFIGH_
#define _CONFIGH_
#include <map>
#define FORMAT_SPIFFS_IF_FAILED true

typedef enum {
  TYPE_BOOL,
  TYPE_INT
} config_type;

typedef struct {
  String info;
  int8_t value;
  config_type type;
  int32_t min;
  int32_t max;
} config_item;

extern std::map<String, config_item> configuration;

bool loadConfig();
bool configMenu(uint8_t myInput);

#endif
