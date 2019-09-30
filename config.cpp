#include "FS.h"
#include "SPIFFS.h"
#include "FFat.h"
#include <ArduinoJson.h>
#include "graphics.h"
#include "util.h"
#include "config.h"

/* 
 *  Configuration Variables 
*/


/* General Settings */
bool SHOW_SPLASH=true;
std::map<String, config_item> configuration={
  {"SHOW_SPLASH", {"Enable Boot Splash", true, TYPE_BOOL, 0, 0}},
  {"INVERT_BUTTON", {"Invert Up/Down Btns", false, TYPE_BOOL, 0, 0}},
  // WIFI CONFIGURATION
  {"WIFI_HIDDEN", {"Show hidden SSID", true, TYPE_BOOL, 0, 0}},
  {"WIFI_BSSID", {"Show BSSID not ESSID", false, TYPE_BOOL, 0, 0}},
  {"WIFI_PASSIVE", {"Use Passive Scanning", false, TYPE_BOOL, 0, 0}},
  {"WIFI_DWELL", {"Time to dwell in 100ms", 3, TYPE_INT, 1, 5}},
  // BLE Config
  {"BLE_ACTIVE", {"Type of BLE Scanning", true, TYPE_BOOL, 0, 0}},
  {"BLE_SCANTIME", {"Time to scan in seconds", 5, TYPE_INT, 1, 255}},
  // Bluetooth Classic Configs
  {"BCL_SCANTIME", {"Time to scan in 1.28 secs", 3, TYPE_INT, 1, 255}},
};

String config_keys[9] = {"SHOW_SPLASH", "INVERT_BUTTON", "WIFI_HIDDEN", "WIFI_BSSID", "WIFI_PASSIVE", "WIFI_DWELL", "BLE_ACTIVE", "BLE_SCANTIME", "BCL_SCANTIME"};

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
      return FFat.begin(true);
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

bool saveConfig() {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["SHOW_SPLASH"]  = (bool)configuration["SHOW_SPLASH"].value;
  json["INVERT_BUTTON"]  = (bool)configuration["INVERT_BUTTON"].value;
  json["WIFI_HIDDEN"]  = (bool)configuration["WIFI_HIDDEN"].value;
  json["WIFI_BSSID"]   = (bool)configuration["WIFI_BSSID"].value;
  json["WIFI_PASSIVE"] = (bool)configuration["WIFI_PASSIVE"].value;
  json["WIFI_DWELL"]   = configuration["WIFI_DWELL"].value;
  json["BLE_ACTIVE"]   = (bool)configuration["BLE_ACTIVE"].value;
  json["BLE_SCANTIME"] = configuration["BLE_SCANTIME"].value;
  json["BCL_SCANTIME"] = configuration["BCL_SCANTIME"].value;

  File configFile = openFile("/config.json", FILE_WRITE);
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  json.printTo(configFile);
  return true;
  
}

bool loadConfig() {
  //configuration["SHOW_SPLASH"]= config_item();
  if(!FSinit()){
    Serial.println("Configuration Mount Failed");
    return false;
  }
  Serial.println("...done");
  
  File configFile = openFile("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    return false;
  }

  std::unique_ptr<char[]> buf(new char[size]);

  configFile.readBytes(buf.get(), size);

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  if (!json.success()) {
    Serial.println("Failed to parse config file");
    return false;
  }
  if (json.containsKey("SHOW_SPLASH")) {
    configuration["SHOW_SPLASH"].value = (bool)json["SHOW_SPLASH"];
  }
  if (json.containsKey("INVERT_BUTTON")) {
    configuration["INVERT_BUTTON"].value = (bool)json["INVERT_BUTTON"];
  }
  if (json.containsKey("WIFI_HIDDEN")) {
    configuration["WIFI_HIDDEN"].value = (bool)json["WIFI_HIDDEN"];
  }
  if (json.containsKey("WIFI_BSSID")) {
    configuration["WIFI_BSSID"].value = (bool)json["WIFI_BSSID"];
  }
  if (json.containsKey("WIFI_PASSIVE")) {
    configuration["WIFI_PASSIVE"].value = (bool)json["WIFI_PASSIVE"];
  }
  if (json.containsKey("WIFI_DWELL")) {
    configuration["WIFI_DWELL"].value = json["WIFI_DWELL"];
  }
  if (json.containsKey("BLE_ACTIVE")) {
    configuration["BLE_ACTIVE"].value = (bool)json["BLE_ACTIVE"];
  }
  if (json.containsKey("BLE_SCANTIME")) {
    configuration["BLE_SCANTIME"].value = json["BLE_SCANTIME"];
  }
  if (json.containsKey("BCL_SCANTIME")) {
    configuration["BCL_SCANTIME"].value = json["BCL_SCANTIME"];
  }
  
  if (DEBUG) Serial.print("Loaded SHOW_SPLASH:");
  if (DEBUG) Serial.println(configuration["SHOW_SPLASH"].value);
  return true;
}
void printConfigEntry(String name, config_item config, bool selected, bool editing, bool clear){
  uint16_t fontPos = tft.getCursorY();
  uint16_t foreground = CLR_DKGRAY;
  uint16_t background= CLR_BLACK;
  if (selected){
    if (editing){
      foreground=CLR_YELLOW;
    }else{
      foreground=CLR_WHITE;
    }
    drawGradient(0, fontPos ,tft.width()-4, 8, SELECTED_COLORS);
  } else if(clear){
    tft.fillRect(0,tft.getCursorY(),tft.width()-4,8, background);
  }

  setTextColor(foreground, background, selected);
  tft.print(name);
  tft.setCursor(tft.width()-(7*7),tft.getCursorY());
  switch(config.type){
    case TYPE_INT:
      tft.printf("< %3d >", config.value);
    break;
    case TYPE_BOOL:
      tft.printf("<%s>", config.value ? " true": "false");
    break;
  }
  tft.println();
}

