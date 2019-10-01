#ifndef _WIFIUTIL_
#define _WIFIUTIL_

#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "util.h"
#include <vector>

typedef struct{
  uint8_t mac[6];
  int8_t rssi;
  uint32_t pcount;
  uint32_t lastSeen;
} wifi_client_t;

void wifiInit(uint8_t mode);
void wifiDeInit();
void addclient(uint8_t* mac, signed rssi);
void wifi_promiscuous_cb(void* buff, wifi_promiscuous_pkt_type_t type);
bool start_sniffer(uint8_t* lock_bssid, uint8_t channel);
bool stop_sniffer();
int get_rssi();
std::vector<wifi_client_t> get_clients(sortOrder sort=SORTBYTIME, uint8_t limit=20);

#endif
