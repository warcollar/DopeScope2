#include "graphics.h"
#include "util.h"
#include "src/btclassic/btclassic.h"
#include <BLEUtils.h>
#include <BLEAdvertisedDevice.h>
#include "wifi_util.h"
#include "ble_util.h"
#include "version.h"

// Define Screensize
// 240 is 240x240 ST7789 based display
// 160 is 160x80  ST7735 based display
#ifndef DISPLAY
#define DISPLAY 240
#endif

#if DISPLAY==240
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_DC,  TFT_CS, TFT_RST);
#else
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);
#endif

bool splashRun=false;
uint32_t splashTimer;
uint16_t scr_width=0;
uint16_t scr_height=0;

MODE modes[3] = {
  {
    "WiFi Scan",
    {1,{{CLR_RED,6,7,{ 0x54, 0x53, 0x48, 0x27, 0x10, 0x0F }}}}
  },
  {
    "Blutooth LE Scan",
    {2,{{CLR_WHITE,6,7,{ 0x6b, 0x77, 0x41, 0x55, 0x6b, 0x7f }},
        {CLR_BLUE, 6,7,{ 0x14, 0x08, 0x3e, 0x2a, 0x14, 0x00 }}}}
  },
  {
    "Settings",
    {2,{{CLR_DKGREEN,7,7,{ 0x1c, 0x3e, 0x7f, 0x45, 0x7f, 0x3e, 0x1c }},
        {CLR_WHITE, 7,7,{ 0x00, 0x00, 0x00, 0x58, 0x00, 0x00, 0x00 }}}}
  }
};

/*MODE modes[4] = {
  {
    "WiFi Scan",
    {1,{{CLR_RED,6,7,{ 0x54, 0x53, 0x48, 0x27, 0x10, 0x0F }}}}
  },
  {
    "Blutooth LE Scan",
    {2,{{CLR_WHITE,6,7,{ 0x6b, 0x77, 0x41, 0x55, 0x6b, 0x7f }},
        {CLR_BLUE, 6,7,{ 0x14, 0x08, 0x3e, 0x2a, 0x14, 0x00 }}}}
  },
  {
    "Blutooth Scan",
    {2,{{CLR_BLUE,6,7,{ 0x6b, 0x77, 0x41, 0x55, 0x6b, 0x7f }},
        {CLR_WHITE, 6,7,{ 0x14, 0x08, 0x3e, 0x2a, 0x14, 0x00 }}}}
  },
  {
    "Settings",
    {2,{{CLR_DKGREEN,7,7,{ 0x1c, 0x3e, 0x7f, 0x45, 0x7f, 0x3e, 0x1c }},
        {CLR_WHITE, 7,7,{ 0x00, 0x00, 0x00, 0x58, 0x00, 0x00, 0x00 }}}}
  }
};*/

void ScreenInit(){
#if DISPLAY==240
  tft.init(240,240);
  scr_width=240;
  scr_height=240;
#else
  tft.initR(INITR_MINI160x80_ODD);   // initialize a ST7735S chip, mini display
  scr_width=169;
  scr_height=80;
  tft.invertDisplay(true);
#endif
  clearScreen();
  tft.setRotation(3);
  tft.setTextWrap(false);
}

void setTextColor(uint16_t foreground, uint16_t background, bool transparent) {
  if (transparent){
    tft.setTextColor(foreground);
  }else{
    tft.setTextColor(foreground, background);
  }
}

void clearScreen(int16_t color) {
  tft.fillScreen(color);
}

void drawSymbol(int symbol, int x, int y, int foreground, int background, bool transparent){
  int SYMSIZE=6;
  //Serial.printf("Printing symbol %d at %d x %d\n", symbol, x, y);
  for (int i=0; i<SYMSIZE; i++){
    char myChar = SYMBOL[symbol][i];
    for (int j=0; j<8; j++){
      if (myChar & (1<<j)){
        tft.drawPixel(x + i, y + j, foreground);
      } else {
        if (! transparent){
          tft.drawPixel(x + i, y + j, background);
        }
      }
    }
  }
}

void drawRGBBitmapLR(int16_t x, int16_t y,  const uint16_t *bitmap, int16_t w, int16_t h, bool flipped) {
    tft.startWrite();
    for(int16_t i=0; i<w; i++ ) {
        for(int16_t j=0; j<h; j++) {
          tft.writePixel(flipped ? abs((x+w)-i) : x + i, y+j, bitmap[i * h + j]);
        }
    }
    tft.endWrite();
}

