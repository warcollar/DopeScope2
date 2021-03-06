#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"

#include <stdio.h>
#include <string>
#include <cstddef>
#include <Wire.h>
#include "HardwareSerial.h"
#include "ble_util.h"
#include <algorithm>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEAddress.h>
#include "config.h"
#include <map>
#include "util.h"
#include "storage.h"
using namespace std;
#define SNAP_LEN 2324   // max len of each recieved packet
#ifdef __AVR__
 #include <avr/io.h>
 #include <avr/pgmspace.h>
#elif defined(ESP8266)
 #include <pgmspace.h>
#else
 #define PROGMEM
#endif

bool ble_update_lock=false;
BLEScan* pBLEScan=NULL;
std::string watch_addr;
BLEAdvertisedDevice BLE_DEVICES[25];

bool dbAvailable=false;

uint8_t* ble_bssid;
int ble_rssi=-99;
bool ble_scan_running = false;
bool ble_newData = false;
File dbFile;
uint8_t MAXLEN;
uint16_t TOTALM;

void MyAdvertisedDeviceCallbacks::onResult(BLEAdvertisedDevice device) {
  // Something like... if UUID = service UUID, then add a counter to the list and update the RSSI.
  BLEAddress addr = device.getAddress();
  if (watch_addr.compare(addr.toString()) == 0){
    ble_rssi = abs(device.getRSSI());
    pCount = 100-ble_rssi;
    ble_newData = true;
  }
};

void BLEInit() {
  if (DEBUG) Serial.print("Initializing BLE...");
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  if (DEBUG) Serial.println("done.");
}

void BLEDeinit() {
  if (DEBUG) Serial.print("Deinitializing BLE...");
  pBLEScan->stop();
  // When the 1.4 version of the arduino/esp32 code is released, this should work and can be reenabled
  //BLEDevice::deinit(false);
  if (DEBUG) Serial.println("done.");
}

static void sniffCompleteCB(BLEScanResults scanResults) {
  ble_scan_running=false;
}

bool ble_start_sniffer(BLEAddress addr){
  if(!ble_scan_running){
    watch_addr = addr.toString();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
    pBLEScan->setInterval(0x50);
    pBLEScan->setWindow(0x30);  // less or equal setInterval value
    pBLEScan->start(1, sniffCompleteCB);
    ble_scan_running=true;
    return true;
  }
  return false;
}

int ble_get_rssi(){
  return ble_rssi;
}

bool ble_stop_sniffer(){
    pBLEScan->clearResults();
    pBLEScan->stop();
    activity.clear();
    ble_scan_running=false;
    return true;
}

void initMfrDB(){
  // Init Filesystem
  if(!FSinit()){
    if (DEBUG) Serial.println("Storage Mount Failed");
    return;
  }
  if (DEBUG) Serial.println("...done");
  
  dbFile = openFile("/manufacturers.db", "r");
  if (!dbFile) {
    if (DEBUG) Serial.println("Failed to open db file");
    return;
  }

  size_t size = dbFile.size();
  if (size < 1024) {
    if (DEBUG) Serial.println("DB file size is too small");
    return;
  } else {
    if (DEBUG) {
      Serial.print("FileSize: ");
      Serial.println(size);
    } 
  }
  bool dbAvailable=false;
  MAXLEN=dbFile.read();
  dbFile.seek(2);
  uint8_t lower = dbFile.read();
  uint8_t upper = dbFile.read();
  TOTALM = (upper << 8) + lower;
  if (DEBUG) {
    Serial.print("MAXLEN: ");
    Serial.println(MAXLEN);
    Serial.print("Total Records: ");
    Serial.println(TOTALM);
  }
}

void getMfrData(uint16_t mfr, uint8_t* manuf){
  manuf[0] = 0;
  // If mfr > max, return
  if (mfr > TOTALM) {
    if (DEBUG) Serial.println("MFR out of range");
    return;
  }
  // Check database
  dbFile.seek((mfr+1) * MAXLEN);
  dbFile.read(manuf, (size_t) 24);
  manuf[MAXLEN] = 0;
  dbFile.seek(0);
}


std::map<int,String> ble_icon =
{
    {0x0000, "Unknown"},
    {0x0040, "Generic Phone"},
    {0x0080, "Generic Computer"},
    {0x00C0, "Generic Watch"},
    {0x00C1, "Sports Watch"},
    {0x0100, "Generic Clock"},
    {0x0140, "Generic Display"},
    {0x0180, "Generic Remote"},
    {0x01C0, "Generic Eyeglass"},
    {0x0200, "Generic Tag"},
    {0x0240, "Generic Keyring"},
    {0x0280, "Generic Media Player"},
    {0x02C0, "Generic Barcode Scanner"},
    {0x0300, "Generic Thermometer"},
    {0x0301, "Thermometer Ear"},
    {0x0340, "Generic Heart  Rate"},
    {0x0341, "Heart Rate Belt"},
    {0x0380, "Generic Blood Pressure"},
    {0x0381, "Blood Pressure Arm"},
    {0x0382, "Blood Pressure Wrist"},
    {0x03C0, "Generic Hid"},
    {0x03C1, "HID Keyboard"},
    {0x03C2, "HID Mouse"},
    {0x03C3, "HID Joystick"},
    {0x03C4, "HID Gamepad"},
    {0x03C5, "HID Digitizer Tablet"},
    {0x03C6, "HID Card Reader"},
    {0x03C7, "HID Digital Pen"},
    {0x03C8, "HID Barcode Scanner"},
    {0x0400, "Generic Glucose"},
    {0x0440, "Generic Walking"},
    {0x0441, "Walking in Square"},
    {0x0442, "Walking on Shoe"},
    {0x0443, "Walking On Hip"},
    {0x0480, "Generic Cycling"},
    {0x0481, "Cycling Computer"},
    {0x0482, "Cycling Speed"},
    {0x0483, "Cycling Cadence"},
    {0x0484, "Cycling Power"},
    {0x0486, "Cycling Speed Cadence"},
    {0x0C40, "Generic Pulse Oximeter"},
    {0x0C41, "Pulse Oximeter Fingertip"},
    {0x0C42, "Pulse Oximeter Wrist"},
    {0x0C80, "Generic Weight"},
    {0x1440, "Generic Outdoor Sports"},
    {0x1441, "Outdoor Sports Location"},
    {0x1442, "Outdoor Sports Location and Nav"},
    {0x1443, "Outdoor Sports Location Pod"},
    {0x1444, "Outdoor Sports Location Pod and Nav"},
};

const std::map<int,String> ble_dev_type = 
{
  {0x01, "Xbox One"},
  {0x06, "iPhone"},
  {0x07, "iPad"},
  {0x08, "Android"},
  {0x09, "Win10 Desktop"},
  {0x0B, "Win10 Phone"},
  {0x0C, "Linux Device"},
  {0x0D, "Windows IoT"},
  {0x0E, "Surface Hub"},
};
