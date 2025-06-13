/**	captive-portal-component

  Copyright (c) 2021 Jeremy Carter <jeremy@jeremycarter.ca>

  This code is released under the license terms contained in the
  file named LICENSE, which is found in the top-level folder in
  this project. You must agree to follow those license terms,
  otherwise you aren't allowed to copy, distribute, or use any
  part of this project in any way.

  Contains some modified example code from here:
  https://github.com/espressif/esp-idf/blob/release/v4.2/examples/protocols/http_server/simple/main/main.c
  https://github.com/espressif/esp-idf/blob/release/v4.2/examples/protocols/http_server/restful_server/main/rest_server.c

  Original Example Code Header:
  This example code is in the Public Domain (or CC0 licensed, at your option.)

  Unless required by applicable law or agreed to in writing, this
  software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
  CONDITIONS OF ANY KIND, either express or implied.
*/
#include "captive-portal-httpd.h"

#include <string.h>
#include <fcntl.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_vfs_semihost.h"
#include "esp_vfs_fat.h"
#include "esp_vfs.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "esp_http_server.h"



/** @brief This is where you redirect to whatever DNS address you prefer to open the captive portal page.
 *
 *  This DNS address will be displayed at the top of the
 *  page, so maybe you want to choose a nice name to use (it can be any legal DNS name that you prefer).
 */

static const char *HTTPD_TAG = "cap-httpd";

/* Send HTTP response with the contents of the requested file */
static esp_err_t common_get_handler(httpd_req_t *req)
{
  size_t req_hdr_host_len = httpd_req_get_hdr_value_len(req, "Host");
  char req_hdr_host_val[req_hdr_host_len + 1];

  esp_err_t res = httpd_req_get_hdr_value_str(req, "Host", (char *)&req_hdr_host_val, sizeof(char) * req_hdr_host_len + 1);
  if (res == ESP_OK)
  {
    ESP_LOGI(HTTPD_TAG, "Got HOST header value: %s", req_hdr_host_val);

    const char redir_trigger_host[] = "connectivitycheck.gstatic.com";
    if (strncmp(req_hdr_host_val, redir_trigger_host, strlen(redir_trigger_host)) == 0)
    {

      const char resp[] = "302 Found";
      ESP_LOGI(HTTPD_TAG, "Detected redirect trigger HOST: %s", redir_trigger_host);
      httpd_resp_set_status(req, resp);
      httpd_resp_set_hdr(req, "Location", DNS_PAGE_NAME);
      return httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    }
  }

  httpd_resp_sendstr(req, "You have been captured");
  return ESP_OK;
}

static void start_httpd(void *pvParameter)
{
  /** HTTP server */
  httpd_handle_t server = NULL;

  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.uri_match_fn = httpd_uri_match_wildcard;
  config.lru_purge_enable = true;

  if (httpd_start(&server, &config) != ESP_OK)
  {
    ESP_LOGE(HTTPD_TAG, "Failed to start HTTP Server.");
    return;
  }
  ESP_LOGI(HTTPD_TAG, "Started HTTP Server.");

  ESP_LOGI(HTTPD_TAG, "Registering HTTP server URI handlers...");

  /** URI handler */
  httpd_uri_t common_get_uri = {
      .uri = "/*",
      .method = HTTP_GET,
      .handler = common_get_handler,
    };

  httpd_register_uri_handler(server, &common_get_uri);

  ESP_LOGI(HTTPD_TAG, "Registered HTTP server URI handlers.");

  return;
}

void wifi_captive_portal_esp_idf_httpd_init(void)
{
  start_httpd(NULL);
}