void drawRGBBitmap(int16_t x, int16_t y,  const uint16_t *bitmap, int16_t w, int16_t h, bool flipped, bool mask, float ratio, uint16_t fadeColor) {
    tft.startWrite();
    for(int16_t j=0; j<h; j++) {
        for(int16_t i=0; i<w; i++ ) {
          uint16_t color = bitmap[j * w + i];
          if ((!mask) || (color > 0)){
            if ((color > 0) && (ratio < 1)){
              uint16_t newcolor = fadeToColor(color,fadeColor, ratio);
              color = newcolor;
            }
            tft.writePixel(flipped ? (x+w)-i : x + i, y+j, color);
          }
        }
    }
    tft.endWrite();

}

void drawLogoEyes(uint16_t x, uint16_t y) {
  const uint8_t separation = 51;
  drawRGBBitmap(x, y, logoEye, 18, 14, false);
  drawRGBBitmap(x + separation, y, logoEye, 18, 14, true);
}

bool displaySplash(){
  bool complete = false;
  uint16_t eyes_time = 750;
  uint16_t fade_time = 1000;
  uint16_t hold_time = 1250;
  uint16_t fadeout_time = eyes_time + fade_time + hold_time;
  uint16_t full_time = (fade_time * 2) + hold_time;
  uint16_t slide_time = 750;
  uint16_t slide_start = eyes_time + full_time;
  uint8_t fade_steps = 10;
  uint8_t slide_steps = 16;
  uint32_t logotime = millis() - splashTimer;
  static bool sliding;
  static uint8_t curr_step;
  uint8_t slide_interval = slide_time / slide_steps;
  uint16_t centerx=tft.width()/2;
  uint16_t centery=tft.height()/2;
  uint16_t logowidth=122;
  uint16_t logoheight=68;
  uint16_t eyewidth=18;
  uint16_t eyeheight=14;
  //Draw eyes for the first 3 seconds
  if (! splashRun){
    sliding = false;
    slide_steps = 14;
    splashTimer=millis();
    drawLogoEyes(centerx-38, centery-16);
    //24);
    splashRun=true;
  } else if ((logotime > eyes_time) && (logotime < eyes_time + fade_time)){
    // Do fade in
    float ratio = (float)(logotime - eyes_time)/ fade_time;
    drawRGBBitmap(centerx-61, centery-34, logoFull, 122, 68, false, true, ratio);  
  } else if ((logotime > fadeout_time) && (logotime < slide_start)){
    // Do fade out
    float ratio = (float)1.0 - (float)(logotime - fadeout_time)/ fade_time;
    drawRGBBitmap(centerx-61, centery-34, logoFull, 122, 68, false, true, ratio); 
  } else if (logotime > slide_start) {
    if (!sliding){
      //clearScreen();
      drawRGBBitmap(centerx-61, centery-34, logoFull, 122, 68, false, true, 0);
      sliding=true;
      curr_step=0;
    }
    uint16_t step_time = curr_step * slide_interval;
    if (logotime >=slide_start + step_time) {
      drawLogoEyes(centerx-38, centery-16-curr_step);
      tft.drawLine(0, centery-1-curr_step, tft.width()-1, centery-1-curr_step, CLR_BLACK);
      curr_step += 1;
    }
  }
  if (logotime > eyes_time + full_time + slide_time) {
    complete = true;
  }
  return complete;
}
void drawThrobber(uint8_t rotation) {
  uint8_t masterx = (tft.width()/2)-12;
  uint8_t mastery = (tft.height()/2)-12;
  //int masterx=68;
  //int mastery=28;
  for ( int t=0; t<8; t++){
    uint16_t color = THROBBER_COLORS[(t+rotation) % 8];
    int x = masterx + THROBBER_OFFSET[t][0];
    int y = mastery + THROBBER_OFFSET[t][1];
    
    for (int i=0; i<8; i++){
      char myChar = THROBBER[t][i];
      for (int j=0; j<8; j++){
        if (myChar & (1<<j)){
          tft.drawPixel(x + i, y + j, color);
        }
      }
    }
  }
}

uint16_t fadeToColor(uint16_t color1, uint16_t color2, uint8_t steps, uint8_t step){
  float ratio = (float)step/(float)steps;
  return fadeToColor(color1, color2, ratio);
}

uint16_t fadeToColor(uint16_t color1, uint16_t color2, float ratio){
  uint8_t r1 = color1 >> 11;
  uint8_t g1 = color1 >> 5 & 0x3F;
  uint8_t b1 = color1 & 0x1F;
  uint16_t curr_red;
  uint16_t curr_green;
  uint16_t curr_blue;    
  uint16_t newcolor;
  if (color2 > 0){
    uint8_t r2 = color2 >> 11;
    uint8_t g2 = color2 >> 5 & 0x3F;
    uint8_t b2 = color2 & 0x1F;
    int16_t d_red   = r1 - r2;
    int16_t d_green = g1 - g2;
    int16_t d_blue  = b1 - b2;
    curr_red   = r1 - (int)(d_red   * ratio);
    curr_green = g1 - (int)(d_green * ratio);
    curr_blue  = b1 - (int)(d_blue  * ratio);
  } else {
    curr_red   = (uint8_t)(r1 * ratio);
    curr_green = (uint8_t)(g1 * ratio);
    curr_blue  = (uint8_t)(b1 * ratio);
  }
  newcolor = (curr_red << 11) + (curr_green << 5) + curr_blue;
  return newcolor;
}

