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
#include "HardwareSerial.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_spp_api.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"

#include "btclassic.h"
#include <vector>

//static app_gap_cb_t m_dev_info;

app_gap_state_t BTCisStarted = APP_GAP_STATE_IDLE;
std::vector<gap_device_t> m_vectorFoundDevices;

app_gap_state_t bt_getStatus(void) {
  return BTCisStarted;
}



static char *bda2str(esp_bd_addr_t bda, char *str, size_t size, bool uppercase)
{
    if (bda == NULL || str == NULL || size < 18) {
        return NULL;
    }
    
    uint8_t *p = bda;
    if(uppercase){
    sprintf(str, "%02X:%02X:%02X:%02X:%02X:%02X",
            p[0], p[1], p[2], p[3], p[4], p[5]); 
    }else{
    sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
            p[0], p[1], p[2], p[3], p[4], p[5]);
    }
    return str;
}

static char *uuid2str(esp_bt_uuid_t *uuid, char *str, size_t size)
{
    if (uuid == NULL || str == NULL) {
        return NULL;
    }

    if (uuid->len == 2 && size >= 5) {
        sprintf(str, "%04x", uuid->uuid.uuid16);
    } else if (uuid->len == 4 && size >= 9) {
        sprintf(str, "%08x", uuid->uuid.uuid32);
    } else if (uuid->len == 16 && size >= 37) {
        uint8_t *p = uuid->uuid.uuid128;
        sprintf(str, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                p[15], p[14], p[13], p[12], p[11], p[10], p[9], p[8],
                p[7], p[6], p[5], p[4], p[3], p[2], p[1], p[0]);
    } else {
        return NULL;
    }

    return str;
}

static bool bt_get_name_from_eir(uint8_t *eir, char *bdname, uint8_t *bdname_len)
{
    uint8_t *rmt_bdname = NULL;
    uint8_t rmt_bdname_len = 0;
    Serial.print("Looking for name...");
    if (!eir) {
      Serial.println("No EIR");
        return false;
    }

    rmt_bdname = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME, &rmt_bdname_len);
    if (!rmt_bdname) {
      Serial.print("looking for shortname...");
        rmt_bdname = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME, &rmt_bdname_len);
    }

    if (rmt_bdname) {
        if (rmt_bdname_len > ESP_BT_GAP_MAX_BDNAME_LEN) {
            rmt_bdname_len = ESP_BT_GAP_MAX_BDNAME_LEN;
        }

        if (bdname) {
            memcpy(bdname, rmt_bdname, rmt_bdname_len);
            bdname[rmt_bdname_len] = '\0';
            Serial.printf("got: %s ", bdname);
        }
        if (bdname_len) {
            Serial.println(rmt_bdname_len);
            *bdname_len = rmt_bdname_len;
        }
        return true;
    }
    Serial.println("struck out");
    return false;
}

void bt_sort_devices(){
  // Sort the results
  int n = m_vectorFoundDevices.size();
  for(int i=0;i<n;i++)
  {
      for(int j=0;j<n-1;j++)
      {
          if(m_vectorFoundDevices[j].rssi < m_vectorFoundDevices[j+1].rssi)
          {
              gap_device_t temp = m_vectorFoundDevices[j];
              m_vectorFoundDevices[j] = m_vectorFoundDevices[j+1];
              m_vectorFoundDevices[j+1] = temp;
          }
      }
  }
}

