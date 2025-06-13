/**	captive-portal-component

  Copyright (c) 2021 Jeremy Carter <jeremy@jeremycarter.ca>

  This code is released under the license terms contained in the
  file named LICENSE, which is found in the top-level folder in
  this project. You must agree to follow those license terms,
  otherwise you aren't allowed to copy, distribute, or use any
  part of this project in any way.

  Contains some modified example code from here:
  https://github.com/espressif/esp-idf/blob/release/v4.2/examples/system/ota/advanced_https_ota/main/advanced_https_ota_example.c

  Original Example Code Header:
  This code is in the Public Domain (or CC0 licensed, at your option.)

  Unless required by applicable law or agreed to in writing, this
  software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
  CONDITIONS OF ANY KIND, either express or implied.
*/
#include "example-wifi.h"

#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "lwip/inet.h"
#include "lwip/ip4_addr.h"
#include "lwip/dns.h"

const char *TAG = "cap-wifi";



static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
  if (event_id == WIFI_EVENT_AP_STACONNECTED)
  {
    wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
    ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
             MAC2STR(event->mac), event->aid);
  }
  else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
  {
    wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
    ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
             MAC2STR(event->mac), event->aid);
  }
}

/** NOTE: This is where you set the access point (AP) IP address
    and gateway address. It has to be a class A internet address
    otherwise the captive portal sign-in prompt won't show up	on
    Android when you connect to the access point. */

static void wifi_captive_portal_esp_idf_wifi_ap_init(void)
{
  const int max_connection = 4;
  const int channel = 5;

  wifi_config_t wifi_config_ap = {
      .ap = {
          .ssid = cap_ssid,
          .ssid_len = strlen(cap_ssid),
          .channel = channel,
          .password = cap_pwd,
          .max_connection = max_connection,
          .authmode = WIFI_AUTH_WPA_WPA2_PSK},
  };
  if (strlen(CONFIG_EXAMPLE_WIFI_AP_PASSWORD) == 0)
  {
    wifi_config_ap.ap.authmode = WIFI_AUTH_OPEN;
  }

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config_ap));

  ESP_LOGI(TAG, "starting WiFi access point: SSID: %s password:%s channel: %d",
           CONFIG_EXAMPLE_WIFI_AP_SSID, CONFIG_EXAMPLE_WIFI_AP_PASSWORD, CONFIG_EXAMPLE_WIFI_AP_CHANNEL);
}

void example_wifi_init(void)
{
  static bool wifi_is_init = false;

  /** Init only once. */
  if (wifi_is_init)
  {
    return;
  }
  wifi_is_init = true;

  esp_err_t err = esp_event_loop_create_default();
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
  {
    ESP_LOGE(TAG, "Failed to create event loop: %s", esp_err_to_name(err));
    return;
  }


  ESP_ERROR_CHECK(esp_netif_init());
  esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
  assert(ap_netif);

  esp_netif_ip_info_t ip_info;
  ip4_addr_t addr;
  ipaddr_aton(cap_ip_gw, (ip_addr_t *)&addr);
  ip_info.ip.addr = addr.addr;
  ip_info.gw.addr = addr.addr;
  IP4_ADDR(&ip_info.netmask, 255, 0, 0, 0);

  esp_netif_dhcps_stop(ap_netif);
  esp_netif_set_ip_info(ap_netif, &ip_info);
  esp_netif_dhcps_start(ap_netif);

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  wifi_captive_portal_esp_idf_wifi_ap_init();

  esp_event_handler_instance_t instance_any_id;

  ESP_ERROR_CHECK(
      esp_event_handler_instance_register(
          WIFI_EVENT,
          ESP_EVENT_ANY_ID,
          &wifi_event_handler,
          NULL,
          &instance_any_id));

  ESP_ERROR_CHECK(esp_wifi_start());
}