void clearThrobber(){
  uint8_t x = (tft.width()/2)-12;
  uint8_t y = (tft.height()/2)-12;
  tft.fillRect(x,y,24,24,0x0000);
}

void drawScroll(uint8_t index, uint8_t qty) {
  const uint8_t pageSize = 8;
  uint8_t slideSize = 64;
  uint8_t barSize = 4;
  uint8_t slideArea = slideSize - barSize;
  uint16_t up_color = CLR_DKGRAY;
  uint16_t dwn_color = CLR_DKGRAY;
  if (index < qty) dwn_color = CLR_GREEN;
  if (index > qty) up_color = CLR_GREEN;

  drawSymbol(15, tft.width()-7, 10, up_color, CLR_BLACK, true);
  drawSymbol(16, tft.width()-7, tft.height()-8, dwn_color, CLR_BLACK, true);
  tft.drawLine(tft.width()-3, 13, tft.width()-3, tft.height()-3, CLR_DKGRAY);
  if (qty > pageSize) {
    float pixPerIndex = (float)slideArea/qty;
    uint8_t saLocation = index * pixPerIndex;    
    uint8_t locationY = 13 + saLocation;
    tft.drawLine(tft.width()-3, locationY, tft.width()-3, locationY + barSize, CLR_GREEN);
  }
}

void drawGradient (int x, int y, int w, int h, const uint16_t *pallet){
  for (int i=0; i< w; i++){
    for (uint8_t j=0; j<h; j++){
      tft.drawPixel(x+i, y+j, pallet[j]);
    }
  }
}

void drawImage(uint8_t x, uint8_t y, uint16_t foreground, uint16_t background, uint8_t width, uint8_t height, uint32_t *pixels, bool transparent){
  for (int i=0; i<width; i++){
    for (int j=0; j<height; j++){
      if (pixels[i] & (1<<j)){
        tft.drawPixel(x + i, y + j, foreground);
      } else {
        if (! transparent){
          tft.drawPixel(x + i, y + j, background);
        }
      }
    }
  }
}

void drawIcon(ICON icon,uint8_t x, uint8_t y){
  for (int i = 0; i < icon.num_layers; i++){
    LAYER layer = icon.layers[i];
    drawImage(x, y, layer.color, CLR_BLACK, layer.width, layer.height, layer.pixels, true);
  }
}

void drawHeader(uint8_t curr_mode) {
    drawGradient(0,0,tft.width(), 9, HEADER_COLORS);
    drawIcon(modes[curr_mode].icon, 0, 0);
    tft.setCursor(11,0);
    tft.setTextColor(ST77XX_BLACK);
    tft.print(modes[curr_mode].name);
}

void drawCounter(uint8_t counter){
  drawGradient(tft.width()-15, 0, 14, 9, HEADER_COLORS);
  tft.setCursor(tft.width()-14,0);
  tft.setTextColor(ST77XX_RED);
  tft.print(counter);
}

void drawState(bool paused){
  drawGradient(tft.width() - 35, 0, 10, 9, HEADER_COLORS);
  //tft.setCursor(tft.width() - 24,0);
  if (!paused){
    drawSymbol(5, tft.width() - 34, 0, CLR_DKGREEN, CLR_BLACK, true);
  } else {
    drawSymbol(6, tft.width() - 34, 0, CLR_ORANGE, CLR_BLACK, true);
  }
}

