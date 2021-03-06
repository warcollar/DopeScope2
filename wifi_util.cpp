#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"

#include <WiFi.h>
#include <WiFiGeneric.h>
#include <stdio.h>
#include <string>
#include <cstddef>
#include <Wire.h>
#include "HardwareSerial.h"
#include "wifi_util.h"
#include <vector>
#include <deque>
#include <algorithm>
#include "util.h"

#define SNAP_LEN 2324   // max len of each recieved packet
static bool isInit = false;
static wifi_country_t wifi_country = {
  cc: "JP",
  schan:1,
  nchan:14,
  max_tx_power: 20,
  policy:WIFI_COUNTRY_POLICY_MANUAL
};

std::vector<wifi_client_t> clients;
bool update_lock=false;

sortOrder sort_method=SORTBYTIME;

void wifiInit(uint8_t mode) {
  wifi_mode_t currMode;
  esp_wifi_get_mode(&currMode);
  // If we are already in the correct mode, just return
  //if (currMode == mode) return;
  //wifiDeInit();
  if (DEBUG) Serial.print("Setting WiFi state for ");

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
  ESP_ERROR_CHECK( esp_wifi_set_country(&wifi_country) );
  ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
  switch (mode)
  {
    case 0:
      if (DEBUG) Serial.print("sniffing...");
      // NULL mode for promiscuous sniffing
      ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
      break;
    case 1:
      if (DEBUG) Serial.print("scanning...");
      // STATION mode for active scanning
      ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
      break;
    
  }
  delay(1);
  ESP_ERROR_CHECK( esp_wifi_start() );
  if (DEBUG) Serial.println("done");
}

void wifiDeInit(){
  Serial.println("DeinitWiFi");
  WiFi.scanDelete();
  //WiFi.mode(0);
  while (WiFi.scanComplete() == -1){
    delay(1);
  }
  esp_wifi_stop();
}

bool sortClients(wifi_client_t client1, wifi_client_t client2){
  switch (sort_method)
  {
    case SORTBYTIME:
      return client1.lastSeen > client2.lastSeen;
    case SORTBYRSSI:
      return client1.rssi > client2.rssi;
    case SORTBYPKT:
      return client1.pcount > client2.pcount;
  }
}

void addclient(uint8_t* mac, signed rssi){
  //if mac is already in list, update the RSSI
  bool isNew=true;
  for (int b; b<clients.size(); b++){
    if (macs_match(mac, clients[b].mac)){
      //Update lastseen and RSSI
      clients[b].lastSeen=millis();
      clients[b].pcount+=1;
      clients[b].rssi = rssi;
      isNew=false;
    }
  }
  if (isNew){
      wifi_client_t new_client;
      memcpy(new_client.mac, mac, 6 * sizeof(uint8_t));
      new_client.rssi = rssi;
      new_client.lastSeen=millis();
      new_client.pcount=1;
      while (update_lock == true){
        //Do something to wait for semiphore to unlock
      }
      update_lock=true;
      clients.push_back(new_client);
      update_lock=false;
  }
  sort_method=SORTBYTIME;
  std::sort(clients.begin(), clients.end(), sortClients);

  if (clients.size() > 20){
    clients.resize(20);
  }
  pCount += 1;
  return;
}

uint8_t* bssid;
int ap_rssi=-99;

void wifi_promiscuous_cb(void* buff, wifi_promiscuous_pkt_type_t type){
  wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buff;
  wifi_pkt_rx_ctrl_t radiotap = (wifi_pkt_rx_ctrl_t)pkt->rx_ctrl;
  if (radiotap.sig_len > SNAP_LEN) return; // too big?
  signed RSSI=radiotap.rssi;
  uint8_t ta[6];
  uint8_t ra[6];
  // Get TA
  uint32_t addroff=10;
  for (int x=0; x<6; x++){
    ta[x] = pkt->payload[x+addroff];
  }
  // Get RA
  addroff=4;
  for (int x=0; x<6; x++){
    ra[x] = pkt->payload[x+addroff];
  }
  if (!(macs_match(ta, bssid) || macs_match(ra, bssid))){
    // Packet on channel from device on different AP
    return;
  }

  if (macs_match(ta, bssid)){
    ap_rssi = RSSI;
    /* We need to filter for broadcast and multicast addresses (ff:ff:ff:ff:ff:ff, 01:00:5e, 01:80, 33:33, etc), 
       which are all defined as having the least significant bit set to "1"
    */
    if (! ra[0] & 1){
      addclient(ra, -99);
    }
  }else{
    addclient(ta, RSSI);
  }
}

bool start_sniffer(uint8_t* mac, uint8_t channel){
  if(DEBUG) Serial.println("Staring sniffer...");
  bssid = mac;
  wifiInit(0);
  //WiFi.mode(WIFI_STA);
  //WiFi.disconnect();
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous_rx_cb(&wifi_promiscuous_cb);
  esp_wifi_set_promiscuous(true);
  return true;
}

int get_rssi(){
  return ap_rssi;
}

std::vector<wifi_client_t> get_clients(sortOrder sort, uint8_t limit){
  while (update_lock == true){
    //Do something to wait for semiphore to unlock
  }
  sort_method=sort;
  update_lock=true;
  std::vector<wifi_client_t> clientList = clients;
  update_lock=false;
  std::sort(clientList.begin(), clientList.end(), sortClients);
  if (clientList.size() > limit){
    clientList.resize(limit);
  }
  return clientList;
}

bool stop_sniffer(){
  esp_wifi_set_promiscuous(false);
  clients.clear();
  activity.clear();
  return true;
}
