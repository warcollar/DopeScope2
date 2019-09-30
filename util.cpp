#include "util.h"
#include <stdint.h>
#include <WString.h>
#include "esp_wifi.h"
#include "HardwareSerial.h"
#include<bits/stdc++.h> 
using namespace std;

std::deque<uint32_t> activity; 
uint32_t pCount;

vector<menu_item> m_MainMenu={
  {"WiFi Scan", 1},
  {"BLE Scan",  2},
  //{"BT Scan",   3},
  {"Settings",  3}
};

vector<menu_item> m_WiFiMenu {
  {"Details",      1},
  {"Snoop Mode",   2},
  {"Back to scan", 3},
  {"Main Menu",    4}
};

vector<menu_item> m_BLEMenu {
  {"Details",      1},
  {"Back to scan", 2},
  {"Main Menu",    3}
};

String str_truncate(const char *input, uint8_t str_len, bool pad){
  String NewString = input;
  int REM = str_len - NewString.length();
  if (NewString.length() > str_len ){
    NewString = NewString.substring(0,str_len);
  }
  if (pad){
    for ( int j=0; j < REM; j++){
      NewString=NewString + " ";
    }
  }
  return NewString;
}

String str_truncate(String* input, uint8_t str_len, bool pad){
  String NewString = *input;
  int REM = str_len - NewString.length();
  if (NewString.length() > str_len ){
    NewString = NewString.substring(0,str_len);
  }
  if (pad){
    for ( int j=0; j < REM; j++){
      NewString=NewString + " ";
    }
  }
  return NewString;
}

bool wifiScanStop(void) {
  if (DEBUG) Serial.println("Stopping Wifi Scan...");
  esp_wifi_scan_stop();
}

bool macs_match(uint8_t* mac1, uint8_t* mac2){
  for (int a=0; a<6; a++){
    if (mac1[a] != mac2[a]){
      return 0;
    }
  }
  return 1;
}

uint32_t getActivitySize(){
  return activity.size();
}

uint32_t getMaxActivity(){
  uint32_t maxActivity=0;
  for (int x=0; x<activity.size(); x++){
    if (activity[x] > maxActivity){
      maxActivity = activity[x];
    }
  }
  return maxActivity;
}

uint32_t getActivity(uint16_t index){
  return activity[index];
}


void pushCount(uint16_t maxSize){
  static uint32_t lastrun=millis();
  if (millis() - lastrun > 1000){
    lastrun=millis();
    while (activity.size() >= maxSize){
      activity.pop_front();
    }
    activity.push_back(pCount);
    pCount=0;
  }
}