void drawMenu(uint8_t x, uint8_t y, uint16_t foreground, uint16_t background, bool border, char *name, String menuitems[], uint8_t menu_sz, uint8_t menuoption, bool firstrun){
  uint8_t padding = 4;
  uint8_t textLen = 0;
  for (int j=0; j< menu_sz; j++){
    if (menuitems[j].length() > textLen){
      textLen = menuitems[j].length();
    }
  }
  if (firstrun){
    tft.fillRect(x-padding, y+3, (6 * (textLen+2))+(padding*2), (8 * (menu_sz+1)) + padding, background);
    if (border){
      tft.drawRect(x-padding, y+3, (6 * (textLen+2))+(padding*2), (8 * (menu_sz+1)) + padding, foreground);
    }
  }
  tft.setTextColor(foreground, background);
  tft.setCursor(x,y);
  tft.println(name);
  tft.setCursor(x, tft.getCursorY()+2);
  for (int i=0;i<menu_sz; i++){
    tft.setCursor(x, tft.getCursorY());
    if (i==menuoption){
      tft.print("> ");
    }else{
      tft.print("  ");
    }
    tft.println(menuitems[i]);
  }
}
//Need to add filter ability
void drawMenu(uint8_t x, uint8_t y, uint16_t foreground, uint16_t background, bool border, char *name, vector<menu_item> *menuitems, uint8_t menu_sz, uint8_t menuoption, bool firstrun){
  uint8_t padding = 4;
  uint8_t textLen = 0;
  for (int j=0; j< menu_sz; j++){
    //menu_item m = menuitems->at(j);
    if (menuitems->at(j).name.length() > textLen){
      textLen = menuitems->at(j).name.length();
    }
  }
  if (firstrun){
    tft.fillRect(x-padding, y+3, (6 * (textLen+2))+(padding*2), (8 * (menu_sz+1)) + padding, background);
    if (border){
      tft.drawRect(x-padding, y+3, (6 * (textLen+2))+(padding*2), (8 * (menu_sz+1)) + padding, foreground);
    }
  }
  tft.setTextColor(foreground, background);
  tft.setCursor(x,y);
  tft.println(name);
  tft.setCursor(x, tft.getCursorY()+2);
  for (int i=0;i<menu_sz; i++){
    tft.setCursor(x, tft.getCursorY());
    if (i==menuoption){
      tft.print("> ");
    }else{
      tft.print("  ");
    }
    tft.println(menuitems->at(i).name);
  }
}

void setRSSIColor(uint8_t RSSI, bool transparent=false){
  setTextColor(CLR_BLUE, CLR_BLACK, transparent);
  if(RSSI < 60)
  {
      setTextColor(CLR_GREEN, CLR_BLACK, transparent);
  }
  if (RSSI > 90)
  {
      setTextColor(CLR_RED, CLR_BLACK, transparent);
  } 
}

void printRSSI(uint8_t RSSI, uint16_t fontPos){
  tft.setCursor(-3,fontPos);
  tft.print("-");
  tft.setCursor(3,fontPos);
  tft.print(RSSI);
}

void printStar(bool hasStar, uint16_t color, uint16_t badColor, uint16_t background, bool selected){
  if (!hasStar){
    color = badColor;
  }
  drawSymbol(10, tft.getCursorX(), tft.getCursorY(), color, background, selected);
  tft.setCursor(tft.getCursorX() + 6, tft.getCursorY());
}

void printBLERecord(BLEAdvertisedDevice* device, bool selected, bool clear){
  uint16_t fontPos = tft.getCursorY();
  uint16_t foreground = CLR_WHITE;
  uint16_t background= CLR_BLACK;
  if (selected){
    foreground=CLR_WHITE;
    drawGradient(0,fontPos ,tft.width()-4, 8, SELECTED_COLORS);
  } else if(clear){
    tft.fillRect(0,tft.getCursorY(),tft.width()-4,8, background);
  }
  setTextColor(foreground, background, selected);
  
  int BLE_RSSI = device->getRSSI();
  int rssi = abs(BLE_RSSI);
  setRSSIColor(rssi, selected);
  printRSSI(rssi, fontPos);

  setTextColor(foreground, background, selected);
  if (device->haveName()) {
    String temp = device->getName().c_str();
    tft.print(str_truncate(device->getName().c_str(), 18, true));
  }else{
    String temp = device->getAddress().toString().c_str();
    temp.toUpperCase();
    tft.print(str_truncate(&temp, 18, true));
  }
  printStar(device->haveAppearance(),       CLR_ORANGE,  CLR_GRAY, background, selected);
  printStar(device->haveManufacturerData(), CLR_BLUE,    CLR_GRAY, background, selected);
  printStar(device->haveServiceData(),      CLR_CYAN,    CLR_GRAY, background, selected);
  printStar(device->haveServiceUUID(),      CLR_MAGENTA, CLR_GRAY, background, selected);
  
  setTextColor(foreground, background, selected);
  tft.println();
}

void printBCLRecord(gap_device_t* device, bool selected, bool clear){
  char bda_str[18];
  sprintf(bda_str, "%02X:%02X:%02X:%02X:%02X:%02X",
      device->bda[0], device->bda[1], device->bda[2], device->bda[3], device->bda[4], device->bda[5]);
  printBCLRecord(bda_str, abs(device->rssi), device->cod, selected, clear);
}

