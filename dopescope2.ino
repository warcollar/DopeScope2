/*
 *  WarCollar DopeScope 2.0
 */

#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include "src/btclassic/btclassic.h"
#include "ble_util.h"
#include "graphics.h"
#include "util.h"
#include "config.h"
#include "version.h"
#include <bits/stdc++.h> 

using namespace std; 


// MODE Control Variables
bool INTRO_COMPLETE=false;
bool MENU_HASRUN=false;
bool BLE_HASRUN=false;
bool BCL_HASRUN=false;
bool WIFI_HASRUN=false;
uint8_t BLE_LASTCOUNT=0;
uint8_t BCL_LASTCOUNT=0;
uint8_t BLE_COUNT=0;
bool BLE_CHANGE=false;
bool BLE_RUNNING=false;
bool BCL_RUNNING=false;



// BUTTON Counters
uint8_t CNT_BUTTON_A = 0;
uint8_t CNT_LAST_BUTTON_A = 0;
uint8_t CNT_BUTTON_B = 0;
uint8_t CNT_LAST_BUTTON_B = 0;
uint8_t CNT_BUTTON_C = 0;
uint8_t CNT_LAST_BUTTON_C = 0;

int scanTime = 5; //In seconds
int fontPos = 0;

int mode = 0; //1=WIFI,2=BLE,3=WEBSITE
int oldmode =0;  //Used to store the previous mode.  Really only used for webserver mode where we don't want to restart it on each loop
int lastRun=0;

// Setup Storage for Surveys
std::vector<wifi_ap> wifiDevices;
std::vector<BLEAdvertisedDevice> BLEDevices;
std::vector<gap_device_t> BCLDevices;

bool waiting=false;
uint8_t throb=0;
uint32_t lastThrob=millis();



void endThrobber(){
  clearThrobber();
  waiting=false;
}

void handleButtonA(){
  static unsigned long lastButtonMillis = 0;
  unsigned long buttonMillis = millis();
  if (buttonMillis - lastButtonMillis > 200) {
    CNT_BUTTON_A += 1;
  }
  lastButtonMillis = buttonMillis;
}

void handleButtonB(){
  static unsigned long lastButtonMillis = 0;
  unsigned long buttonMillis = millis();
  if (buttonMillis - lastButtonMillis > 200) {
    CNT_BUTTON_B += 1;
  }
  lastButtonMillis = buttonMillis;
}

void handleButtonC(){
  static unsigned long lastButtonMillis = 0;
  unsigned long buttonMillis = millis();
  if (buttonMillis - lastButtonMillis > 200) {
    CNT_BUTTON_C += 1;
  }
  lastButtonMillis = buttonMillis;
}

void setup()
{
    Serial.begin(115200);
    loadConfig();
    ledcSetup(0, 5000, 8);
    ledcAttachPin(BLPIN, 0);
    //Turn off display during init to prevent garbage from old memory.
    ledcWrite(0,0); 
    ScreenInit();
    delay(10);
    // Turn display backlight on
    ledcWrite(0,255); 
    if ((bool)configuration["SHOW_SPLASH"].value==true) {
      displaySplash();
    } else {
      INTRO_COMPLETE=true;
    }

    // Dumb little dance to setup and init the WiFi stack since those methods are private (hint!)
    // Set WiFi to station mode and disconnect from an AP if it was previously connected
    WiFi.mode(WIFI_STA);
    WiFi.mode(WIFI_MODE_NULL);
    WiFi.disconnect();
    wifiDeInit();
    
    pinMode(BTNPIN1, INPUT_PULLUP);
    pinMode(BTNPIN2, INPUT_PULLUP);
    pinMode(BTNPIN3, INPUT_PULLUP);
    if ((bool)configuration["INVERT_BUTTON"].value==true) {
      attachInterrupt(digitalPinToInterrupt(BTNPIN3), handleButtonA, FALLING);
      attachInterrupt(digitalPinToInterrupt(BTNPIN1), handleButtonC, FALLING);
    } else {
      attachInterrupt(digitalPinToInterrupt(BTNPIN1), handleButtonA, FALLING);
      attachInterrupt(digitalPinToInterrupt(BTNPIN3), handleButtonC, FALLING);
    }
    attachInterrupt(digitalPinToInterrupt(BTNPIN2), handleButtonB, FALLING);
    if (DEBUG) Serial.println("Setup complete");
    initMfrDB();
}