bool drawButton(uint16_t x, uint16_t y, uint16_t width, uint16_t height, String label, uint16_t btnColor, uint16_t hltColor, bool selected) {
  if (selected){
    tft.fillRoundRect(x, y, width, height, 3, btnColor);
    setTextColor(hltColor, btnColor);
  } else {
    tft.fillRoundRect(x, y, width, height, 3, CLR_BLACK);
    tft.drawRoundRect(x, y, width, height, 3, btnColor);
    setTextColor(btnColor, CLR_BLACK);
    
  }
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(label, x, y, &x1, &y1, &w, &h);
  uint8_t hoff = (height - h) / 2;
  uint8_t woff = (width - w) / 2;
  tft.setCursor(x + woff, y+hoff);
  tft.print(label);
}

void drawHelper(String help){
  tft.fillRect(0, tft.height() - 21, tft.width()-4, 7, CLR_BLACK);
  tft.setCursor(1, tft.height() - 21);
  setTextColor(CLR_CYAN, CLR_BLACK);
  tft.print(help);
}

bool configMenu(uint8_t myInput){
  static bool haschanged=false;
  static bool CFG_HASRUN=false;
  static uint8_t index = 0;
  static uint8_t last_index = 0;
  static uint8_t first = 0;
  static int16_t editidx=-1;
  static config_item * current_item;
  static uint8_t pageSize = (tft.height() - 40) / lineHeight;
  static uint8_t buttonSelect=0;
  String helper;
  // Check for input
  // If we have had User Input, we need to process it.
  if (myInput) {
    if (DEBUG) Serial.println(myInput);
    if(editidx > -1){
      int val=0;
      if (myInput & 1){
        val=1;
      }
      if (myInput & 2){
        editidx = -1;
      }
      if (myInput & 4){
        val=-1;
      }
      if (val != 0){
        if (DEBUG) Serial.printf("adjusting value by %d\n", val);
        switch (current_item->type){
          case TYPE_BOOL:
            if (DEBUG) Serial.println("Adjusting boolean");
            current_item->value= !current_item->value;
            //(current_item->value==true) ? false : true;
            break;
          case TYPE_INT:
            current_item->value+= val;
            if (current_item->value > current_item->max) current_item->value = current_item->max;
            if (current_item->value < current_item->min) current_item->value = current_item->min;
        }
      }
      haschanged=true;
    } else {
      if (myInput & 1){
        if (index < (configuration.size()-1) ) {
          last_index = index;
          index = (index + 1);
        }else{
          //Move to button
          last_index = index;
          buttonSelect +=1;
          if (buttonSelect > 2) buttonSelect = 2;
          if (buttonSelect == 1){
            helper="Save config to flash";
          } else {
            helper="Return to Main Menu";
          }
        }
        haschanged = 1;
      }
      if (myInput & 2){
        if (buttonSelect > 0){
          switch (buttonSelect){
            case 1:
              if (DEBUG) Serial.println("Saving config...");
              saveConfig();
              helper="Save Successful!";
              break;
            case 2:
              CFG_HASRUN=false;
              index = 0;
              last_index = 0;
              first = 0;
              buttonSelect=0;
              return false;
              break;
          }
        }else{
          editidx = index;
        }
        haschanged=true;
      }
      if (myInput & 4){
        if (buttonSelect > 0){
          buttonSelect-=1;
          if (buttonSelect == 1) helper="Save config to flash";
        } else if (index > 0){
          last_index = index;
          index = (index - 1);
        }
        haschanged = true;
      }
      if (index < first) first = index;
      if (index > first + pageSize) first = index-pageSize;
    }
  } else {
    haschanged=false;
  }    
  // Draw header if we haven't run
  if (!CFG_HASRUN){
    clearThrobber();
    drawHeader(3);
  }
  if ((!CFG_HASRUN) || (haschanged)){
    //Draw Buttons
    uint16_t ba_width= tft.width() - 8;
    uint8_t bwidth = ba_width / 2;
    uint8_t bheight = 12;
    uint16_t ba_height = tft.height() - bheight;
    drawButton(1, ba_height, bwidth, 12, "Save", CLR_ORANGE, CLR_WHITE, (bool)(buttonSelect==1));
    drawButton(bwidth + 2, ba_height, bwidth, 12, "Return", CLR_ORANGE, CLR_WHITE, (bool)(buttonSelect==2));
  }
  
  if ((haschanged) || (! CFG_HASRUN)){
    tft.setCursor(0,lineHeight + 2);
    // Now print the results
    uint32_t i=0;
    bool editing=false;
    for (std::map<String, config_item>::iterator it=configuration.begin(); it!=configuration.end(); it++){
      if (i >= first){
        //int realNum = i + first;
        bool selected=false;
        if ((i==index) && (buttonSelect==0)){
          selected=true;
          helper=it->second.info;
        }
        bool clear = false;
        if (i==last_index){
          clear=true;
        }
        if (i == editidx) editing=true;
        printConfigEntry(it->first, it->second, selected, editing, clear);
        if (selected) current_item= &it->second;
      }
      if (i >= (pageSize + first)){
        break;
      }
      i+=1;
    }
    drawHelper(helper);
    drawScroll(index, configuration.size()-1);
  }

  CFG_HASRUN=true;
  return true;
}