void printBCLRecord(char* name, uint8_t rssi, uint32_t cod, bool selected, bool clear) {
  uint16_t fontPos = tft.getCursorY();
  uint16_t foreground = CLR_WHITE;
  uint16_t background= CLR_BLACK;
  if (selected){
    foreground=CLR_WHITE;
    drawGradient(0,fontPos ,tft.width()-4, 8, SELECTED_COLORS);
  } else if(clear){
    tft.fillRect(0,tft.getCursorY(),tft.width()-4,8, background);
  }
  setRSSIColor(rssi, selected);
  printRSSI(rssi, fontPos);
  tft.setCursor(21, fontPos);
  setTextColor(foreground, background, selected);
  tft.printf("%s ", name);
  tft.printf("%x", cod);
  tft.println();
}

void graphicsTest(){
  tft.setCursor(0,0);
  tft.println("abcdefghijklmnopq");
  tft.println("ABCDEFGHIJKLMNOPQ");
  tft.println("!@#$%^&*()_+ <>?:");
  tft.println("1234567890,./;'[]");
}

void drawWiFiRecord(wifi_ap *ap, bool WIFI_BSSID, bool selected, bool clear){
  uint16_t fontPos = tft.getCursorY();
  bool hidden=false;
  uint16_t foreground = CLR_WHITE;
  uint16_t background= CLR_BLACK;
  if (selected){
    foreground=CLR_WHITE;
    drawGradient(0,fontPos ,tft.width()-4, 8, SELECTED_COLORS);
  } else if(clear){
    tft.fillRect(0,tft.getCursorY(),tft.width()-4,8, background);
  }
  setTextColor(foreground, background, selected);
  // Print SSID and RSSI for each network found
  
  String SSID = ap->SSID;
  if (WIFI_BSSID) {
    SSID=ap->BSSID;
  }
  if (SSID.length() == 0){
    hidden=true;
    SSID = ap->BSSID;
  }
  SSID=str_truncate(&SSID, 18, true);

  int RSSI = abs(ap->RSSI);
  setRSSIColor(RSSI, selected);
  printRSSI(RSSI, fontPos);
  
  int encColor=0;
  switch(ap->encryption)
  {
    case 0:
      encColor=CLR_BLACK;
    case 1:
      encColor=CLR_RED;
      break;
    case 2:
      encColor=CLR_BLUE;
      break;   
    case 3:
      encColor=CLR_GREEN;
      break;
    case 4:
      encColor=CLR_YELLOW;
      break;
  }
  
  drawSymbol(ap->encryption, 17, fontPos, encColor, CLR_BLACK, selected);
  tft.setCursor(25, fontPos);
  setTextColor(foreground, background, selected);
  if (hidden){
    setTextColor(CLR_ORANGE, CLR_BLACK, selected);
  }
  tft.print(ap->channel);
  if (ap->channel < 10){
    tft.print(" ");
  }
  tft.setCursor(39, fontPos);
  tft.println((char *)SSID.c_str());    
}

void clearLowerScreen(){
  tft.fillRect(0,tft.getCursorY(),tft.width(),tft.height() - tft.getCursorY(), CLR_BLACK);
}

void clearLine(uint8_t height=6){
  tft.fillRect(tft.getCursorX(), tft.getCursorY(), tft.width(), tft.getCursorY()+height, CLR_BLACK);
}