void clearState(int m) {
  if (DEBUG) Serial.println("Clearing State...");
  if (mode == m){
    return;
  }
  // Clear activity deque
  activity.clear();
  // Stop all processes
  wifiDeInit();
  WiFi.scanDelete();
  
  // Clear BLE Stack
  if (BLEDevice::getInitialized()) {
    BLEDeinit();
  }
  // Stop BCL Scans
  bt_cancel_scan();
  // Clear all control variables
  BLE_HASRUN=false;
  BLE_RUNNING=false;
  WIFI_HASRUN=false;
  MENU_HASRUN=false;
  BCL_HASRUN=false;
  BCL_RUNNING=false;
  // Reset all input counters
  CNT_LAST_BUTTON_A = CNT_BUTTON_A;
  CNT_LAST_BUTTON_B = CNT_BUTTON_B;
  CNT_LAST_BUTTON_C = CNT_BUTTON_C;
  // Clear all storage Vectors
  wifiDevices.clear();
  BCLDevices.clear();
  BLEDevices.clear();
  
  
  //Set mode state
  oldmode=mode;
  mode=m;
  // Clear display and kicj off throbber.
  clearScreen();
  waiting=true;
}

uint8_t hadInput(){
  return (CNT_BUTTON_A != CNT_LAST_BUTTON_A) << 2 & (CNT_BUTTON_B != CNT_LAST_BUTTON_B) << 1 & (CNT_BUTTON_C != CNT_LAST_BUTTON_C);
}


uint8_t checkInput(){
  uint8_t ret=0;
  if(CNT_BUTTON_A != CNT_LAST_BUTTON_A)
      {
        CNT_LAST_BUTTON_A = CNT_BUTTON_A;
        ret += 1;
      }
      if(CNT_BUTTON_B != CNT_LAST_BUTTON_B)
      {
        CNT_LAST_BUTTON_B = CNT_BUTTON_B;
        ret += (1 << 1);
      }
      if(CNT_BUTTON_C != CNT_LAST_BUTTON_C)
      {
        CNT_LAST_BUTTON_C = CNT_BUTTON_C;
        ret += (1 << 2);
      }
      return ret;
}

void loop()
{
  if (INTRO_COMPLETE) {
    if ((waiting) & (millis() - lastThrob > 100)){
      lastThrob=millis();
      throb=(throb + 1) % 8;
      drawThrobber(7-throb);
    }
    bool ret=true;
    switch(mode)
    {
      case 0:
        //Main Menu
        MainMenu();
      break;
      case 1:
        // Wifi Scan
        RunWifiScan();
      break;
      case 2:
        //BLE Scan
        RunBLEScan();
      break;
      /*case 3:
       * // BL Classic Scan
        RunBCLScan();
        if (hadInput()) clearState(0);
      break;
      case 4:*/
      case 3:
        // Settings Configuration
        waiting=false;
        ret = configMenu(checkInput());
      break;
    }
    if (!ret){
      if (DEBUG) Serial.println("No ret code");
      clearState(0);
    }
  } else {
    INTRO_COMPLETE=displaySplash();
  }
}