void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    app_gap_cb_t *p_dev = &m_dev_info;
    char bda_str[18];
    char uuid_str[37];

    switch (event) {
    case ESP_BT_GAP_DISC_RES_EVT: {
        // Callback for devices discovered during generic GAP discovery
        bool found = false;
        char bda_str[18];
        
        // See if we already have that device
        for (int n=0; n < m_vectorFoundDevices.size(); n++){
          int z=memcmp ( m_vectorFoundDevices[n].bda, param->disc_res.bda, 6 );
          if ( z==0 ) {
            found = true;
            break;
          }
        }
        if (!found){
          gap_device_t new_device;
          esp_bt_gap_dev_prop_t *p;
          memcpy(new_device.bda, param->disc_res.bda, ESP_BD_ADDR_LEN);
          Serial.printf("Device found: %s\n", bda2str(param->disc_res.bda, bda_str, 18));
          for (int i = 0; i < param->disc_res.num_prop; i++) {
            p = param->disc_res.prop + i;
            switch (p->type) {
            case ESP_BT_GAP_DEV_PROP_COD:
                new_device.cod = *(uint32_t *)(p->val);
                break;
            case ESP_BT_GAP_DEV_PROP_RSSI:
                new_device.rssi = *(int8_t *)(p->val);
                break;
            case ESP_BT_GAP_DEV_PROP_BDNAME: {
                uint8_t len = (p->len > ESP_BT_GAP_MAX_BDNAME_LEN) ? ESP_BT_GAP_MAX_BDNAME_LEN : (uint8_t)p->len;
                memcpy(new_device.bdname, (uint8_t *)(p->val), len);
                new_device.bdname[len] = '\0';
                new_device.bdname_len = len;
                break;
            }
            case ESP_BT_GAP_DEV_PROP_EIR: {
                memcpy(new_device.eir, (uint8_t *)(p->val), p->len);
                new_device.eir_len = p->len;
                bt_get_name_from_eir(new_device.eir, new_device.bdname, &new_device.bdname_len);
                break;
            }
            default:
                break;
            }
        }
        m_vectorFoundDevices.push_back(new_device);
        }
        break;
    }
    
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT: {
        if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
            Serial.printf("Device discovery stopped.\n");
            /*if ( (p_dev->state == APP_GAP_STATE_DEVICE_DISCOVER_COMPLETE ||
                    p_dev->state == APP_GAP_STATE_DEVICE_DISCOVERING)
                    && p_dev->dev_found) {
                p_dev->state = APP_GAP_STATE_SERVICE_DISCOVERING;
                Serial.printf("Discover services ...\n");
                // Try to get info on services available
                esp_bt_gap_get_remote_services(p_dev->bda);
            }*/
            BTCisStarted = APP_GAP_STATE_IDLE;
        } else if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STARTED) {
            Serial.printf("Discovery started.\n");
            p_dev->state = APP_GAP_STATE_DEVICE_DISCOVERING;
            BTCisStarted = APP_GAP_STATE_DEVICE_DISCOVERING;
        }
        break;
    }
    
    case ESP_BT_GAP_RMT_SRVCS_EVT: {
        if (memcmp(param->rmt_srvcs.bda, p_dev->bda, ESP_BD_ADDR_LEN) == 0 &&
                p_dev->state == APP_GAP_STATE_SERVICE_DISCOVERING) {
            p_dev->state = APP_GAP_STATE_SERVICE_DISCOVER_COMPLETE;
            if (param->rmt_srvcs.stat == ESP_BT_STATUS_SUCCESS) {
                Serial.printf("Services for device %s found\n",  bda2str(p_dev->bda, bda_str, 18));
                for (int i = 0; i < param->rmt_srvcs.num_uuids; i++) {
                    esp_bt_uuid_t *u = param->rmt_srvcs.uuid_list + i;
                    Serial.printf("--%s\n", uuid2str(u, uuid_str, 37));
                    // ESP_LOGI(GAP_TAG, "--%d", u->len);
                }
            } else {
                Serial.printf("Services for device %s not found\n",  bda2str(p_dev->bda, bda_str, 18));
            }
        }
        break;
    }
    case ESP_BT_GAP_RMT_SRVC_REC_EVT:
    default: {
        Serial.printf("event: %d\n", event);
        break;
    }
    }
    return;
}

void bt_cancel_scan() {
  esp_bt_gap_cancel_discovery();
}

uint32_t bt_get_found_devices() {
  return m_vectorFoundDevices.size();
}

uint8_t *bt_get_bda(uint32_t index) {
  return m_vectorFoundDevices[index].bda;
}
int8_t bt_get_rssi(uint32_t index){
  return m_vectorFoundDevices[index].rssi;
}
uint32_t bt_get_cod(uint32_t index){
  return m_vectorFoundDevices[index].cod;
}
gap_device_t bt_get_device(uint32_t index){
  return m_vectorFoundDevices[index];
}

void bt_app_gap_start_up(uint8_t scan_time)
{
  if (! BTCisStarted){
    char *dev_name = "WC_SCANNER";
    esp_bt_dev_set_device_name(dev_name);

    /* set discoverable and connectable mode, wait to be connected */
    esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_NONE);

    /* register GAP callback function */
    esp_bt_gap_register_callback(bt_app_gap_cb);

    /* inititialize device information and status */
    app_gap_cb_t *p_dev = &m_dev_info;
    memset(p_dev, 0, sizeof(app_gap_cb_t));
    m_vectorFoundDevices.clear();
    /* start to discover nearby Bluetooth devices */
    p_dev->state = APP_GAP_STATE_DEVICE_DISCOVERING;
    esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, scan_time, 0);
  }
}