bool displayWiFiDetail(wifi_ap *ap, uint8_t input){
  static bool firstRun = true;
  tft.setCursor(0,10);
  uint16_t foreground = CLR_WHITE;
  uint16_t background= CLR_BLACK;
  static int baseRSSI;
  static uint32_t timer=0;
  static uint32_t charttimer=0;
  static sortOrder sort_order=SORTBYTIME;
  bool shouldUpdate=false;
  bool updateChart=false;
  if (firstRun == true){
    clearLowerScreen();
    /*  
     *  Start Passive Wifi Collection and update RSSI and client lists...
     */
     if (start_sniffer(ap->iBSSID, ap->channel)){
       Serial.println("Sniffer started");
     }
     baseRSSI = ap->RSSI;
     shouldUpdate=true;
     updateChart=true;
     timer=millis();
     charttimer=millis();
     firstRun=false;
  }
  if (millis() > timer + 500){
    baseRSSI=get_rssi();
    shouldUpdate=true;
    timer=millis();
  }
  if(millis() > charttimer + 1000){
    pushCount(tft.width());
    updateChart=true;
    charttimer=millis();
  }
  int RSSI = abs(baseRSSI);
  uint16_t currentX = tft.getCursorX();
  uint16_t currentY = tft.getCursorY();
  setRSSIColor(RSSI, false);
  //printRSSI(RSSI, tft.getCursorY());
  tft.setCursor(tft.width() - 34, currentY+4);
  tft.setTextSize(2);
  tft.print(-1 * RSSI);
  tft.setTextSize(1);
  setTextColor(foreground, background, false);
  //tft.print(" ");
  tft.setCursor(currentX, currentY);
  String SSID = ap-> SSID;
  if (SSID.length() == 0){
    tft.println("<hidden ssid>");
  } else {
    tft.println(ap->SSID);
  }
  tft.println(ap->BSSID);
  
  tft.print("Ch: ");
  tft.printf("%02d - ", ap->channel);
  switch(ap->encryption)
  {
    case 0:
      tft.println("Open");
      break;
    case 1:
      setTextColor(CLR_RED, background, false);
      tft.println("WEP");
      break;
    case 2:\
      setTextColor(CLR_BLUE, background, false);
      tft.println("WPA");
      break;   
    case 3:
      setTextColor(CLR_GREEN, background, false);
      tft.println("WPA2");
      break;
    case 4:
      setTextColor(CLR_YELLOW, background, false);
      tft.println("WPA/WPA2");
      break;
  }
  tft.drawLine(tft.getCursorX(), tft.getCursorY(), tft.width(), tft.getCursorY(), CLR_GRAY);
  tft.setCursor(0, tft.getCursorY()+1);;

  
  if(updateChart){
    //Draw chart
    uint32_t maxActivity = getMaxActivity();
    uint16_t pixels = tft.height() - tft.getCursorY();
    double scaler = (double)pixels / (double)maxActivity;  //numer of pixels to represent 1 packet
    uint16_t lines = getActivitySize();
    for (uint16_t x = 0; x < lines; x++){
      //drawLine
      uint32_t activity = getActivity(x);
      uint16_t screenHeight = tft.height();
      float chartLineHeight = (float)activity * scaler;
      tft.drawLine(x, tft.getCursorY(), x, tft.height() - (int)chartLineHeight, CLR_BLACK); // Dynamic rescale of chart
      tft.drawLine(x, tft.height() - (int)chartLineHeight, x, tft.height(), fadeToColor(CLR_BLUE, CLR_WHITE, pixels, chartLineHeight));
    }
    setTextColor(CLR_ORANGE, background, true);
    tft.print("pkts");
    setTextColor(CLR_ORANGE, background, false);
    tft.setCursor(tft.width() - 24, tft.getCursorY());
    tft.printf("%4d", maxActivity);
  }
  
  // Process Input
  if (input == 2){
    firstRun=true;
    bool ret = stop_sniffer();
    timer=0;
    sort_order=SORTBYTIME;
    return true;
  } else if (input == 1){
    switch(sort_order){
      case SORTBYTIME:
        sort_order=SORTBYRSSI;
        break;
      case SORTBYRSSI:
        sort_order=SORTBYPKT;
        break;
      case SORTBYPKT:
        sort_order=SORTBYTIME;
        break;
    }
  } else if (input == 4){
    switch(sort_order){
      case SORTBYPKT:
        sort_order=SORTBYRSSI;
        break;
      case SORTBYTIME:
        sort_order=SORTBYPKT;
        break;
      case SORTBYRSSI:
        sort_order=SORTBYTIME;
        break;
    }
  }
  return false;
}

