#ifndef __WIFI_CAPTIVE_PORTAL_ESP_IDF_COMPONENT_WIFI_CAPTIVE_PORTAL_ESP_IDF_HTTPD_H_INCLUDED__
#define __WIFI_CAPTIVE_PORTAL_ESP_IDF_COMPONENT_WIFI_CAPTIVE_PORTAL_ESP_IDF_HTTPD_H_INCLUDED__
/**	captive-portal-component

  Copyright (c) 2021 Jeremy Carter <jeremy@jeremycarter.ca>

  This code is released under the license terms contained in the
  file named LICENSE, which is found in the top-level folder in
  this project. You must agree to follow those license terms,
  otherwise you aren't allowed to copy, distribute, or use any
  part of this project in any way.

  Contains some modified example code from here:
  https://github.com/espressif/esp-idf/blob/release/v4.2/examples/protocols/openssl_server/main/openssl_server_example.h

  Original Example Code Header:
  This example code is in the Public Domain (or CC0 licensed, at your option.)

  Unless required by applicable law or agreed to in writing, this
  software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
  CONDITIONS OF ANY KIND, either express or implied.
*/


#define DNS_PAGE_NAME "http://wifi-captive-portal/"

#ifdef __cplusplus
extern "C"
{
#endif

  /** The esp-idf task function. */
  void wifi_captive_portal_esp_idf_httpd_init(void);

#ifdef __cplusplus
}
#endif

#endif