void MainMenu() {
  uint16_t centerx=tft.width()/2;
  uint16_t centery=tft.height()/2;
  if (!MENU_HASRUN){
    waiting=false;
    clearScreen();
    drawLogoEyes(centerx-38, centery-32);
    drawVersion();
  }
  
  static unsigned int menuoption = 0;
  static bool FIRSTTIME = false;
  uint8_t MENU_SZ = m_MainMenu.size();
  bool haschanged = false;
  if(CNT_BUTTON_A != CNT_LAST_BUTTON_A)
  {
    CNT_LAST_BUTTON_A = CNT_BUTTON_A;
    menuoption = (menuoption + 1) % MENU_SZ;
    haschanged = true;
  }
  if(CNT_BUTTON_B != CNT_LAST_BUTTON_B)
  {
    CNT_LAST_BUTTON_B = CNT_BUTTON_B;
    clearState(menuoption+1);
    return;
  }
  if(CNT_BUTTON_C != CNT_LAST_BUTTON_C)
  {
    CNT_LAST_BUTTON_C = CNT_BUTTON_C;
    menuoption = (MENU_SZ + menuoption - 1) % MENU_SZ;
    haschanged = true;
  }
  if (haschanged || (!MENU_HASRUN)){
    drawMenu(centerx-36, centery-14, ST77XX_WHITE, ST77XX_BLACK, true, "-Main Menu", &m_MainMenu, MENU_SZ, menuoption, !MENU_HASRUN);
  }
  MENU_HASRUN=true;
}

uint8_t SubMenu(char *name, std::vector<menu_item> *menuitems, bool firstrun=false){
  static unsigned int menuoption = 0;
  uint8_t selected = 0;
  static bool FIRSTTIME = true;
  uint16_t centerx=tft.width()/2;
  uint16_t centery=tft.height()/2;
  if (firstrun) {
    FIRSTTIME=true;
    menuoption=0;
  }
  uint8_t MENU_SZ = menuitems->size();
  bool haschanged = false;
  if(CNT_BUTTON_A != CNT_LAST_BUTTON_A)
  {
    CNT_LAST_BUTTON_A = CNT_BUTTON_A;
    menuoption = (menuoption + 1) % MENU_SZ;
    haschanged = true;
  }
  if(CNT_BUTTON_B != CNT_LAST_BUTTON_B)
  {
    CNT_LAST_BUTTON_B = CNT_BUTTON_B;
    selected = menuoption + 1;
  }
  if(CNT_BUTTON_C != CNT_LAST_BUTTON_C)
  {
    CNT_LAST_BUTTON_C = CNT_BUTTON_C;
    menuoption = (MENU_SZ + menuoption - 1) % MENU_SZ;
    haschanged = true;
  }
  if (haschanged || (FIRSTTIME)){
    drawMenu(centerx-37, centery-30, CLR_WHITE, CLR_BLACK, true, name, menuitems, MENU_SZ, menuoption, FIRSTTIME);
    FIRSTTIME=false;
  }
  return selected;
}


