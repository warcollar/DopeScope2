#ifndef _BLEUTIL_
#define _BLEUTIL_

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAddress.h>
#include <BLEAdvertisedDevice.h>

#include "util.h"

extern BLEScan* pBLEScan;
extern BLEAdvertisedDevice BLE_DEVICES[25];
extern std::map<int,String> ble_icon;
extern const std::map<int,String> ble_dev_type;
extern const std::map<int,String> manufacturers;
extern bool ble_newData;

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice device);
};

std::string getAppearanceString(uint16_t icon);
void BLEInit();
void BLEDeinit();
void initMfrDB();
uint8_t* getMfrData(uint16_t mfr);
void getMfrData(uint16_t mfr, uint8_t* manuf);
bool ble_start_sniffer(BLEAddress addr);
bool ble_stop_sniffer();
int ble_get_rssi();
bool StartBLEScan2();
#endif
