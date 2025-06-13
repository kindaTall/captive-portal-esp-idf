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

#define cap_ssid "esp32_CaptivePortal"
#define cap_pwd "esp32pwd"
#define cap_ip_gw "192.213.16.19"

#ifdef __cplusplus
extern "C"
{
#endif

  /** The esp-idf task function. */
  void example_wifi_init(void);

#ifdef __cplusplus
}
#endif

