#ifndef _GRAPHICSH_
#define _GRAPHICSH_
#include <Adafruit_GFX.h>    // Core graphics library
#include "src/Nadafruit/Nadafruit_ST7735.h" // Hardware-specific library for ST7735
#include "src/Nadafruit/Nadafruit_ST7789.h" // Hardware-specific library for ST7735
#include "logo.h"
#include "util.h"
#include "ble_util.h"
#include <BLEAdvertisedDevice.h>
#include "src/btclassic/btclassic.h"
#include "wifi_util.h"

#define CLR_RED     0xF800
#define CLR_ORANGE  0xFC00
#define CLR_YELLOW  0xFFE0
#define CLR_GREEN   0x07E0
#define CLR_CYAN    0x07FF
#define CLR_BLUE    0x001F
#define CLR_MAGENTA 0xF81F
#define CLR_BLACK   0x0000
#define CLR_WHITE   0xFFFF
#define CLR_GRAY  0x7BEF
#define CLR_DKGRAY  0x1082
#define CLR_DKGREEN 0x04A0

#define SZ_FONT     1
#define lineHeight  8


#if DISPLAY==240
extern Adafruit_ST7789 tft;// = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);
#else
extern Adafruit_ST7735 tft;// = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);
#endif

const uint16_t HEADER_COLORS[]= { 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0x8410,0x52aa};
//const uint16_t SELECTED_COLORS[]= { 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0xc618, 0x8410};
const uint16_t SELECTED_COLORS[] = {0x1082, 0x1082, 0x1082, 0x1082, 0x1082, 0x1082, 0x1082, 0x20C2};
void graphicsTest();
// Special Symbols (Font Size)
const unsigned char SYMBOL[][6] =
{
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Open -             0
  0x7e, 0x79, 0x49, 0x79, 0x78, 0x00, // WEP -              1 
  0x7e, 0x79, 0x49, 0x79, 0x7e, 0x00, // WPA -              2 
  0x7e, 0x79, 0x49, 0x79, 0x7e, 0x00, // WPA2 -             3
  0x7e, 0x79, 0x49, 0x79, 0x7e, 0x00, // WPA2/WPA -         4
  0x7f, 0x7f, 0x3e, 0x1c, 0x08, 0x00, // Play Button -      5
  0x7f, 0x7f, 0x00, 0x7f, 0x7f, 0x00, // Pause -            6
  0x54, 0x53, 0x48, 0x27, 0x10, 0x0F, // Wifi -             7
  0x6b, 0x77, 0x41, 0x55, 0x6b, 0x7f, // BT Background -    8
  0x14, 0x08, 0x3e, 0x2a, 0x14, 0x00, // BT Foreground -    9
  0x48, 0x28, 0x1e, 0x28, 0x48, 0x00, // Star -             10
  0x10, 0x08, 0x07, 0x05, 0x07, 0x00, // Small Throbber 1 - 11
  0x00, 0x10, 0x08, 0x07, 0x05, 0x07, // Small Throbber 2 - 12
  0x00, 0x40, 0x20, 0x1c, 0x14, 0x1c, // Small Throbber 3 - 13
  0x40, 0x20, 0x1c, 0x14, 0x1c, 0x00, // Small Throbber 4 - 14
  0x00, 0x00, 0x00, 0x04, 0x02, 0x04, // Up Scroll          15
  0x00, 0x00, 0x00, 0x20, 0x40, 0x20, // Down Scroll        16
};

// Special Symbols (Font Size)
const unsigned char UI[][16] =
{
  0x6b, 0x77, 0x41, 0x55, 0x6b, 0x7f, // BT Background -    9
  0x14, 0x08, 0x3e, 0x2a, 0x14, 0x00, // BT Foreground -    10
};

