#ifndef _BTCLASSICH_
#define _BTCLASSICH_
/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/



/****************************************************************************
*
* This file is for Classic Bluetooth device and service discovery Demo.
*
****************************************************************************/

#include <stdint.h>
#include <string.h>
#include <vector>
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"

typedef enum {
    APP_GAP_STATE_IDLE = 0,
    APP_GAP_STATE_DEVICE_DISCOVERING,
    APP_GAP_STATE_DEVICE_DISCOVER_COMPLETE,
    APP_GAP_STATE_SERVICE_DISCOVERING,
    APP_GAP_STATE_SERVICE_DISCOVER_COMPLETE,
} app_gap_state_t;

typedef struct {
    bool dev_found;
    uint8_t bdname_len;
    uint8_t eir_len;
    int8_t rssi;
    uint32_t cod;
    uint8_t eir[ESP_BT_GAP_EIR_DATA_LEN];
    char bdname[ESP_BT_GAP_MAX_BDNAME_LEN + 1];
    esp_bd_addr_t bda;
    app_gap_state_t state;
} app_gap_cb_t;

typedef struct {
    esp_bd_addr_t bda;    
    int8_t rssi;
    uint32_t cod;
    uint8_t eir[ESP_BT_GAP_EIR_DATA_LEN];
    uint8_t eir_len;
    char bdname[ESP_BT_GAP_MAX_BDNAME_LEN + 1];
    uint8_t bdname_len;
} gap_device_t;

static app_gap_cb_t m_dev_info;
app_gap_state_t bt_getStatus(void);
static char *bda2str(esp_bd_addr_t bda, char *str, size_t size, bool uppercase=false);
void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
void bt_cancel_scan(void);
void bt_app_gap_start_up(uint8_t scan_time);
void bt_sort_devices();
uint32_t bt_get_found_devices();
uint8_t *bt_get_bda(uint32_t index);
int8_t bt_get_rssi(uint32_t index);
uint32_t bt_get_cod(uint32_t index);
gap_device_t bt_get_device(uint32_t index);
#endif