bool displayWiFiSnoop(wifi_ap *ap, uint8_t input){
  static bool firstRun = true;
  tft.setCursor(0,10);
  uint16_t foreground = CLR_WHITE;
  uint16_t background= CLR_BLACK;
  static int baseRSSI;
  static uint32_t timer=0;
  static sortOrder sort_order=SORTBYTIME;
  bool shouldUpdate=false;
  if (firstRun == true){
    clearLowerScreen();
    /*  
     *  Start Passive Wifi Collection and update RSSI and client lists...
     */
     // Set channel
     // Set Callback
     if (start_sniffer(ap->iBSSID, ap->channel)){
       Serial.println("Sniffer Started");
     }
     shouldUpdate=true;
     timer=millis();
  }
  if (millis() > timer + 500){
    shouldUpdate=true;
    timer=millis();
  }
  
  if (firstRun == true){
    tft.drawLine(0, tft.getCursorY()+4,tft.width(), tft.getCursorY()+4, foreground);
    tft.setCursor(10, tft.getCursorY());
    tft.println("clients");
    firstRun=false;
  }
  /* Print list of clients in sorted order by RSSI */
  if(shouldUpdate){
    tft.setCursor(tft.width()-36, 10);
    switch(sort_order){
      case SORTBYTIME:
        tft.println("<secs>");
        break;
      case SORTBYRSSI:
        tft.println("<rssi>");
        break;
      case SORTBYPKT:
        tft.println("<pkts>");
        break;
    }
    //Get clients and print
    vector<wifi_client_t> clients=get_clients(sort_order);
    if (DEBUG) Serial.printf("Got list of %d clients\n", clients.size());
    for (int x=0; x < clients.size(); x++){
      tft.setCursor(tft.getCursorX()+2, tft.getCursorY());
      int cRSSI = abs(clients[x].rssi);
      setRSSIColor(cRSSI, false);
      printRSSI(cRSSI, tft.getCursorY());
      setTextColor(foreground, background, false);
      tft.print(" ");
      //char macstr[18] = { 0 };
      uint8_t mac[6];
      memcpy(mac, clients[x].mac, 6 * sizeof(uint8_t));
      uint32_t seconds = (millis() - clients[x].lastSeen)/1000;
      tft.printf("%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      switch(sort_order){
        case SORTBYTIME:
          tft.printf(" %ds", seconds);
          break;
        case SORTBYRSSI:
          tft.printf(" %d", clients[x].pcount);
          break;
        case SORTBYPKT:
          tft.printf(" %d", clients[x].pcount);
          break;
      }
      clearLine();
      tft.println();
    }
    //shouldUpdate=false;
  }
  if (input == 2){
    firstRun=true;
    bool ret = stop_sniffer();
    timer=0;
    sort_order=SORTBYTIME;
    return true;
  } else if (input == 1){
    switch(sort_order){
      case SORTBYTIME:
        sort_order=SORTBYRSSI;
        break;
      case SORTBYRSSI:
        sort_order=SORTBYPKT;
        break;
      case SORTBYPKT:
        sort_order=SORTBYTIME;
        break;
    }
  } else if (input == 4){
    switch(sort_order){
      case SORTBYPKT:
        sort_order=SORTBYRSSI;
        break;
      case SORTBYTIME:
        sort_order=SORTBYPKT;
        break;
      case SORTBYRSSI:
        sort_order=SORTBYTIME;
        break;
    }
  }
  return false;
}