void RunWifiScan()
{
  int16_t scanResults = WiFi.scanComplete();
  static bool haschanged=false;
  static bool pause = false;
  static uint8_t index = 0;
  static uint8_t last_index = 0;
  static uint8_t first = 0;
  static uint8_t wifiMode=0;
  static bool wifi_menu_hasrun=false;
  int16_t wifiDeviceCount=wifiDevices.size();
  if (! WIFI_HASRUN){
    drawHeader(0);
  }
  switch(wifiMode){
    case 0: // Scanning
    {
      uint8_t myInput = checkInput();
      if (myInput) {
          if (myInput & 1){
            if (pause){
              if (index < (wifiDeviceCount-1) ) {
                last_index = index;
                index = (index + 1);
              }
            } else {
              pause=true;
              drawState(pause);
            }
            haschanged = true;        
          }
          if(myInput & 2)
          {
            if(pause){
              wifiMode=1;
            }else {
              pause=false;
              clearState(0);
            }
            return;
          }
          if(myInput & 4)
          {
            if (pause){
              if (index > 0){
                last_index = index;
                index = (index - 1);
              } else {
                pause=false;
                tft.setCursor(0,lineHeight + 2);
                drawWiFiRecord(&wifiDevices[0], (bool)configuration["WIFI_BSSID"].value, false, true);
                drawState(pause);
                return;
              }
              haschanged = true;
            } else {
              pause=true;
              drawState(pause);
            }
            haschanged = true;
          }
          //wifiScanStop();
          if (index < first) first = index;
          if (index > first + 7) first = index-7;
      } else {
        haschanged=false;
      }
    }
    break;
    case 1: // WiFi Menu
    {
      uint8_t ret = SubMenu( "-WiFi Manu", &m_WiFiMenu, !wifi_menu_hasrun);
      if (ret){
        wifiMode = ret + 1;
        wifi_menu_hasrun=false;
      } else {
        wifi_menu_hasrun=true;
      }
      return;
    }
    case 2:
     {
      // Detail Screen
      bool ret = displayWiFiDetail(&wifiDevices[index], checkInput());
      if (ret){
        wifiMode=1;
        haschanged=true;
        wifi_menu_hasrun=false;
        clearScreen();
        drawHeader(0);
        break;
      }
      return;
     }
    case 3:
      {
      // Snoop Mode
      bool ret = displayWiFiSnoop(&wifiDevices[index], checkInput());
      if (ret){
        wifiMode=1;
        haschanged=true;
        wifi_menu_hasrun=false;
        clearScreen();
        drawHeader(0);
        break;
      }
      return;
      }
    case 4:
      // Return to scanning
      wifiMode = 0;
      //index=0;
      haschanged=true;
      wifi_menu_hasrun=false;
      break;
    case 5:
      // Return to Main Menu
      wifiMode=0;
      index=0;
      pause=false;
      wifi_menu_hasrun=false;
      clearState(0);
      return;
  }
 
  // If we have new data from a scan, we should update our datastore  
  if ((!pause) && (scanResults>=0)){
    if (waiting){
      endThrobber();
    }
    wifiDevices.clear();
    //= new wifi_ap[scanResults];
    for (int i =0; i < scanResults; i++){
      wifi_ap newAP;
      // Eventually the maintainers will allow access here so we can get more fun data
      // Would be nice to get channel HT20/40 size and capabilities (BGN), etc
      //wifi_ap_record_t* it = reinterpret_cast<wifi_ap_record_t*>(WiFi.getScanInfoByIndex(i));
      newAP.SSID = WiFi.SSID(i);
      // Apple likes to use the Unicode "smart quote" in it's naming, so let's nuke the smart quote so all the apple fanboys don't throw a hissy fit
      // We could/should properly implent unicode, but, dude, ain't nobody got time for dat
      String str2 = "’";
      std::size_t found = newAP.SSID.indexOf(str2);
      if (found != -1){
        newAP.SSID.replace(str2,"'");
      }
      newAP.BSSID = WiFi.BSSIDstr(i);
      memcpy(newAP.iBSSID,WiFi.BSSID(i), 6 * sizeof(uint8_t));
      newAP.channel = WiFi.channel(i);
      newAP.RSSI = WiFi.RSSI(i);
      newAP.encryption = WiFi.encryptionType(i);
      
      wifiDevices.push_back(newAP);
    }
    wifiDeviceCount = scanResults;
    // We need to clear out the scan results and stop the scanning
    haschanged=true;
    WiFi.scanDelete();
  }
  // If we have updated either the datastore or userinput, we need to refresh our display...
  if (haschanged){
    drawState(pause);
    fontPos = 2;
    drawCounter(wifiDeviceCount);
    fontPos += lineHeight;
    tft.setCursor(0,fontPos);

    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    uint8_t count = wifiDeviceCount > 8 ? 8 : wifiDeviceCount;
    
    for (int i = 0; i < count; ++i) {
      int realNum = i + first;
      bool selected=false;
      if ((pause) && (realNum==index)){
        selected=true;
      }
      bool clear = false;
      if (realNum==last_index){
        clear=true;
      }
      drawWiFiRecord(&wifiDevices[realNum], (bool)configuration["WIFI_BSSID"].value, selected, clear);
    }
    clearLowerScreen();
    drawScroll(index, wifiDeviceCount-1);
  }
  // If we are not in a paused state and we don't have data in the store...let's get some
  if (!pause) {
    if (((scanResults < -1)||(scanResults == 0)) || ((scanResults == -1) && (! WIFI_HASRUN))){
      if (! WIFI_HASRUN){
        //WiFi.mode(WIFI_STA);
        //WiFi.disconnect();
        //WiFi.scanDelete();
        wifiInit(1);
        waiting=true;
      }
      if (DEBUG) Serial.println("Starting WiFi Scan!");
      WiFi.scanNetworks(true, (bool)configuration["WIFI_HIDDEN"].value, (bool)configuration["WIFI_PASSIVE"].value, (uint8_t)configuration["WIFI_DWELL"].value * 100);
      WIFI_HASRUN=true;
    }
  }
}

