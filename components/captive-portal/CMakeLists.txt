# captive-portal-component
#
# Copyright (c) 2021 Jeremy Carter <jeremy@jeremycarter.ca>
#
# This code is released under the license terms contained in the
# file named LICENSE, which is found in the top-level folder in
# this project. You must agree to follow those license terms,
# otherwise you aren't allowed to copy, distribute, or use any
# part of this project in any way.

idf_component_register(
  SRCS
    "captive-portal.c"
    "captive-portal-dns.c"
    "captive-portal-httpd.c"

  REQUIRES
     "nvs_flash" "fatfs" "lwip" "esp_http_server"

  INCLUDE_DIRS
    "include"

  PRIV_INCLUDE_DIRS
    "."

)