const uint16_t THROBBER_COLORS[] = {0xF800, 0xFC00, 0xFFE0, 0x07E0, 0x07FF, 0x001F, 0xF81F, 0xFFFF};
const uint8_t THROBBER_OFFSET[][2] = {
1,1, 7,0, 14,1, 15,7, 14,14, 7,15, 1,14, 0,7
};
const unsigned char THROBBER[][8] = 
{
  0x00, 0x0e, 0x1e, 0x1e, 0x3c, 0x70, 0x60, 0x80, // Top Left
  0x00, 0x00, 0x06, 0x3f, 0xff, 0x3f, 0x06, 0x00, // Top Center
  0x80, 0x60, 0x70, 0x3c, 0x1e, 0x1e, 0x0e, 0x00, // Top Right
  0x10, 0x10, 0x38, 0x38, 0x38, 0x7c, 0x7c, 0x38, // Middle Right
  0x01, 0x06, 0x0e, 0x3c, 0x78, 0x78, 0x70, 0x00, // Bottom Right
  0x00, 0x00, 0x60, 0xfc, 0xff, 0xfc, 0x60, 0x00, // Bottom Center
  0x00, 0x70, 0x78, 0x78, 0x3c, 0x0e, 0x06, 0x01, // Bottom Left
  0x38, 0x7c, 0x7c, 0x38, 0x38, 0x38, 0x10, 0x10  // Middle Left
};
void ScreenInit();
//void drawGradient (int x, int y, int h, int w, uint16_t pallet);
void drawGradient (int x, int y, int w, int h, const uint16_t *pallet);
void drawCounter(uint8_t counter);
void drawHeader(uint8_t curr_mode);
void drawSymbol(int symbol, int x, int y, int foreground, int background, bool transparent=false);
void drawImage(uint8_t x, uint8_t y, uint16_t foreground, uint16_t background, uint8_t width, uint8_t height, uint32_t *pixels[32], bool transparent=false);
void drawMenu(uint8_t x, uint8_t y, uint16_t foreground, uint16_t background, bool boarder, char *name, String menuitems[], uint8_t menu_sz, uint8_t menuoption, bool firstrun=false);
void drawMenu(uint8_t x, uint8_t y, uint16_t foreground, uint16_t background, bool boarder, char *name, std::vector<menu_item> *menuitems, uint8_t menu_sz, uint8_t menuoption, bool firstrun=false);
void drawThrobber(uint8_t rotation);
void drawScroll(uint8_t page, uint8_t qty);
//void drawRGBBitmapLR(int16_t x, int16_t y,  const uint16_t *bitmap, int16_t w, int16_t h, bool flipped=false);
void drawRGBBitmap(int16_t x, int16_t y,  const uint16_t *bitmap, int16_t w, int16_t h, bool flipped=false, bool mask=false, float ratio=1, uint16_t fadeColor=0x0000);
bool displaySplash();
uint16_t fadeToColor(uint16_t color1, uint16_t color2, float ratio);
uint16_t fadeToColor(uint16_t color1, uint16_t color2, uint8_t steps, uint8_t step);
void drawWiFiRecord(wifi_ap *ap, bool WIFI_BSSID, bool selected=false, bool clear=false);
//void printBLERecord(char* name, uint8_t rssi, uint32_t cod);
void printBLERecord(BLEAdvertisedDevice* device, bool selected=false, bool clear=false);
void printBCLRecord(gap_device_t* device, bool selected=false, bool clear=false);
void printBCLRecord(char* name, uint8_t rssi, uint32_t cod, bool selected=false, bool clear=false);
void clearThrobber();
void clearScreen(int16_t color= 0x0000);
void clearLowerScreen();
void drawState(bool playing);
void drawLogoEyes(uint16_t x, uint16_t y);
bool displayWiFiDetail(wifi_ap *ap, uint8_t input);
bool displayWiFiSnoop(wifi_ap *ap, uint8_t input);
bool displayBLEDetail(BLEAdvertisedDevice* device, uint8_t input);
void setTextColor(uint16_t foreground, uint16_t background, bool transparent=false);

struct LAYER
{
  uint16_t color;
  uint8_t width;
  uint8_t height;
  uint32_t pixels[16];
};

struct ICON
{
  uint8_t num_layers;
  LAYER layers[4];
};

struct MODE
{
  char name[18];
  ICON icon;
};

#endif