void StartBLEScan() {
  if (BLE_RUNNING){
    return;
  } else {
    pBLEScan->setActiveScan((bool)configuration["BLE_ACTIVE"].value);
    //pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->start(configuration["BLE_SCANTIME"].value, scanCompleteCB);
    BLE_RUNNING=true;
  }
}

bool BLESort(BLEAdvertisedDevice i, BLEAdvertisedDevice j) {
  return (i.getRSSI() > j.getRSSI());
}

static void scanCompleteCB(BLEScanResults scanResults) {
  if (BLE_RUNNING){
    if (waiting){
      endThrobber();
    }
    // Update data set
    BLEDevices.clear();
    uint8_t MAXDEV=25;
    if (scanResults.getCount() < MAXDEV ) MAXDEV=scanResults.getCount();
    for (int i=0; i<MAXDEV; i++) {     
      BLEDevices.push_back(scanResults.getDevice(i));
    }
    std::sort (BLEDevices.begin(), BLEDevices.end(), BLESort);
    pBLEScan->clearResults();
    // Set scan complete
    BLE_COUNT = scanResults.getCount();
    if (BLE_COUNT>0){
      if (waiting){
        endThrobber();
      }
    }
    drawCounter(BLE_COUNT);
    BLE_CHANGE=true;
    BLE_RUNNING=false;
  }
}

void RunBLEScan() {
  static bool haschanged=false;
  static bool pause = false;
  static uint8_t index = 0;
  static uint8_t last_index = 0;
  static uint8_t first = 0;
  static uint8_t bleMode=0;
  static bool ble_menu_hasrun=false;
  int16_t bleDeviceCount=BLEDevices.size();
  if (! BLE_HASRUN){
    drawHeader(1);
    BLEInit();
    //checkInput();
  }
  switch(bleMode){
    case 0:
    {
      uint8_t myInput = checkInput();
      if (myInput) {
        if (myInput & 1){
          if (pause){
            if (index < (BLEDevices.size()-1) ) {
              last_index = index;
              index = (index + 1);
            }
          } else {
              pause=true;
              BLE_RUNNING=false;
              drawState(pause);
          }
          haschanged = true;
        }
        if (myInput & 2){
          if (pause){
            bleMode=1;
          }else{
            pause=false;
            clearState(0);
          }
          return;
        }
        if (myInput & 4){
          if (pause){
            if (index > 0){
              last_index = index;
              index = (index - 1);
            } else {
              pause=false;
              tft.setCursor(0,lineHeight + 2);
              printBLERecord(&BLEDevices[0], false, true);
              drawState(pause);
              return;
            }
            haschanged = true;
          } else {
            pause=true;
            drawState(pause);
          }
          haschanged = true;
        }
        if (index < first) first = index;
        if (index > first + 7) first = index-7;
      } else {
        haschanged=false;
      }
    }
    break;
    case 1: // BLE Sub Menu
    {
      uint8_t ret = SubMenu( "-BLE Menu", &m_BLEMenu, !ble_menu_hasrun);
      if (ret){
        bleMode = ret + 1;
        ble_menu_hasrun=false;
      } else {
        ble_menu_hasrun=true;
      }
      return;
    }
    case 2:
     {
      // Detail Screen
      bool ret = displayBLEDetail(&BLEDevices[index], checkInput());
      if (ret){
        bleMode=0;
        haschanged=true;
        clearScreen();
        drawHeader(1);
        break;
      }
      return;
     }
    case 3:
      // Return to scanning
      bleMode = 0;
      //index=0;
      haschanged=true;
      ble_menu_hasrun=false;
      break;
    case 4:
      // Return to Main Menu
      bleMode=0;
      index=0;
      pause=false;
      ble_menu_hasrun=false;
      clearState(0);
      return;
  }
  // See if we have new data and account for it.
  if(BLE_CHANGE){
    haschanged=true;
    BLE_CHANGE=false;
  }
  if (haschanged){  
    drawState(pause);
    fontPos = 2;
    drawCounter(BLEDevices.size());
    fontPos += lineHeight;
    tft.setCursor(0,fontPos);
    uint8_t count = BLEDevices.size() > 8 ? 8 : BLEDevices.size();
    // Now print the results
    for (int i = 0; i < count; ++i) {
    //for (uint32_t i=0; i<BLEDevices.size(); i++) {
      int realNum = i + first;
      bool selected=false;
      if ((pause) && (realNum==index)){
        selected=true;
      }
      bool clear = false;
      if (realNum==last_index){
        clear=true;
      }
      printBLERecord(&BLEDevices[realNum], selected, clear);
    }
    clearLowerScreen();
    drawScroll(index, BLEDevices.size()-1);
  }
  
  // If we are not paused and we are idle, we should update the data by rescanning
  if ((!pause) && (!BLE_RUNNING)) {
    StartBLEScan();
  }
  BLE_HASRUN=true;
}

