/**	captive-portal-component

  Copyright (c) 2021 Jeremy Carter <jeremy@jeremycarter.ca>

  This code is released under the license terms contained in the
  file named LICENSE, which is found in the top-level folder in
  this project. You must agree to follow those license terms,
  otherwise you aren't allowed to copy, distribute, or use any
  part of this project in any way.
*/
#include "captive-portal.h"
#include "captive-portal-dns.h"
#include "captive-portal-httpd.h"


void captive_portal_init(void)
{  
  wifi_captive_portal_esp_idf_dns_init();
  wifi_captive_portal_esp_idf_httpd_init();  
}
