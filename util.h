#ifndef _UTILH_
#define _UTILH_
#include <stdint.h>
#include <string.h>
#include <WString.h>
#include<bits/stdc++.h>

using namespace std;

// Board 1 is production board
// Board 2 is test board with 240 display hat
// Board 3 is test board with 160 display
#define BOARD 1

#if BOARD==1
#define DISPLAY 160
#define BLPIN   33
#define BTNPIN1 25
#define BTNPIN2 26
#define BTNPIN3 27
#define TFT_CS  15
#elif BOARD==2
#define DISPLAY 240
#define BLPIN   13
#define BTNPIN1 14  
#define BTNPIN2 27
#define BTNPIN3 26
#define TFT_CS  15
#else
#define DISPLAY 160
#define BLPIN   33
#define BTNPIN1 25
#define BTNPIN2 26
#define BTNPIN3 27
#define TFT_CS  13
#endif

#define TFT_RST     4  
#define TFT_DC      16

#define DEBUG 1
#define FSTYPE 0

typedef struct {
    uint8_t encryption;
    uint8_t channel;
    int8_t RSSI;
    String SSID;
    String BSSID;
    uint8_t iBSSID[6];
} wifi_ap;

typedef enum {
  SNIFF_IDLE,
  SNIFF_RUNNING
} sniff_state;

typedef enum {
  SORTBYTIME,
  SORTBYRSSI,
  SORTBYPKT
} sortOrder;

typedef struct {
  String name;
  uint8_t number;  
} menu_item;



extern std::vector<menu_item> m_MainMenu;
extern std::vector<menu_item> m_WiFiMenu;
extern std::vector<menu_item> m_BLEMenu;
extern uint32_t pCount;
extern std::deque<uint32_t> activity;

String str_truncate(const char *input, uint8_t str_len, bool pad=false);
String str_truncate(String *input, uint8_t str_len, bool pad=false);
bool wifiScanStop(void);
bool macs_match(uint8_t* mac1, uint8_t* mac2);
uint32_t getActivitySize();
uint32_t getMaxActivity();
uint32_t getActivity(uint16_t index);
void pushCount(uint16_t maxSize);
#endif