/*
bool BCLSort(gap_device_t i, gap_device_t j) {
  return (i.rssi > j.rssi);
}

void RunBCLScan() {
  static bool haschanged=false;
  static bool pause = false;
  static uint8_t index = 0;
  static uint8_t last_index = 0;
  static uint8_t first = 0;
  fontPos = 0;
  // Check for input
  // If we have had User Input, we need to process it.
  uint8_t myInput = checkInput();
  if (myInput) {
    if (myInput & 1){
      Serial.println("Processing 1...");
      if (pause){
        if (index < (BCLDevices.size()-1) ) {
          last_index = index;
          index = (index + 1);
        }
      }else {
          pause=true;
          drawState(pause);
      }
      haschanged = 1;
    }
    if (myInput & 2){
      pause=false;
      clearState(0);
      return;
    }
    if (myInput & 4){
      if (pause){
        if (index > 0){
          last_index = index;
          index = (index - 1);
        } else {
          pause=false;
          tft.setCursor(0,lineHeight + 2);
          printBCLRecord(&BCLDevices[0], false, true);
          drawState(pause);
          return;
        }
        haschanged = true;
      } else {
        pause=true;
        drawState(pause);
      }
      haschanged = true;
    }
    //wifiScanStop();
    if (index < first) first = index;
    if (index > first + 7) first = index-7;
  } else {
    haschanged=false;
  }
  // If idle, check for new scan data
  if ((!pause) && (bt_getStatus() == APP_GAP_STATE_IDLE )){
    if(waiting){
      endThrobber();
    }
    waiting=false;
    uint32_t n = bt_get_found_devices();
    if (n>0){
      // Clear the vector
      BCLDevices.clear();
      // Push objects into the vector
      for (uint32_t i=0; i < n; i++){
        BCLDevices.push_back(bt_get_device(i));
      }
      // Sort the vector by RSSI:
      std::sort (BCLDevices.begin(), BCLDevices.end(), BCLSort);
      haschanged=true;
    }
  }
  if (haschanged){
    Serial.printf("Updating BCL display.  %d items in store\n", BCLDevices.size());
    drawState(pause);
    fontPos = 2;
    drawCounter(BCLDevices.size());
    fontPos += lineHeight;
    tft.setCursor(0,fontPos);
    
    // Now print the results
    for (uint32_t i=0; i<BCLDevices.size(); i++) {
      int realNum = i + first;
      bool selected=false;
      if ((pause) && (realNum==index)){
        selected=true;
      }
      bool clear = false;
      if (realNum==last_index){
        clear=true;
      }
      printBCLRecord(&BCLDevices[i], selected, clear);
    }
    clearLowerScreen();
    drawScroll(index, BCLDevices.size()-1);
  }
  // If we are not paused and we are idle, we should update the data by rescanning
  if ((!pause) && (bt_getStatus() == APP_GAP_STATE_IDLE )) {
    bt_app_gap_start_up(configuration["BCL_SCANTIME"].value);
  }
  // Draw header if we haven't run
  if (!BCL_HASRUN){
    drawHeader(2);
    Serial.print("Startling BtCL Scan...");
    waiting=true;
  }
  BCL_HASRUN=true;
  BCL_RUNNING=true;
}*/
