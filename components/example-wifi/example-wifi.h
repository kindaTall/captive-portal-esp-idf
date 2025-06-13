#pragma once
/**	captive-portal-component

  Copyright (c) 2021 Jeremy Carter <jeremy@jeremycarter.ca>

  This code is released under the license terms contained in the
  file named LICENSE, which is found in the top-level folder in
  this project. You must agree to follow those license terms,
  otherwise you aren't allowed to copy, distribute, or use any
  part of this project in any way.
*/

/** An event base type for "captive-portal-wifi". */

#define EXAMPLE_WIFI_AP_SSID CONFIG_EXAMPLE_WIFI_AP_SSID
#define EXAMPLE_WIFI_AP_PASSWORD CONFIG_EXAMPLE_WIFI_AP_PASSWORD
#define EXAMPLE_WIFI_AP_IP_GW CONFIG_EXAMPLE_WIFI_AP_IP_GW
#define EXAMPLE_MAX_AP_CONN CONFIG_EXAMPLE_MAX_AP_CONN
#define EXAMPLE_WIFI_AP_CHANNEL CONFIG_EXAMPLE_WIFI_AP_CHANNEL

#ifdef __cplusplus
extern "C"
{
#endif

  /** The esp-idf task function. */
  void example_wifi_init(void);

#ifdef __cplusplus
}
#endif