bool displayBLEDetail(BLEAdvertisedDevice* device, uint8_t input){  
  static bool firstRun = true;
  tft.setCursor(0,10);
  uint16_t foreground = CLR_WHITE;
  uint16_t background= CLR_BLACK;
  static int baseRSSI;
  static uint32_t timer=0;
  static uint32_t charttimer=0;
  static uint16_t currentX = 0;
  static uint16_t currentY = 0;
  static uint16_t timerY = 0;
  static sortOrder sort_order=SORTBYTIME;
  static uint32_t lastSeen = millis();
  bool shouldUpdate=false;
  bool updateChart=false;  
  if (firstRun == true){
    ble_newData=true;
    clearLowerScreen();
    baseRSSI = device->getRSSI();
    shouldUpdate=true;
    updateChart=true;
    lastSeen = millis();
    timer=millis();
    charttimer=millis();
    firstRun=false;
  }
  ble_start_sniffer(device->getAddress());
  if (millis() > timer + 500){
    baseRSSI=ble_get_rssi();
    shouldUpdate=true;
    timer=millis();
  }
  if(millis() > charttimer + 1000){
    pushCount(tft.width());
    updateChart=true;
    charttimer=millis();
  }
  
  if (ble_newData){
    ble_newData=false;
    lastSeen = millis();
    int RSSI = abs(baseRSSI);
    currentX = tft.getCursorX();
    currentY = tft.getCursorY();
    setRSSIColor(RSSI, false);
    tft.setCursor(tft.width() - 34, currentY);
    tft.setTextSize(2);
    tft.println(-1 * RSSI);
    timerY = tft.getCursorY();
  
      
    tft.setTextSize(1);
    setTextColor(foreground, background, false);
    tft.setCursor(currentX, currentY);
    if (device->haveName()) {
      tft.println(str_truncate(device->getName().c_str(), 18, true));
    }else{
      tft.println("<Hidden>");
    }
  
    String macaddr = device->getAddress().toString().c_str();
    macaddr.toUpperCase();
    tft.print(str_truncate(&macaddr, 18, true));
    tft.println("");
  
    if (device->haveAppearance()) {
      uint16_t myData = device->getAppearance();
      setTextColor(CLR_ORANGE, CLR_BLACK);
      std::map<int, String>::iterator it;
      it = ble_icon.find(myData);
      if (it != ble_icon.end()) {
        tft.print("A| ");
        tft.print(it->second);
      }else{
        tft.printf("A| 0x%04X", myData);
      }
      tft.println("");
    }
    if (device->haveManufacturerData()) {
      setTextColor(CLR_BLUE, CLR_BLACK);
      string myData = device->getManufacturerData();
      uint8_t datalength = device->getManufacturerData().length();

      const char *payload = myData.data();
      int dev_id = -1;
      uint16_t mfr_id = 0;
      if (datalength < 4) {
        ESP_LOGE(LOG_TAG, "Length too small for Manufacturer Data"); // MFR is a 2 byte field
        if (DEBUG) Serial.printf("Length (%d) too small for ESP_BLE_AD_TYPE_128SERVICE_DATA", datalength);
      } else {
        mfr_id = *payload + (*(payload + 1) << 8);
      };
      if (mfr_id){
        uint8_t manuf[64];
        getMfrData(mfr_id, manuf);
        if (manuf[0] == 0 ){
          char *pHex = BLEUtils::buildHexData(nullptr, (uint8_t*)device->getManufacturerData().data(), device->getManufacturerData().length());
          tft.printf("M| 0x%s", pHex);
          tft.println("");
          free(pHex);
        }else{
          tft.println(str_truncate((const char*)manuf, 18, true));
          if (mfr_id == 0x0006){
            std::map<int, String>::const_iterator it;
            payload += 3;
            dev_id = *payload;
            it = ble_dev_type.find(dev_id);
            if (it != ble_dev_type.end()) {
              tft.println(it->second);
            } else {
              tft.printf("Unknown Dev (%d)", dev_id);
              tft.println();
            }
          }
        }
      }else{
        char *pHex = BLEUtils::buildHexData(nullptr, (uint8_t*)device->getManufacturerData().data(), device->getManufacturerData().length());
        tft.printf("M| 0x%s", pHex);
        tft.println("");
        free(pHex);
      }
    }
    if (device->haveServiceData()) {
      char *pHex = BLEUtils::buildHexData(nullptr, (uint8_t*)device->getServiceData().data(), device->getServiceData().length());
      //String myData = device->getServiceData().c_str();
      setTextColor(CLR_CYAN, CLR_BLACK);
      tft.printf("S| 0x%s", pHex);
      tft.println("");
      free(pHex);
    }
    if (device->haveServiceUUID()) {
      string myData = device->getServiceUUID().toString().c_str();
      setTextColor(CLR_MAGENTA, CLR_BLACK);
      tft.printf("U| ");
      tft.println(myData.substr(0,18).c_str());
      //std::string second = myData.substr(19).c_str());
      tft.printf("   ");
      tft.println(myData.substr(19).c_str());
    }
    tft.drawLine(tft.getCursorX(), tft.getCursorY(), tft.width(), tft.getCursorY(), CLR_GRAY);
    tft.setCursor(0, tft.getCursorY()+1);
    currentX = tft.getCursorX();
    currentY = tft.getCursorY();
  }
  if(updateChart){
    //With every chart update, we should also update the time since last seen:
    uint32_t tDelta = (millis() - lastSeen) / 1000;
    if (timerY != 0){ // Don't render this until we have our first RSSI printed.
      tft.setCursor(tft.width() - 30, timerY);
      if (tDelta >= 60){
        setTextColor(CLR_RED, CLR_BLACK);
      }else if(tDelta >= 30){
        setTextColor(CLR_ORANGE, CLR_BLACK);
      }else if(tDelta >= 10){
        setTextColor(CLR_YELLOW, CLR_BLACK);
      }else{
        setTextColor(CLR_WHITE, CLR_BLACK);
      }
      tft.printf("%4ds", tDelta);
    
      //Draw chart
      uint32_t maxActivity = getMaxActivity();
      uint16_t pixels = tft.height() - currentY;
      double scaler = (double)pixels / (double)maxActivity;  //numer of pixels to represent 1 packet
      uint16_t lines = getActivitySize();
      for (uint16_t x = 0; x < lines; x++){
        //drawLine
        uint32_t activity = getActivity(x);
        uint16_t screenHeight = tft.height();
        float chartLineHeight = (float)activity * scaler;
        tft.drawLine(x, currentY, x, tft.height() - (int)chartLineHeight, CLR_BLACK); // Dynamic rescale of chart
        tft.drawLine(x, tft.height() - (int)chartLineHeight, x, tft.height(), fadeToColor(CLR_BLUE, CLR_WHITE, pixels, chartLineHeight));
      }
      setTextColor(CLR_ORANGE, background, true);
      tft.setCursor(0, currentY);
      tft.print("rssi");
      setTextColor(CLR_ORANGE, background, false);
      tft.setCursor(tft.width() - 24, currentY);
      tft.printf("%4d", (100-maxActivity) * -1);
    }
  }
  // Process Input
  if (input == 2){
    firstRun=true;
    bool ret = ble_stop_sniffer();
    timer=0;
    timerY=0;
    sort_order=SORTBYTIME;
    return true;
  }
  return false;
}

void drawVersion(){
  tft.setCursor(0, tft.height()-lineHeight);
  setTextColor(CLR_DKGRAY, CLR_BLACK, false);
  tft.printf("v%s.%s", MAJVER, BUILDVER);
}
